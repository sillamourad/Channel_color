#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <elf.h>
#include <sys/system_properties.h>

typedef unsigned int HI_U32;
typedef int          HI_S32;

typedef HI_S32 (*fn_get_snr_t)(HI_U32, HI_U32*);
typedef HI_S32 (*fn_get_quality_t)(HI_U32, HI_U32*);
typedef HI_S32 (*fn_get_strength_t)(HI_U32, HI_U32*);
typedef HI_S32 (*fn_get_ber_t)(HI_U32, HI_U32*);

static fn_get_snr_t      real_get_snr      = NULL;
static fn_get_quality_t  real_get_quality  = NULL;
static fn_get_strength_t real_get_strength = NULL;
static fn_get_ber_t      real_get_ber      = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t last_write = 0;
static HI_U32 last_snr=0, last_quality=0, last_strength=0, last_ber=0, last_tuner=0;

#define OUTPUT_FILE "/data/.snr_value.txt"
#define LOG_FILE    "/data/local/tmp/snr_hook.log"

static void log_msg(const char* fmt, ...) {
    FILE* fp = fopen(LOG_FILE, "a");
    if (!fp) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
    fclose(fp);
}

static void write_value_locked(void) {
    const char* tmp = "/data/.snr_value.tmp";
    int fd = open(tmp, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd < 0) return;
    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "tuner_id=%u\n"
        "snr_raw=%u\n"
        "snr_db=%.2f\n"
        "quality_pct=%u\n"
        "strength_pct=%u\n"
        "ber=%u\n"
        "timestamp=%ld\n",
        last_tuner, last_snr, (float)last_snr/100.0f,
        last_quality, last_strength, last_ber, (long)time(NULL));
    if (write(fd, buf, len) != len) { close(fd); unlink(tmp); return; }
    fsync(fd); close(fd);
    chmod(tmp, 0644);
    rename(tmp, OUTPUT_FILE);
}

// Find base address of f_server
static uintptr_t get_fsrv_base(void) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "f_server")) {
            unsigned long start, end;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                base = (uintptr_t)start;
                break;
            }
        }
    }
    fclose(fp);
    return base;
}

// Universal ELF symbol resolver from /proc/self/exe
static uintptr_t find_symbol_offset(const char* sym_name) {
    int fd = open("/proc/self/exe", O_RDONLY);
    if (fd < 0) return 0;

    Elf32_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) { close(fd); return 0; }
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) { close(fd); return 0; }

    Elf32_Shdr *shdrs = malloc(ehdr.e_shentsize * ehdr.e_shnum);
    if (!shdrs) { close(fd); return 0; }
    lseek(fd, ehdr.e_shoff, SEEK_SET);
    if (read(fd, shdrs, ehdr.e_shentsize * ehdr.e_shnum) != (ssize_t)(ehdr.e_shentsize * ehdr.e_shnum)) {
        free(shdrs); close(fd); return 0;
    }

    Elf32_Shdr *shstr_hdr = &shdrs[ehdr.e_shstrndx];
    char *shstrtab = malloc(shstr_hdr->sh_size);
    lseek(fd, shstr_hdr->sh_offset, SEEK_SET);
    read(fd, shstrtab, shstr_hdr->sh_size);

    Elf32_Shdr *symtab_hdr = NULL;
    Elf32_Shdr *strtab_hdr = NULL;

    for (int i = 0; i < ehdr.e_shnum; i++) {
        const char *name = shstrtab + shdrs[i].sh_name;
        if (strcmp(name, ".symtab") == 0) symtab_hdr = &shdrs[i];
        if (strcmp(name, ".strtab") == 0) strtab_hdr = &shdrs[i];
    }

    uintptr_t result = 0;
    if (symtab_hdr && strtab_hdr) {
        char *strtab = malloc(strtab_hdr->sh_size);
        lseek(fd, strtab_hdr->sh_offset, SEEK_SET);
        read(fd, strtab, strtab_hdr->sh_size);

        int num_syms = symtab_hdr->sh_size / sizeof(Elf32_Sym);
        Elf32_Sym *syms = malloc(symtab_hdr->sh_size);
        lseek(fd, symtab_hdr->sh_offset, SEEK_SET);
        read(fd, syms, symtab_hdr->sh_size);

        for (int i = 0; i < num_syms; i++) {
            const char *name = strtab + syms[i].st_name;
            if (strcmp(name, sym_name) == 0) {
                result = (uintptr_t)syms[i].st_value;
                break;
            }
        }
        free(syms);
        free(strtab);
    }

    free(shstrtab);
    free(shdrs);
    close(fd);
    return result;
}

