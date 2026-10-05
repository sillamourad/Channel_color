#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <asm/ptrace.h>

static pid_t find_pid_by_name(const char* name) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "pidof %s", name);
    FILE* fp = popen(cmd, "r");
    if (!fp) return -1;
    pid_t pid = -1;
    if (fscanf(fp, "%d", &pid) != 1) pid = -1;
    pclose(fp);
    return pid;
}

static int is_already_loaded(pid_t pid, const char* lib_substr) {
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE* fp = fopen(path, "r");
    if (!fp) return 0;
    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, lib_substr)) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

static uintptr_t get_module_base(pid_t pid, const char* mod_name) {
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE* fp = fopen(path, "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, mod_name) && strstr(line, "r-xp")) {
            unsigned long start = 0;
            if (sscanf(line, "%lx-", &start) == 1) {
                base = (uintptr_t)start;
                break;
            }
        }
    }
    fclose(fp);
    return base;
}

static int write_mem(pid_t pid, uintptr_t addr, const void* src, size_t len) {
    size_t words = (len + 3) / 4;
    const uint32_t* p = (const uint32_t*)src;
    for (size_t i = 0; i < words; ++i) {
        uint32_t val = 0;
        size_t rem = len - i * 4;
        if (rem >= 4) {
            val = p[i];
        } else {
            // Read-modify-write for partial word
            errno = 0;
            long orig = ptrace(PTRACE_PEEKTEXT, pid, (void*)(addr + i * 4), NULL);
            if (errno != 0) return -1;
            memcpy(&orig, (const char*)src + i * 4, rem);
            val = (uint32_t)orig;
        }
        if (ptrace(PTRACE_POKETEXT, pid, (void*)(addr + i * 4), (void*)(uintptr_t)val) != 0) {
            return -1;
        }
    }
    return 0;
}

static int read_mem(pid_t pid, uintptr_t addr, void* dst, size_t len) {
    size_t words = (len + 3) / 4;
    uint32_t* p = (uint32_t*)dst;
    for (size_t i = 0; i < words; ++i) {
        errno = 0;
        long val = ptrace(PTRACE_PEEKTEXT, pid, (void*)(addr + i * 4), NULL);
        if (errno != 0) return -1;
        size_t rem = len - i * 4;
        if (rem >= 4) {
            p[i] = (uint32_t)val;
        } else {
            memcpy((char*)dst + i * 4, &val, rem);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    const char* target_process = "f_server";
    const char* so_path = "/data/plugin/ColorPro_data/libsnr_hook.so";

    if (argc >= 2) so_path = argv[1];
    if (argc >= 3) target_process = argv[2];

    printf("[inject] Target: %s, Library: %s\n", target_process, so_path);

    if (access(so_path, F_OK) != 0) {
        fprintf(stderr, "[inject] Error: Library %s not found on disk\n", so_path);
        return 1;
    }

    pid_t pid = find_pid_by_name(target_process);
    if (pid <= 0) {
        fprintf(stderr, "[inject] Target process %s not running\n", target_process);
        return 1;
    }
    printf("[inject] Found %s PID=%d\n", target_process, (int)pid);

    if (is_already_loaded(pid, "libsnr_hook.so")) {
        printf("[inject] libsnr_hook.so is already loaded in PID %d. Nothing to do.\n", (int)pid);
        return 0;
    }

    uintptr_t linker_base = get_module_base(pid, "/system/bin/linker");
    if (!linker_base) {
        fprintf(stderr, "[inject] Failed to find /system/bin/linker base in PID %d\n", (int)pid);
        return 1;
    }
    printf("[inject] Linker base: 0x%08lx\n", (unsigned long)linker_base);

    // In Android 7 (Nougat, API 24), __dl_dlopen is at offset 0x2c39 (Thumb) in /system/bin/linker
    uintptr_t dlopen_addr = linker_base + 0x00002c38; // Even address for instruction, Thumb bit handled later

    uintptr_t fsrv_base = get_module_base(pid, target_process);
    if (!fsrv_base) {
        fprintf(stderr, "[inject] Failed to find %s base in PID %d\n", target_process, (int)pid);
        return 1;
    }
    printf("[inject] %s base: 0x%08lx\n", target_process, (unsigned long)fsrv_base);

    // Attach to target
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) != 0) {
        perror("[inject] ptrace(ATTACH) failed");
        return 1;
    }

    int status = 0;
    if (waitpid(pid, &status, WUNTRACED) != pid || !WIFSTOPPED(status)) {
        perror("[inject] waitpid failed after ATTACH");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }
    printf("[inject] Attached to PID %d (stopped by signal %d)\n", (int)pid, WSTOPSIG(status));

    struct pt_regs orig_regs, cur_regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &orig_regs) != 0) {
        perror("[inject] ptrace(GETREGS) failed");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }
    cur_regs = orig_regs;

    // Entry point of f_server is 0x11ea8 (ARM mode), test_dummy is 0x1428
    uintptr_t bkpt_addr = fsrv_base + 0x11ea8;
    int is_bkpt_thumb = 0;
    if (strstr(target_process, "dummy")) {
        bkpt_addr = fsrv_base + 0x1428;
        is_bkpt_thumb = 1;
    }

    uint32_t orig_code = 0;
    if (read_mem(pid, bkpt_addr & ~1, &orig_code, 4) != 0) {
        fprintf(stderr, "[inject] Failed to read original code at 0x%08lx\n", (unsigned long)bkpt_addr);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    uint32_t patch_code = is_bkpt_thumb ? ((orig_code & 0xFFFF0000) | 0x0000de01) : 0xe1200070;
    if (write_mem(pid, bkpt_addr & ~1, &patch_code, 4) != 0) {
        fprintf(stderr, "[inject] Failed to write bkpt at 0x%08lx\n", (unsigned long)bkpt_addr);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    // Write library path string to stack red-zone
    size_t path_len = strlen(so_path) + 1;
    uintptr_t path_addr = (cur_regs.ARM_sp - 256) & ~7;
    uint8_t orig_stack[256];
    read_mem(pid, path_addr, orig_stack, sizeof(orig_stack));
    write_mem(pid, path_addr, so_path, path_len);

    uintptr_t return_lr = is_bkpt_thumb ? (bkpt_addr | 1) : (bkpt_addr & ~1);

    // Prepare register state for __dl_dlopen:
    // r0 = const char* filename
    // r1 = int flags (RTLD_NOW = 2)
    // lr = return address
    // pc = dlopen_addr & ~1
    // cpsr |= 0x20 (Thumb mode for __dl_dlopen)
    cur_regs.ARM_ORIG_r0 = -1; // Cancel syscall restart
    cur_regs.ARM_r7 = 0;
    cur_regs.ARM_r0 = path_addr;
    cur_regs.ARM_r1 = 2; // RTLD_NOW
    cur_regs.ARM_r2 = 0; // extinfo = NULL
    cur_regs.ARM_r3 = return_lr; // caller address
    cur_regs.ARM_lr = return_lr;
    cur_regs.ARM_pc = dlopen_addr & ~1;
    cur_regs.ARM_cpsr |= 0x20; // Ensure Thumb mode for linker
    cur_regs.ARM_sp = (path_addr - 64) & ~7; // 8-byte aligned stack frame below string

    if (ptrace(PTRACE_SETREGS, pid, NULL, &cur_regs) != 0) {
        perror("[inject] ptrace(SETREGS) failed");
        write_mem(pid, bkpt_addr, &orig_code, 4);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    // Continue target execution to run dlopen
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) != 0) {
        perror("[inject] ptrace(CONT) failed");
        write_mem(pid, bkpt_addr, &orig_code, 4);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    // Wait for the breakpoint signal (SIGTRAP)
    int stopped = 0;
    while (1) {
        pid_t w = waitpid(pid, &status, 0);
        if (w == pid) {
            if (WIFSTOPPED(status)) {
                int sig = WSTOPSIG(status);
                printf("[inject] Target stopped with signal %d\n", sig);
                if (sig == SIGTRAP || sig == SIGSEGV || sig == SIGILL) {
                    struct pt_regs chk_regs;
                    ptrace(PTRACE_GETREGS, pid, NULL, &chk_regs);
                    uintptr_t stopped_pc = chk_regs.ARM_pc & ~1;
                    printf("[inject] Target stopped with signal %d at PC=0x%08lx (bkpt_addr=0x%08lx), r0=0x%08lx\n",
                           sig, (unsigned long)stopped_pc, (unsigned long)bkpt_addr, (unsigned long)chk_regs.ARM_r0);
                    if (stopped_pc == (bkpt_addr & ~1) || stopped_pc == ((bkpt_addr + 2) & ~1) || stopped_pc == ((bkpt_addr + 4) & ~1)) {
                        stopped = 1;
                        break;
                    }
                    fprintf(stderr, "[inject] Fatal: signal %d not at bkpt_addr!\n", sig);
                    break;
                } else {
                    // Forward other signals and keep waiting
                    ptrace(PTRACE_CONT, pid, NULL, (void*)(uintptr_t)sig);
                }
            } else if (WIFEXITED(status)) {
                fprintf(stderr, "[inject] Target process unexpectedly exited\n");
                return 1;
            }
        }
    }

    // Check return value in r0
    struct pt_regs ret_regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &ret_regs);
    uintptr_t handle = ret_regs.ARM_r0;
    printf("[inject] dlopen returned handle: 0x%08lx\n", (unsigned long)handle);

    if (!handle) {
        char err_buf[512] = {0};
        read_mem(pid, linker_base + 0x6226a, err_buf, sizeof(err_buf) - 1);
        printf("[inject] Linker error buffer: %s\n", err_buf);
    }

    // Restore original code at bkpt_addr
    write_mem(pid, bkpt_addr, &orig_code, 4);

    // Restore original stack
    write_mem(pid, path_addr, orig_stack, sizeof(orig_stack));

    // Restore original registers
    ptrace(PTRACE_SETREGS, pid, NULL, &orig_regs);

    // Detach cleanly
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[inject] Detached cleanly from PID %d. Injection %s!\n", (int)pid, handle ? "SUCCESS" : "FAILED (handle is 0)");

    return handle ? 0 : 1;
}