static void* avl_poller_thread(void* arg) {
    (void)arg;
    log_msg("[avl] poller thread started in pid %d\n", getpid());

    uintptr_t base = get_fsrv_base();
    if (!base) {
        log_msg("[avl] ERROR: could not find f_server base address\n");
        return NULL;
    }
    log_msg("[avl] f_server base address: 0x%lx\n", base);

    typedef int (*fn_avl_snr_t)(short*, void*);
    typedef int (*fn_avl_str_t)(unsigned short*, void*);
    typedef int (*fn_avl_q_t)(unsigned short*, void*);

    // 1. Dynamic resolution via ELF symbol table
    uintptr_t off_snr  = find_symbol_offset("AVL62X1_GetSNR");
    uintptr_t off_str  = find_symbol_offset("AVL62X1_GetSignalStrength");
    uintptr_t off_q    = find_symbol_offset("AVL62X1_GetSignalQuality");
    uintptr_t off_chip = find_symbol_offset("g_AVL62X1_Chip");
    uintptr_t off_chan = find_symbol_offset("_tChannelStatus");

    // 2. Fallback if symbol parsing unavailable
    if (!off_snr || !off_chip) {
        char model[64] = {0};
        __system_property_get("ro.product.model", model);
        log_msg("[avl] ELF lookup missed, checking model: '%s'\n", model);
        if (strstr(model, "WEGOO") || strstr(model, "wegoo")) {
            off_snr  = 0x000a2b21;
            off_str  = 0x000a2cc1;
            off_q    = 0x000a2d49;
            off_chip = 0x000d78ac;
            off_chan = 0x00e3c5d8;
        } else {
            // IRON / IRON PRO / PLUS
            off_snr  = 0x000a9121;
            off_str  = 0x000a92c1;
            off_q    = 0x000a9349;
            off_chip = 0x000e58f0;
            off_chan = 0x00e4a6b8;
        }
    }

    log_msg("[avl] Resolved Offsets:\n"
            "      AVL62X1_GetSNR:            0x%lx\n"
            "      AVL62X1_GetSignalStrength: 0x%lx\n"
            "      AVL62X1_GetSignalQuality:  0x%lx\n"
            "      g_AVL62X1_Chip:            0x%lx\n"
            "      _tChannelStatus:           0x%lx\n",
            off_snr, off_str, off_q, off_chip, off_chan);

    fn_avl_snr_t avl_get_snr = (fn_avl_snr_t)(base + off_snr);
    fn_avl_str_t avl_get_str = (fn_avl_str_t)(base + off_str);
    fn_avl_q_t   avl_get_q   = (fn_avl_q_t)(base + off_q);
    void *pChip              = (void*)(base + off_chip);

    sleep(1);

    while (1) {
        short raw_snr = 0;
        unsigned short raw_str = 0, raw_q = 0;
        
        // Read chip state
        uint16_t *chip_head = (uint16_t*)pChip;
        if (chip_head && *chip_head != 0) {
            int ret = avl_get_snr(&raw_snr, pChip);
            if (ret == 0) {
                avl_get_str(&raw_str, pChip);
                avl_get_q(&raw_q, pChip);

                // Channel status fallback for strength/quality
                uint32_t t_str = 0, t_q = 0;
                if (off_chan) {
                    uint32_t *tchan = (uint32_t*)(base + off_chan);
                    if (tchan) {
                        t_str = tchan[0x14 / 4];
                        t_q   = tchan[0x18 / 4];
                    }
                }

                pthread_mutex_lock(&mutex);
                last_tuner = 0;
                last_snr = (HI_U32)(raw_snr > 0 ? raw_snr : 0);
                last_strength = (t_str > 0) ? t_str : (HI_U32)raw_str;
                last_quality  = (t_q > 0) ? t_q : (HI_U32)raw_q;
                time_t now = time(NULL);
                if (now != last_write) {
                    last_write = now;
                    write_value_locked();
                }
                pthread_mutex_unlock(&mutex);
            }
        }
        usleep(300000); // 300ms polling for responsive SNR bar
    }
    return NULL;
}

__attribute__((constructor))
static void init_hook(void) {
    void* h = dlopen("/system/lib/libhi_msp.so", RTLD_LAZY);
    if (!h) h = RTLD_NEXT;
    if (h) {
        real_get_snr      = (fn_get_snr_t)     dlsym(h, "HI_UNF_TUNER_GetSNR");
        real_get_quality  = (fn_get_quality_t) dlsym(h, "HI_UNF_TUNER_GetSignalQuality");
        real_get_strength = (fn_get_strength_t)dlsym(h, "HI_UNF_TUNER_GetSignalStrength");
        real_get_ber      = (fn_get_ber_t)     dlsym(h, "HI_UNF_TUNER_GetBER");
    }

    char comm[64] = {0};
    int fd = open("/proc/self/comm", O_RDONLY);
    if (fd >= 0) {
        read(fd, comm, sizeof(comm) - 1);
        close(fd);
    }
    if (strstr(comm, "f_server")) {
        pthread_t tid;
        pthread_create(&tid, NULL, avl_poller_thread, NULL);
        pthread_detach(tid);
    }
}

HI_S32 HI_UNF_TUNER_GetSNR(HI_U32 tunerId, HI_U32 *snr) {
    if (!real_get_snr) init_hook();
    if (!real_get_snr) return -1;
    HI_S32 ret = real_get_snr(tunerId, snr);
    if (ret == 0 && snr) {
        pthread_mutex_lock(&mutex);
        last_tuner = tunerId;
        last_snr = *snr;
        time_t now = time(NULL);
        if (now != last_write) { last_write = now; write_value_locked(); }
        pthread_mutex_unlock(&mutex);
    }
    return ret;
}

HI_S32 HI_UNF_TUNER_GetSignalQuality(HI_U32 t, HI_U32 *q) {
    if (!real_get_quality) init_hook();
    if (!real_get_quality) return -1;
    HI_S32 ret = real_get_quality(t, q);
    if (ret == 0 && q) { pthread_mutex_lock(&mutex); last_quality=*q; pthread_mutex_unlock(&mutex); }
    return ret;
}

HI_S32 HI_UNF_TUNER_GetSignalStrength(HI_U32 t, HI_U32 *s) {
    if (!real_get_strength) init_hook();
    if (!real_get_strength) return -1;
    HI_S32 ret = real_get_strength(t, s);
    if (ret == 0 && s) { pthread_mutex_lock(&mutex); last_strength=*s; pthread_mutex_unlock(&mutex); }
    return ret;
}

HI_S32 HI_UNF_TUNER_GetBER(HI_U32 t, HI_U32 *b) {
    if (!real_get_ber) init_hook();
    if (!real_get_ber) return -1;
    HI_S32 ret = real_get_ber(t, b);
    if (ret == 0 && b) { pthread_mutex_lock(&mutex); last_ber=*b; pthread_mutex_unlock(&mutex); }
    return ret;
}
