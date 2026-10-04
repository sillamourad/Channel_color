/* .ColorPro - improved version
 * Plugin for switching Launcher colors on Hisilicon Android TV boxes.
 * Supports both Icone Wegoo and Icone Iron Pro / Iron / Plus with Auto-Detection.
 */

#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <csignal>
#include <cstdarg>
#include <cerrno>
#include <cctype>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <vector>
#include <set>
#include <unordered_set>
#include <utility>
#include <sys/system_properties.h>

extern "C" {
#include "tweetnacl.h"

void randombytes(unsigned char* p, unsigned long long n) {
    (void)p; (void)n;
}
}

/* ---------- Paths & constants (A-2 Obfuscated) ---------- */
/* ---------- Secure String Obfuscation Engine (Phase A-2) ---------- */
static volatile uint32_t s_obf_seed = 0x6d2b79f5;

static inline uint8_t getObfByte(size_t index, size_t strId) {
    uint32_t seed = s_obf_seed;
    uint32_t a = seed ^ (uint32_t)(strId * 0x9e3779b1);
    uint32_t b = (uint32_t)(index * 0x85ebca77) ^ 0x3c6ef372;
    uint32_t c = (a + b) ^ (((a << 5) & 0xFFFFFFFF) | (a >> 27));
    c = (c * 0xc2b2ae3d) ^ (b >> 11);
    return (uint8_t)(c ^ (c >> 8) ^ (c >> 16) ^ (c >> 24));
}

static inline const char* deobf(size_t id, const uint8_t* enc, size_t len) {
    static __thread char s_ring[16][256];
    static __thread size_t s_idx = 0;
    s_idx = (s_idx + 1) & 15;
    char* buf = s_ring[s_idx];
    volatile const uint8_t* p = enc;
    if (len >= 256) len = 255;
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (char)(p[i] ^ getObfByte(i, id));
    }
    buf[len] = '\0';
    return buf;
}

static inline void wipeObfMemory() {
    static __thread char s_ring[16][256];
    volatile char* p = (volatile char*)s_ring;
    for (size_t i = 0; i < sizeof(s_ring); ++i) p[i] = 0;
}

static inline off_t decOff(uint32_t enc) {
    return (off_t)(enc ^ 0x5c3a71b4);
}


#define DATA_DIR "/data/plugin/ColorPro_data"
#define DATA_DIR_LEGACY "/data/plugin/ColorPro_data"
#define TARGET_BIN_PATH "/data/plugin/ColorPro"
#define TARGET_BIN_PNG "/data/plugin/ColorPro.png"
#define TARGET_BIN_PREV "/data/plugin/ColorPro.prev"
#define SERVICE_DB "/data/db/service.db"
#define SERVICE_TMP "/data/db/service.db.tmp"
#define SERVICE_DIR "/data/db"
#define SERVICE_DIR_FMT "/data/db/%s"
#define SERVICE_DB_JOURNAL "/data/db/service.db-journal"
#define SERVICE_DB_WAL "/data/db/service.db-wal"
#define SERVICE_DB_SHM "/data/db/service.db-shm"
#define OLD_FREE_DB "/data/plugin/ColorPro_data/free.db"
#define OLD_FREE_OPEN_DB "/data/plugin/ColorPro_data/free_open.db"
#define OLD_ENC_OPEN_DB "/data/plugin/ColorPro_data/enc_open.db"
#define OLD_ENC_CLOSED_DB "/data/plugin/ColorPro_data/enc_closed.db"
#define DALVIK_TARGET "/data/dalvik-cache/arm/system@priv-app@Launcher@Launcher.apk@classes.dex"
#define ART_PATH "/data/dalvik-cache/arm/system@framework@boot.art"
#define HUD_JAR "/data/plugin/ColorPro_data/OverlayHud.jar"
#define HUD_JAR_ALT "/data/plugin/ColorPro_data/OverlayHud.jar"
#define BACKUP_JAR_PATH "/data/plugin/ColorPro_data/OverlayHud.jar.bak"
#define HUD_TXT "/data/local/tmp/hud_colorpro.txt"
#define CLEAR_TXT "/data/local/tmp/clear_colorpro.txt"
#define WARM_TXT "/data/local/tmp/warm_colorpro.txt"
#define PID_FILE "/data/local/tmp/colorpro.pid"
#define DBG_FILE "/data/local/tmp/colorpro_debug.txt"
#define LOCAL_TMP_DIR "/data/local/tmp"
#define SVCLIST_JSON "/data/plugin/ColorPro_data/svclist.json"
#define SVCLIST_JSON_TMP "/data/plugin/ColorPro_data/svclist.tmp"
#define SQLITE3_BIN "/data/plugin/ColorPro_data/sqlite3"
#define VAR_ORIGINAL "/data/plugin/ColorPro_data/service_original.db"
#define VAR_FREE "/data/plugin/ColorPro_data/service_free.db"
#define VAR_FREE_OPEN "/data/plugin/ColorPro_data/service_free_open.db"
#define VAR_ENC_OPEN "/data/plugin/ColorPro_data/service_enc_open.db"
#define VAR_ENC_CLOSED "/data/plugin/ColorPro_data/service_enc_closed.db"
#define VAR_COUNTS "/data/plugin/ColorPro_data/variant_counts.txt"
#define CUR_CHANNELS "/data/plugin/ColorPro_data/current_channels.txt"
#define CUR_CHOICE_FILE "/data/plugin/ColorPro_data/current_choice.txt"
#define VAR_BUILD_TMP "/data/plugin/ColorPro_data/var_build.tmp"
#define REPORT_PATH "/data/plugin/ColorPro_data/report.txt"
#define REPORT_TMP "/data/plugin/ColorPro_data/report.tmp"
#define CHECK_TMP "/data/plugin/ColorPro_data/check.tmp"
#define DIAG_CHANNELS "/data/plugin/ColorPro_data/diag.txt"
#define UPDATE_DIR "/data/plugin/ColorPro_data/update"
#define UPDATE_LOCK_FILE "/data/plugin/ColorPro_data/update.lock"
#define UPDATE_GUARD_BIN "/data/plugin/ColorPro_data/update_guard"
#define UPDATE_OK_FILE "/data/plugin/ColorPro_data/update_ok"
#define UPDATE_PENDING_FILE "/data/plugin/ColorPro_data/update_pending"
#define UPDATE_STATE_FILE "/data/plugin/ColorPro_data/update_state"
#define BACKUP_BIN_PATH "/data/plugin/ColorPro_data/ColorPro.bak"
#define ORCA_KEYS_CACHE "/data/plugin/ColorPro_data/orca_open_keys.bin"
#define ORCA_KEYS_RAW "orca_open_keys.bin"
#define ORCA_KEYS_SLASH "/orca_open_keys.bin"
#define VARF_ORIGINAL "service_original.db"
#define VARF_FREE "service_free.db"
#define VARF_FREE_OPEN "service_free_open.db"
#define VARF_ENC_OPEN "service_enc_open.db"
#define VARF_ENC_CLOSED "service_enc_closed.db"
#define NAME_SERVICE_DB "service.db"
#define SLASH_SERVICE_DB "/service.db"
#define SLASH_CC_BIN "/ColorPro"
#define CC_NAME_RAW "ColorPro"
#define HUD_NAME_RAW "OverlayHud.jar"
#define UPDATE_BASE_URL "https://raw.githubusercontent.com/sillamourad/Channel_color/main/release"
static const uint8_t s_obf_LAUNCHER_DVB_DIR[] = { 0x64, 0xf4, 0x91, 0xe3, 0x23, 0x92, 0x77, 0xc1, 0x7c, 0xc0, 0x6d, 0xb6, 0xf2, 0xcd, 0xe3, 0x18, 0x1d, 0xbb, 0x34, 0x10, 0x59, 0x37, 0xe3, 0xc6, 0x5a, 0x35, 0x26, 0x46, 0x01, 0x47, 0x89, 0x23, 0xbd, 0xdc, 0x61 };
#define LAUNCHER_DVB_DIR deobf(64, s_obf_LAUNCHER_DVB_DIR, sizeof(s_obf_LAUNCHER_DVB_DIR))
static const uint8_t s_obf_LAUNCHER_HISI_DIR[] = { 0xd3, 0x1c, 0xd5, 0x45, 0xdd, 0x5d, 0xf9, 0x5c, 0x96, 0xf9, 0xd5, 0x0c, 0x12, 0x80, 0x4f, 0x2d, 0x22, 0x95, 0x9b, 0x1a, 0x81, 0xfe, 0xf2, 0x9f, 0xa2, 0x00, 0x6d, 0x71, 0xde, 0x35, 0x8c, 0x24, 0xe3, 0x85, 0x09, 0x2d, 0xae, 0x99, 0x5d, 0x15, 0x8e };
#define LAUNCHER_HISI_DIR deobf(65, s_obf_LAUNCHER_HISI_DIR, sizeof(s_obf_LAUNCHER_HISI_DIR))
static const uint8_t s_obf_PROC_NAME_LAUNCHER_DVB[] = { 0x86, 0xfd, 0xc7, 0x46, 0x2d, 0xd5, 0x8d, 0x8f, 0x9e, 0xa6, 0x20, 0x59, 0x26, 0x21, 0x13, 0x7a, 0xcf, 0x54, 0xc4, 0x69, 0x0b, 0xf4, 0xf3, 0x46 };
#define PROC_NAME_LAUNCHER_DVB deobf(66, s_obf_PROC_NAME_LAUNCHER_DVB, sizeof(s_obf_PROC_NAME_LAUNCHER_DVB))
static const uint8_t s_obf_PROC_NAME_LAUNCHER_HISI[] = { 0xd3, 0x4c, 0xf5, 0x61, 0xcc, 0xb3, 0x65, 0xb0, 0x94, 0xf6, 0xd9, 0x0c, 0x0f, 0xf2, 0x72, 0x3a, 0xce, 0x45, 0x82, 0xd4, 0x84, 0xc1, 0x8d, 0x33, 0x64, 0x47, 0xb2, 0xd4, 0xbb, 0xaf };
#define PROC_NAME_LAUNCHER_HISI deobf(67, s_obf_PROC_NAME_LAUNCHER_HISI, sizeof(s_obf_PROC_NAME_LAUNCHER_HISI))
static const uint8_t s_obf_CMD_RESTORECON[] = { 0xdc, 0xa9, 0xb0, 0xdc, 0x1c, 0x8d, 0xb4, 0xfb, 0x15, 0x0d, 0x56, 0x63, 0x61, 0xfb, 0x60, 0x19, 0x79, 0x03, 0x39, 0x1c, 0xbd, 0x8b, 0x38, 0xd1, 0xe1, 0x57, 0x05, 0xd5, 0x97, 0xde, 0xe1, 0x6a, 0x34, 0x02, 0xee, 0x5f, 0xe4, 0xe4, 0xec, 0x54, 0x34, 0x1d };
#define CMD_RESTORECON deobf(68, s_obf_CMD_RESTORECON, sizeof(s_obf_CMD_RESTORECON))
static const uint8_t s_obf_CMD_CHECK_DVB[] = { 0xb6, 0x59, 0xc4, 0x3e, 0xb1, 0xde, 0x7d, 0x9e, 0xc4, 0x36, 0x27, 0x9c, 0xda, 0x85, 0x20, 0x94, 0x04, 0x7c, 0xad, 0xdf, 0x47, 0xa4, 0x5b, 0x1b, 0xf5, 0xb3, 0x60, 0xb8, 0x3f, 0xb9, 0xd8, 0x4b, 0x3f, 0xda, 0xec, 0xda, 0xbf, 0xe9, 0xd2, 0xda, 0x14, 0xae, 0x6e, 0x0c, 0x1b };
#define CMD_CHECK_DVB deobf(69, s_obf_CMD_CHECK_DVB, sizeof(s_obf_CMD_CHECK_DVB))
static const uint8_t s_obf_CMD_OAT_FILE_ARG[] = { 0xec, 0x0f, 0x75, 0xfa, 0x4e, 0xae, 0x6c, 0x18, 0x01, 0xf5, 0x40, 0xfa, 0xf4, 0xb1, 0x31, 0x9e, 0x78, 0xca, 0xab, 0x51, 0x6b, 0x7d, 0x76, 0x1e, 0x4e, 0x18, 0x43, 0xf3, 0xac, 0xa1, 0x88, 0x71, 0x00, 0x05, 0xf4, 0x26, 0x71, 0xad, 0x2f, 0x43, 0x25, 0xcf, 0xf6, 0x28, 0x00, 0xd0, 0xc1, 0x21, 0x34, 0xe5, 0xc3, 0xd2, 0x8a, 0x1e, 0x8a, 0xff, 0x39, 0x97, 0xa4, 0xa2, 0x30, 0x04, 0x7f, 0x8f, 0x20, 0xc2, 0x86, 0xd8, 0x45, 0x10, 0xc2, 0xfd, 0xd2, 0xa6, 0xc1, 0x38, 0x22, 0x96, 0xea, 0x6d, 0x7c, 0x15, 0x99 };
#define CMD_OAT_FILE_ARG deobf(70, s_obf_CMD_OAT_FILE_ARG, sizeof(s_obf_CMD_OAT_FILE_ARG))
static const uint8_t s_obf_SQL_CREATE_INDEX[] = { 0x41, 0xa0, 0x1a, 0xc5, 0x5b, 0x21, 0x72, 0x30, 0xee, 0xfa, 0x21, 0x10, 0x14, 0xcb, 0x91, 0xa8, 0x1b, 0x22, 0x6e, 0x98, 0x4a, 0x05, 0xcb, 0x20, 0xba, 0x87, 0x3a, 0x43, 0x76, 0xf5, 0x92, 0xe4, 0x62, 0xa7, 0xd6, 0x29, 0x9d, 0xc8, 0xa4, 0x20, 0x05, 0x75, 0x59, 0xc7, 0xb0, 0x67, 0x64, 0x1a, 0x60, 0xf5, 0x42 };
#define SQL_CREATE_INDEX deobf(71, s_obf_SQL_CREATE_INDEX, sizeof(s_obf_SQL_CREATE_INDEX))
static const uint8_t s_obf_SQL_CREATE_TEMP_BEGIN[] = { 0xf9, 0xe2, 0xa9, 0x4f, 0xca, 0x09, 0x6f, 0xbb, 0xdf, 0x31, 0xcc, 0x72, 0xf8, 0x3e, 0x31, 0xe6, 0x09, 0xd3, 0xdb, 0x06, 0x42, 0x31, 0x83, 0xea, 0x66, 0x6a, 0x15, 0x79, 0x36, 0x0b, 0xb2, 0xfc, 0xc8, 0x72, 0xbf, 0xb8, 0xea, 0x3d, 0xcb, 0xdb, 0x3b, 0x05, 0x27, 0xdc, 0xd7, 0x39, 0xf6, 0x49, 0xe9, 0x70, 0xd2, 0xef, 0x8f, 0x8e, 0xc6, 0x86, 0x15 };
#define SQL_CREATE_TEMP_BEGIN deobf(72, s_obf_SQL_CREATE_TEMP_BEGIN, sizeof(s_obf_SQL_CREATE_TEMP_BEGIN))
static const uint8_t s_obf_SQL_INSERT_RULE_NL[] = { 0x1b, 0x0b, 0x67, 0xd4, 0x54, 0x40, 0xd5, 0x6c, 0xbd, 0x97, 0x13, 0xb4, 0xc5, 0x12, 0x91, 0x0c, 0xdd, 0x06, 0xb6, 0x4b, 0xad, 0xed, 0x22, 0xe6, 0x20, 0x0b, 0xf8, 0x18, 0x22, 0x1b, 0xc9, 0x3a, 0x2b, 0xed, 0x2b, 0x11, 0x2a, 0x77, 0xea };
#define SQL_INSERT_RULE_NL deobf(73, s_obf_SQL_INSERT_RULE_NL, sizeof(s_obf_SQL_INSERT_RULE_NL))
static const uint8_t s_obf_SQL_QUERY_RULES[] = { 0x12, 0xd8, 0x98, 0x96, 0xb2, 0x45, 0x0a, 0xa4, 0x35, 0xba, 0x11, 0x86, 0x88, 0xbd, 0xa3, 0x2f, 0xbe, 0x7c, 0x44, 0x99, 0xbe, 0x2c, 0xb4, 0x32, 0x25, 0xa7, 0xa7, 0xbe, 0x83, 0x64, 0xe0, 0xea, 0xb3, 0x72, 0x5a, 0x54, 0xf0, 0xdf, 0x50, 0x04, 0x0b, 0x59, 0xc5, 0x0b, 0xc9, 0xb3, 0xed, 0x07, 0x69, 0x3a, 0xb9, 0x54, 0xba, 0xd9, 0xb7, 0xfd, 0xef, 0xa7, 0x95, 0x46, 0xa1, 0x76, 0x1c, 0x6f, 0x22, 0xd0, 0x7c, 0x1a, 0x92, 0x69, 0x8c, 0x00, 0xf8, 0x6e, 0x20, 0x59, 0x63, 0x30, 0x44, 0xe9, 0xd6, 0x4b, 0xc3, 0x9a, 0x19, 0x90, 0xa9, 0xec, 0x86, 0x18, 0xaa, 0xec, 0x09, 0xd2, 0x79, 0x00, 0xde, 0x6f, 0x25, 0x3e, 0x69, 0x9c, 0x4b, 0xe8, 0x1f, 0xd8, 0x64, 0xa6, 0x2d, 0x37, 0xfe, 0xec, 0x69, 0x59, 0x92, 0x35, 0xfa, 0xf6, 0x2f, 0x80, 0x99, 0x88, 0xb2, 0x21, 0xa6, 0x4b, 0x0f, 0x8f, 0x0a, 0x8c, 0x5f, 0xdf, 0x9f, 0xb1, 0xbf, 0x21, 0x98, 0xd6, 0x4c, 0x53, 0x31, 0x33, 0xd5, 0xde, 0xec, 0x76, 0x0e, 0x72, 0x16, 0xb0, 0x2a, 0x76, 0xb5, 0xd7, 0x37, 0x68, 0x03, 0xbc, 0x4a, 0x26, 0xbd, 0x6d, 0x89, 0xf2, 0x61, 0x30, 0x2f, 0xa3, 0xa9, 0x3a, 0x64, 0x1a, 0xe0, 0xee, 0xa8, 0x31, 0x5c, 0x25, 0x55, 0x41 };
#define SQL_QUERY_RULES deobf(74, s_obf_SQL_QUERY_RULES, sizeof(s_obf_SQL_QUERY_RULES))
static const uint8_t s_obf_SQL_ATTACH_ENC[] = { 0x11, 0x68, 0x3a, 0x98, 0xb3, 0xa1, 0x7d, 0xb7, 0xf4, 0xc5, 0x7b, 0x79, 0x87, 0x3a, 0x26, 0xd8, 0x9a, 0x14, 0x5f, 0xee, 0xc3, 0xfe, 0x59, 0x9b, 0x07, 0xf1, 0x3c, 0xc5, 0x6b, 0xca, 0xb5, 0x2f, 0x96, 0xb5, 0xe2, 0x25, 0xe5, 0x8d, 0xfb, 0x67, 0xc0, 0x63, 0xac, 0x3a, 0x02, 0xa6, 0xfe, 0x89, 0x20, 0xbe, 0x16, 0xc7, 0x64, 0x5f, 0x0d, 0x9a, 0xeb, 0x5c, 0xa0, 0x74, 0x4b, 0xec, 0x3e };
#define SQL_ATTACH_ENC deobf(75, s_obf_SQL_ATTACH_ENC, sizeof(s_obf_SQL_ATTACH_ENC))
static const uint8_t s_obf_SQL_ATTACH_ENC_FMT[] = { 0x8c, 0xbe, 0x2f, 0x4f, 0xa2, 0x06, 0xa0, 0xd3, 0xf3, 0x1e, 0x82, 0x24, 0x96, 0xfb, 0x6b, 0x82, 0x58, 0xa7, 0x53 };
#define SQL_ATTACH_ENC_FMT deobf(76, s_obf_SQL_ATTACH_ENC_FMT, sizeof(s_obf_SQL_ATTACH_ENC_FMT))
static const uint8_t s_obf_SQL_COUNT_CAS_NL[] = { 0xcc, 0x00, 0xaf, 0xe7, 0x95, 0x58, 0xb7, 0xcf, 0x6b, 0x7b, 0xad, 0x0a, 0x4c, 0xcb, 0x2a, 0x73, 0x8d, 0x19, 0x0c, 0x98, 0x1a, 0x17, 0x14, 0xa1, 0xd7, 0x77, 0x95, 0x78, 0xb0, 0x74, 0xed, 0xbf, 0xa9, 0x89, 0x87, 0x5c, 0x49, 0x0e, 0x2c, 0xeb, 0x55, 0xac, 0x70, 0x20, 0xd2, 0xd4, 0x57, 0x02, 0x26, 0x16, 0x83, 0xc7, 0x58, 0xaa, 0xc0, 0x5b, 0x67, 0x7e, 0x31, 0xe1, 0x83, 0x5c, 0x9b, 0xf4, 0x70, 0xf1, 0x9d, 0xe6, 0x93, 0xa2, 0x8f };
#define SQL_COUNT_CAS_NL deobf(77, s_obf_SQL_COUNT_CAS_NL, sizeof(s_obf_SQL_COUNT_CAS_NL))
static const uint8_t s_obf_SQL_COUNT_ALL_NL[] = { 0x47, 0x6a, 0x1d, 0x84, 0x10, 0xcc, 0x0e, 0x10, 0xdd, 0x63, 0xbc, 0x56, 0x88, 0x70, 0xd2, 0x3c, 0x5d, 0x24, 0x15, 0x39, 0x76, 0x2c, 0xe2, 0xfd, 0xc0, 0x0a, 0xc5, 0x7b, 0xc3, 0xc7, 0x18, 0x09, 0xeb, 0xa5, 0xba, 0xed, 0xa6, 0xca, 0xfa, 0xcc, 0x73, 0xd3, 0x08, 0xed, 0x95, 0x8f, 0xe0 };
#define SQL_COUNT_ALL_NL deobf(78, s_obf_SQL_COUNT_ALL_NL, sizeof(s_obf_SQL_COUNT_ALL_NL))
static const uint8_t s_obf_SQL_ENC_MISSING_NL[] = { 0x2f, 0x55, 0xe4, 0x79, 0xd4, 0x91, 0x7a, 0x23, 0xe3, 0xa5, 0x6c, 0x41, 0x1a, 0x63, 0x88, 0xcb, 0x87, 0xb1, 0x64, 0x4b, 0xba, 0x6c, 0xd3, 0x8c, 0x0d, 0x00, 0x94, 0xdc, 0x84, 0xc9, 0xc5, 0xe6, 0x14, 0xd4, 0x03, 0x49, 0x7f, 0x57, 0x47, 0xd5, 0x17, 0x62, 0x62, 0x05, 0x84, 0x86, 0x45, 0x40, 0xdd, 0x3a, 0x87, 0x73 };
#define SQL_ENC_MISSING_NL deobf(79, s_obf_SQL_ENC_MISSING_NL, sizeof(s_obf_SQL_ENC_MISSING_NL))
static const uint8_t s_obf_FONT_COLOR_OPEN[] = { 0x9e, 0x73, 0xf0, 0x54, 0x64, 0x09, 0x5a, 0x4f, 0xd9, 0xf8, 0x53, 0x2d, 0xd4 };
#define FONT_COLOR_OPEN deobf(80, s_obf_FONT_COLOR_OPEN, sizeof(s_obf_FONT_COLOR_OPEN))
static const uint8_t s_obf_TEST_DISNEY[] = { 0x8e, 0x04, 0x94, 0xbe, 0x5e, 0x22, 0xb1, 0x88, 0xff, 0x61, 0x77, 0x92, 0x11, 0x66, 0xa7, 0xb1, 0xf0 };
#define TEST_DISNEY deobf(81, s_obf_TEST_DISNEY, sizeof(s_obf_TEST_DISNEY))
static const uint8_t s_obf_TEST_HIAOSVC[] = { 0x30, 0x2a, 0x79, 0x5f, 0x7f, 0xea, 0x48, 0x2f, 0x71, 0x33, 0x1d };
#define TEST_HIAOSVC deobf(82, s_obf_TEST_HIAOSVC, sizeof(s_obf_TEST_HIAOSVC))
static const uint8_t s_obf_MANIFEST_TXT[] = { 0x4f, 0xe8, 0xb0, 0x23, 0xbf, 0x0a, 0xf7, 0x7f, 0x3f, 0x34, 0xab, 0xbb, 0xd5 };
#define MANIFEST_TXT deobf(83, s_obf_MANIFEST_TXT, sizeof(s_obf_MANIFEST_TXT))
static const uint8_t s_obf_MANIFEST_SIG[] = { 0xab, 0xf0, 0x33, 0x9b, 0x28, 0x78, 0x01, 0x4a, 0xe8, 0x21, 0xec, 0x0f, 0x21 };
#define MANIFEST_SIG deobf(84, s_obf_MANIFEST_SIG, sizeof(s_obf_MANIFEST_SIG))
static const uint8_t s_obf_MANIFEST_TXT_PART[] = { 0x19, 0x24, 0xfd, 0x1f, 0x92, 0x11, 0x7e, 0x7c, 0x77, 0x7c, 0x95, 0xeb, 0x70, 0xcc, 0xe9, 0x09, 0x91, 0x1f };
#define MANIFEST_TXT_PART deobf(85, s_obf_MANIFEST_TXT_PART, sizeof(s_obf_MANIFEST_TXT_PART))
static const uint8_t s_obf_MANIFEST_SIG_PART[] = { 0x17, 0x5c, 0xbf, 0xd1, 0xe8, 0x89, 0x3f, 0x59, 0x08, 0xb0, 0x10, 0xb1, 0x2c, 0x79, 0xb0, 0x34, 0xb6, 0x7f };
#define MANIFEST_SIG_PART deobf(86, s_obf_MANIFEST_SIG_PART, sizeof(s_obf_MANIFEST_SIG_PART))
static const uint8_t s_obf_SYSTEM_BIN_FSERVER[] = { 0xaa, 0xc3, 0x53, 0x32, 0x87, 0xfd, 0x03, 0x31, 0x6d, 0x86, 0x32, 0x92, 0xa2, 0x89, 0x83, 0x8a, 0x69, 0x25, 0x76, 0xbb };
#define SYSTEM_BIN_FSERVER deobf(87, s_obf_SYSTEM_BIN_FSERVER, sizeof(s_obf_SYSTEM_BIN_FSERVER))
static const uint8_t s_obf_CMD_LAUNCH_STBSETTING[] = { 0xa6, 0xd5, 0xc3, 0x41, 0x83, 0x8e, 0xb5, 0xa6, 0x59, 0x74, 0x42, 0xaa, 0xd6, 0xea, 0x89, 0xce, 0x0d, 0x05, 0xcf, 0x95, 0x90, 0x7e, 0xff, 0xa1, 0xb3, 0x90, 0xa4, 0x00, 0x30, 0xcd, 0xff, 0x09, 0x7b, 0x3c, 0x16, 0x8e, 0xaa, 0xa8, 0x7a, 0xa3, 0xbf, 0x1d, 0x95, 0x2c, 0xfd, 0xc7, 0xe2, 0x67, 0x46, 0xdd, 0x6c, 0x5f, 0x84, 0x6c, 0x35, 0x34, 0x80, 0x48, 0xb0, 0x20, 0xca, 0x02, 0xe0, 0xad, 0xb8, 0xb7, 0x2e, 0x48, 0x2a, 0x54, 0x5a, 0xb4, 0xdf, 0x37, 0x39, 0x81, 0x12, 0x73, 0xa9, 0x99, 0x0b, 0xb4, 0x2b, 0x8a, 0xb7, 0x05, 0x85, 0x9f, 0xb6, 0x5b, 0x81, 0x9c, 0x0d, 0x40, 0xf3, 0x3c, 0x71, 0x0c, 0x14, 0xca };
#define CMD_LAUNCH_STBSETTING deobf(88, s_obf_CMD_LAUNCH_STBSETTING, sizeof(s_obf_CMD_LAUNCH_STBSETTING))
static const uint8_t s_obf_CMD_LAUNCH_HOME[] = { 0xb2, 0x24, 0x0d, 0x19, 0x00, 0x7c, 0x58, 0xa2, 0x53, 0x45, 0x98, 0x43, 0x0a, 0x8b, 0xf5, 0x4d, 0x44, 0xb9, 0xbc, 0x8d, 0x20, 0xc2, 0x88, 0xd2, 0x20, 0xe7, 0x15, 0x98, 0x55, 0x14, 0x1a, 0x35, 0x27, 0xfe, 0x35, 0x9c, 0x4b, 0x0e, 0xcc, 0x59, 0x74, 0xc4, 0xdc, 0xa5, 0x13, 0x2e, 0x31, 0x2a, 0xd2, 0x36, 0x0c, 0x22, 0x76, 0x82, 0xaf, 0xe2, 0x0d, 0x69, 0x61, 0x64, 0x2b, 0xf4, 0x2e, 0x2a, 0x5b, 0x78, 0x06, 0x66, 0x9c, 0x08, 0x64, 0x87, 0xab, 0xac, 0xce, 0xec, 0xad, 0xcf, 0xb3, 0xb6, 0x70, 0x36, 0x8e, 0x52, 0x7c, 0x9d, 0xf1, 0x1f, 0x37, 0x8e, 0x01, 0x98, 0xbc, 0xc9, 0xfa, 0x2c };
#define CMD_LAUNCH_HOME deobf(89, s_obf_CMD_LAUNCH_HOME, sizeof(s_obf_CMD_LAUNCH_HOME))
static const uint8_t s_obf_SERVICE_MASTER_BAK[] = { 0x0f, 0x90, 0xb0, 0x9a, 0x6b, 0x8e, 0x07, 0x37, 0xff, 0x26, 0xff, 0x3e, 0x28, 0x9c, 0x0a, 0x6c, 0x89, 0x39, 0x7b, 0xc7, 0xac, 0xde, 0xd4, 0xa6, 0xa8, 0x6a, 0xff };
#define SERVICE_MASTER_BAK deobf(90, s_obf_SERVICE_MASTER_BAK, sizeof(s_obf_SERVICE_MASTER_BAK))
static const uint8_t s_obf_ORCA_KEYS_PLUGIN[] = { 0x24, 0x4a, 0xf4, 0xb6, 0x6c, 0xd4, 0x86, 0x3e, 0xd3, 0xa7, 0xa7, 0x8d, 0xa7, 0x37, 0x3d, 0xab, 0x48, 0x7a, 0x3e, 0x39, 0x58, 0x11, 0x06, 0x94, 0xfb, 0x67, 0xba, 0x1a, 0xe3, 0x6e, 0xb3 };
#define ORCA_KEYS_PLUGIN deobf(91, s_obf_ORCA_KEYS_PLUGIN, sizeof(s_obf_ORCA_KEYS_PLUGIN))


#define HUD_JAR_DATA        HUD_JAR
#define TARGET_JAR_PATH     HUD_JAR
#define DEV                "/dev/input/event0"
#define MENU_TIMEOUT_SEC   12
#define POLL_TIMEOUT_SEC   30
#define POLL_TIMEOUT_MS    250
#define WARMUP_DELAY_US    1200000
#define APPLY_DELAY_US     300000
#define KILL_SETTLE_US     500000
#define SUCCESS_DELAY_SEC  2
#define ERROR_DELAY_SEC    1
#define OLD_INSTANCE_WAIT_MS 200
#define CHANNELS_TIMEOUT_SEC   20
#define CH_REBOOT_COUNT_SEC     3   /* countdown before the clean reboot      */

#define CHANNEL_DB_MIN_BYTES   (64 * 1024)
#define SQL_RUN_TIMEOUT_MS     60000
#define SQL_MIN_KEYS           10
#ifndef CC_VERSION_STRING
#define CC_VERSION_STRING "1.4"
#endif
#define CC_BUILD_VERSION  "v1.4-ColorPro"

/* Semantic versioning: CC_VERSION_NUM = major * 10000 + minor * 100
 *   "1.0" -> 10000, "1.1" -> 10100, "1.2" -> 10200, "1.3" -> 10300, "2.0" -> 20000
 * Keep CC_VERSION_STRING in sync with CC_VERSION_NUM. */
#ifndef CC_VERSION_NUM
#define CC_VERSION_NUM   10400
#endif

/* Display-only rendering of a version number.
 *  10000 -> "1.0"  (major*10000 + minor*100, minor always *100)
 *  legacy 4-digit codes (5001, 7003) are NOT semver shaped -> returned as-is.
 * NEVER use this to parse: manifest.txt is read with atoi(). */
static std::string versionToString(int v) {
    char buf[24];
    if (v >= 10000 && (v % 100) == 0)
        snprintf(buf, sizeof(buf), "%d.%d", v / 10000, (v % 10000) / 100);
    else
        snprintf(buf, sizeof(buf), "%d", v);
    return std::string(buf);
}

static int parseVersion(const char* s) {
    int a = 0, b = 0;
    if (sscanf(s, "%d.%d", &a, &b) != 2) return -1;
    if (a < 0 || b < 0 || b > 99) return -1;
    return a * 10000 + b * 100;
}

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
    bool isLegacy = false;
    int legacyNum = 0;
};

static SemVer parseSemVer(const std::string& str) {
    SemVer sv;
    if (str.empty()) return sv;
    bool isPureDigits = true;
    for (char c : str) {
        if (!isdigit((unsigned char)c)) { isPureDigits = false; break; }
    }
    if (isPureDigits && atoi(str.c_str()) >= 1000) {
        sv.isLegacy = true;
        sv.legacyNum = atoi(str.c_str());
        return sv;
    }
    const char* p = str.c_str();
    while (*p == ' ' || *p == 'v' || *p == 'V') p++;
    int n = sscanf(p, "%d.%d.%d", &sv.major, &sv.minor, &sv.patch);
    if (n < 2) {
        sv.minor = 0;
        sv.patch = 0;
        sscanf(p, "%d.%d", &sv.major, &sv.minor);
    }
    return sv;
}

static int compareVersions(const std::string& a, const std::string& b) {
    SemVer sa = parseSemVer(a);
    SemVer sb = parseSemVer(b);
    if (sa.isLegacy && sb.isLegacy) return sa.legacyNum - sb.legacyNum;
    if (!sa.isLegacy && sb.isLegacy) return 1;
    if (sa.isLegacy && !sb.isLegacy) return -1;
    if (sa.major != sb.major) return sa.major - sb.major;
    if (sa.minor != sb.minor) return sa.minor - sb.minor;
    return sa.patch - sb.patch;
}

static const uint8_t UPDATE_PUBKEY[32] = {
    0xab, 0x6a, 0x66, 0x33, 0x26, 0xd3, 0x2f, 0xff,
    0xfd, 0x0c, 0x25, 0xc0, 0xa2, 0xde, 0xf8, 0xb7,
    0xae, 0xbc, 0xc6, 0x04, 0xc5, 0xe9, 0x8b, 0x54,
    0x4f, 0x8f, 0x24, 0xc8, 0x15, 0xe1, 0x33, 0xb0
};

#define UPDATE_FALLBACK_URL ""
#define CURL_BIN            "/system/bin/curl"
#define MAX_MANIFEST_SIZE   4096
#define MAX_EXE_SIZE        (6 * 1024 * 1024)
#define MAX_JAR_SIZE        (2 * 1024 * 1024)

/* Process identification constants */
#define PROC_NAME_F_SERVER        "f_server"


/* HUD channel-switching messages */
static const char* M_CH_RESTARTING = "\xd8\xac\xd8\xa7\xd8\xb1\xd9\x8a\x20\xd8\xa5\xd8\xb9\xd8\xa7\xd8\xaf\xd8\xa9\x20\xd8\xaa\xd8\xb4\xd8\xba\xd9\x8a\xd9\x84\x20\xd8\xa7\xd9\x84\xd8\xae\xd8\xaf\xd9\x85\xd8\xa7\xd8\xaa\x2e\x2e\x2e"; /* جاري إعادة تشغيل الخدمات... */
static const char* M_CH_REBOOT_HINT = "\xd8\xa3\xd8\xb9\xd8\xaf\x20\xd8\xaa\xd8\xb4\xd8\xba\xd9\x8a\xd9\x84\x20\xd8\xa7\xd9\x84\xd8\xac\xd9\x87\xd8\xa7\xd8\xb2\x20\xd9\x84\xd8\xa5\xd8\xaa\xd9\x85\xd8\xa7\xd9\x85\x20\xd8\xa7\xd9\x84\xd8\xaa\xd8\xa8\xd8\xaf\xd9\x8a\xd9\x84"; /* أعد تشغيل الجهاز لإتمام التبديل */
static const char* M_CH_NEED_REBOOT_DMX = "\xd9\x84\xd8\xaa\xd9\x81\xd8\xb9\xd9\x8a\xd9\x84\x20\xd8\xa7\xd9\x84\xd9\x82\xd9\x86\xd9\x88\xd8\xa7\xd8\xaa\x20\xd9\x8a\xd9\x84\xd8\xb2\xd9\x85\x20\xd8\xa5\xd8\xb9\xd8\xa7\xd8\xaf\xd8\xa9\x20\xd8\xa7\xd9\x84\xd8\xaa\xd8\xb4\xd8\xba\xd9\x8a\xd9\x84"; /* لتفعيل القنوات يلزم إعادة التشغيل */
static const char* M_CH_SWITCHING_AR  = "\xd8\xac\xd8\xa7\xd8\xb1\xd9\x8a\x20\xd8\xaa\xd8\xa8\xd8\xaf\xd9\x8a\xd9\x84\x20\xd8\xa7\xd9\x84\xd9\x82\xd9\x86\xd9\x88\xd8\xa7\xd8\xaa\x20\x2e\x2e\x2e\x2e\xd8\xb3\xd9\x8a\xd8\xb9\xd9\x8a\xd8\xaf\x20\xd8\xa7\xd9\x84\xd8\xac\xd9\x87\xd8\xa7\xd8\xb2\x20\xd8\xa7\xd9\x84\xd8\xaa\xd8\xb4\xd8\xba\xd9\x8a\xd9\x84\x20\xd8\xae\xd9\x84\xd8\xa7\xd9\x84\x20";
/* جاري تبديل القنوات ....سيعيد الجهاز التشغيل خلال  */
static const char* M_CH_SECONDS_AR    = "\x20\xd8\xab\xd9\x88\xd8\xa7\xd9\x86\xd9\x8a";
/*  ثواني */
static const char* M_CH_SWITCHING_EN  = "\x53\x77\x69\x74\x63\x68\x69\x6E\x67\x20\x63\x68\x61\x6E\x6E\x65\x6C\x73\x2E\x2E\x2E\x20\x44\x65\x76\x69\x63\x65\x20\x77\x69\x6C\x6C\x20\x72\x65\x62\x6F\x6F\x74\x20\x69\x6E\x20";
/* Switching channels... Device will reboot in  */
static const char* M_CH_ALREADY_AR    = "\xD8\xA3\xD9\x86\xD8\xAA\x20\xD8\xAA\xD8\xB3\xD8\xAA\xD8\xAE\xD8\xAF\xD9\x85\x20\xD9\x87\xD8\xB0\xD8\xA7\x20\xD8\xA7\xD9\x84\xD9\x86\xD9\x88\xD8\xB9\x20\xD8\xA8\xD8\xA7\xD9\x84\xD9\x81\xD8\xB9\xD9\x84";
/* أنت تستخدم هذا النوع بالفعل */
static const char* M_CH_ALREADY_EN    = "\x59\x6F\x75\x20\x61\x72\x65\x20\x61\x6C\x72\x65\x61\x64\x79\x20\x75\x73\x69\x6E\x67\x20\x74\x68\x69\x73\x20\x63\x68\x61\x6E\x6E\x65\x6C\x20\x73\x65\x74";
/* You are already using this channel set */
static const char* M_CH_CANCEL_AR     = "\xD8\xAA\xD9\x85\x20\xD8\xA5\xD9\x84\xD8\xBA\xD8\xA7\xD9\x85\x20\xD8\xA5\xD8\xB9\xD8\xA7\xD8\xAF\xD8\xA9\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xB4\xD8\xBA\xD9\x8A\xD9\x84";
/* تم إلغاء إعادة التشغيل */
static const char* M_CH_CANCEL_EN     = "\x52\x65\x62\x6F\x6F\x74\x20\x63\x61\x6E\x63\x65\x6C\x6C\x65\x64\x20\x2D\x20\x6E\x6F\x20\x72\x65\x62\x6F\x6F\x74";
/* Reboot cancelled - no reboot */
static const char* M_CH_REBOOT_NOW_AR = "\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xA5\xD8\xB9\xD8\xA7\xD8\xAF\xD8\xA9\x20\xD8\xAA\xD8\xB4\xD8\xBA\xD9\x8A\xD9\x84\x20\xD8\xA7\xD9\x84\xD8\xAC\xD9\x87\xD8\xA7\xD8\xB2\x2E\x2E\x2E";
/* جاري إعادة تشغيل الجهاز... */
static const char* M_CH_REBOOT_NOW_EN = "\x52\x65\x62\x6F\x6F\x74\x69\x6E\x67\x20\x6E\x6F\x77\x2E\x2E\x2E";
/* Rebooting now... */
static const char* M_CH_CANCELHINT_AR = "\x45\x58\x49\x54\x20\xD9\x84\xD9\x84\xD8\xA5\xD9\x84\xD8\xBA\xD8\xA7\xD9\x85\x20\x2D\x20\xD8\xA3\xD9\x88\x20\x31\x2D\x34\x20\xD9\x84\xD8\xA7\xD8\xAE\xD8\xAA\xD9\x8A\xD8\xA7\xD8\xB1\x20\xD9\x86\xD9\x88\xD8\xB9\x20\xD8\xA2\xD8\xAE\xD8\xB1";
/* EXIT للإلغاء - أو 1-4 لاختيار نوع آخر */
static const char* M_CH_CANCELHINT_EN = "\x45\x58\x49\x54\x20\x3D\x20\x63\x61\x6E\x63\x65\x6C\x20\x7C\x20\x31\x2D\x34\x20\x3D\x20\x63\x68\x6F\x6F\x73\x65\x20\x61\x6E\x6F\x74\x68\x65\x72\x20\x73\x65\x74";
/* EXIT = cancel | 1-4 = choose another set */
static const char* M_CH_BADDB_AR      = "\xD9\x85\xD9\x84\xD9\x81\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\x20\xD8\xAA\xD8\xA7\xD9\x84\xD9\x81\x20\x2D\x20\xD9\x84\xD9\x85\x20\xD9\x8A\xD8\xAA\xD9\x85\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xA8\xD8\xAF\xD9\x8A\xD9\x84";
/* ملف القنوات تالف - لم يتم التبديل */
static const char* M_CH_BADDB_EN      = "\x43\x68\x61\x6E\x6E\x65\x6C\x20\x64\x61\x74\x61\x62\x61\x73\x65\x20\x69\x73\x20\x64\x61\x6D\x61\x67\x65\x64\x20\x2D\x20\x6E\x6F\x74\x68\x69\x6E\x67\x20\x77\x61\x73\x20\x63\x68\x61\x6E\x67\x65\x64";
/* Channel database is damaged - nothing was changed */

/* Backup paths */
#define CC_BACKUP_DIR         "/data/.ColorPro_d/backup"
#define CC_BACKUP_SERVICE_DB  CC_BACKUP_DIR "/service.db.orig"
#define CC_BACKUP_ORCA_KEYS   CC_BACKUP_DIR "/keys.bin.orig"
#define CC_BACKUP_META        CC_BACKUP_DIR "/backup_meta.txt"
#define CC_BACKUP_RESTORE_SH  CC_BACKUP_DIR "/restore.sh"

/* Backup Arabic messages */
static const char* M_BACKUP_REMOVE_BTN = "\xd8\xa5\xd8\xb2\xd8\xa7\xd9\x84\xd8\xa9\x20\xd8\xa7\xd9\x84\xd8\xa5\xd8\xb6\xd8\xa7\xd9\x81\xd8\xa9\x20\xd9\x88\xd8\xa7\xd8\xb3\xd8\xaa\xd8\xb9\xd8\xa7\xd8\xaf\xd8\xa9\x20\xd8\xa7\xd9\x84\xd9\x82\xd9\x86\xd9\x88\xd8\xa7\xd8\xaa"; /* إزالة الإضافة واستعادة القنوات */
static const char* M_BACKUP_CONFIRM_REMOVE = "\xd8\xb3\xd9\x8a\xd8\xaa\xd9\x85\x20\xd8\xad\xd8\xb0\xd9\x81\x20\xd8\xa7\xd9\x84\xd8\xa5\xd8\xb6\xd8\xa7\xd9\x81\xd8\xa9\x20\xd9\x88\xd8\xa7\xd8\xb3\xd8\xaa\xd8\xb9\xd8\xa7\xd8\xaf\xd8\xa9\x20\xd9\x82\xd9\x86\xd9\x88\xd8\xa7\xd8\xaa\xd9\x83\x20\xd8\xa7\xd9\x84\xd8\xa3\xd8\xb5\xd9\x84\xd9\x8a\xd8\xa9\x2e\x20\xd9\x85\xd8\xaa\xd8\xa7\xd8\xa8\xd8\xb9\xd8\xa9\xd8\x9f"; /* سيتم حذف الإضافة واستعادة قنواتك الأصلية. متابعة؟ */
static const char* M_BACKUP_UPDATE_BTN = "\xd8\xaa\xd8\xad\xd8\xaf\xd9\x8a\xd8\xab\x20\xd8\xa7\xd9\x84\xd9\x86\xd8\xb3\xd8\xae\xd8\xa9\x20\xd8\xa7\xd9\x84\xd8\xa7\xd8\xad\xd8\xaa\xd9\x8a\xd8\xa7\xd8\xb7\xd9\x8a\xd8\xa9\x20\xd9\x84\xd9\x82\xd9\x86\xd9\x88\xd8\xa7\xd8\xaa\xd9\x8a\x20\xd8\xa7\xd9\x84\xd8\xad\xd8\xa7\xd9\x84\xd9\x8a\xd8\xa9"; /* تحديث النسخة الاحتياطية لقنواتي الحالية */
static const char* M_BACKUP_CREATED_NOTIF = "\xd8\xaa\xd9\x85\x20\xd8\xa5\xd9\x86\xd8\xb4\xd8\xa7\xd8\xa1\x20\xd9\x86\xd8\xb3\xd8\xae\xd8\xa9\x20\xd8\xa7\xd8\xad\xd8\xaa\xd9\x8a\xd8\xa7\xd8\xb7\xd9\x8a\xd8\xa9\x20\xd9\x84\xd9\x82\xd9\x86\xd9\x88\xd8\xa7\xd8\xaa\xd9\x83\x20\xd9\x81\xd9\x8a\x20\x2f\x64\x61\x74\x61\x2f\x2e\x73\x79\x73\x75\x70\x64\x5f\x64\x2f\x62\x61\x63\x6b\x75\x70\x2f"; /* تم إنشاء نسخة احتياطية لقنواتك في /data/.ColorPro_d/backup/ */
static const char* M_BACKUP_RESTORING_NOTIF = "\xd8\xac\xd8\xa7\xd8\xb1\xd9\x8a\x20\xd8\xa7\xd9\x84\xd8\xa7\xd8\xb3\xd8\xaa\xd8\xb1\xd8\xac\xd8\xa7\xd8\xb9\x2e\x2e\x2e\x20\xd8\xb3\xd9\x8a\xd8\xb9\xd9\x8a\xd8\xaf\x20\xd8\xa7\xd9\x84\xd8\xac\xd9\x87\xd8\xa7\xd8\xb2\x20\xd8\xa7\xd9\x84\xd8\xaa\xd8\xb4\xd8\xba\xd9\x8a\xd9\x84"; /* جاري الاسترجاع... سيعيد الجهاز التشغيل */
static const char* M_BACKUP_CONFIRM_HINT = "\xd8\xa7\xd8\xb6\xd8\xba\xd8\xb7\x20\x4f\x4b\x20\xd9\x84\xd9\x84\xd8\xaa\xd8\xa3\xd9\x83\xd9\x8a\xd8\xaf\x20\xd8\xa3\xd9\x88\x20\x45\x58\x49\x54\x20\xd9\x84\xd9\x84\xd8\xa5\xd9\x84\xd8\xba\xd8\xa7\xd8\xa1"; /* اضغط OK للتأكيد أو EXIT للإلغاء */
static const char* M_BACKUP_UPDATED_OK = "\xd8\xaa\xd9\x85\x20\xd8\xaa\xd8\xad\xd8\xaf\xd9\x8a\xd8\xab\x20\xd8\xa7\xd9\x84\xd9\x86\xd8\xb3\xd8\xae\xd8\xa9\x20\xd8\xa7\xd9\x84\xd8\xa7\xd8\xad\xd8\xaa\xd9\x8a\xd8\xa7\xd8\xb7\xd9\x8a\xd8\xa9\x20\xd8\xa8\xd9\x86\xd8\xac\xd8\xa7\xd8\xad"; /* تم تحديث النسخة الاحتياطية بنجاح */
static const char* M_BACKUP_RESTORE_ERR = "\xd9\x81\xd8\xb4\xd9\x84\x20\xd8\xa7\xd9\x84\xd8\xa7\xd8\xb3\xd8\xaa\xd8\xb1\xd8\xac\xd8\xa7\xd8\xb9\x3a\x20\xd8\xa7\xd9\x84\xd9\x86\xd8\xb3\xd8\xae\xd8\xa9\x20\xd8\xa7\xd9\x84\xd8\xa7\xd8\xad\xd8\xaa\xd9\x8a\xd8\xa7\xd8\xb7\xd9\x8a\xd8\xa9\x20\xd8\xba\xd9\x8a\xd8\xb1\x20\xd9\x85\xd9\x88\xd8\xac\xd9\x88\xd8\xaf\xd8\xa9"; /* فشل الاسترجاع: النسخة الاحتياطية غير موجودة */

/* Key code definitions */
#ifndef KEY_EXIT
#define KEY_EXIT 0x16a
#endif
#ifndef KEY_7
#define KEY_7 8
#endif
#ifndef KEY_8
#define KEY_8 9
#endif
#ifndef KEY_9
#define KEY_9 10
#endif
#ifndef KEY_OK
#define KEY_OK 352
#endif
#ifndef KEY_ENTER
#define KEY_ENTER 28
#endif

/* ---------- Globals ---------- */
static volatile sig_atomic_t g_running = 1;
static int g_fd = -1;
static int g_overlayPid = -1;
static bool g_grabbed = false;
static FILE* g_dbgFile = nullptr;
static char g_devPath[64] = DEV;       /* discovered IR receiver, DEV = fallback */
static char g_devName[128] = "?";      /* EVIOCGNAME of g_devPath                */
static char g_profileName[32] = "UNKNOWN";   /* DEV_PROFILES[].name of this box  */

/* ---------- Beeper state (SNR monitor) ---------- */
static bool g_beepMute      = false;      /* user toggled mute            */
static long long g_beepLastMs = 0;        /* last beep fire time          */
static int  g_beepQuality   = 0;          /* cached quality for interval  */

/* ---------- Device Detection ---------- */
enum DeviceType {
    DEV_UNKNOWN = 0,
    DEV_WEGOO   = 1,
    DEV_IRON    = 2
};
static DeviceType g_devType = DEV_UNKNOWN;
static char g_modelName[64] = "Unknown";
static long long g_dexSize = -1;
static long long g_artSize = -1;
static long long g_nativeDexSize = -1;
static uintptr_t g_tvSvcOff = 0xe9598;
static off_t     g_patchAOff = 0;
static off_t     g_patchBOff = 0;

static void dbgInit();
static void dbg(const char* fmt, ...);

/* Native Launcher image size for each known box. These were measured from a
 * freshly-booted, never-patched launcher, so they are the only trustworthy
 * reference. A colour file whose size differs is from the wrong box and will
 * corrupt the ART map -> SIGBUS -> "Loading Launcher" forever. */
struct DevProfile {
    const char* name;
    const char* modelKey;   /* matched against ro.product.model */
    long long  dexSize;     /* native classes.dex size, -1 = not measured yet  */
    long long  artSize;     /* native classes.art size, -1 = not measured yet  */
    uintptr_t  tvSvcOff;    /* offset of _tvSvc in /system/bin/f_server        */
    off_t      patchAOff;   /* offset of color byte in getView                 */
    off_t      patchBOff;   /* offset of color byte in refreshListEnableCheck  */
};

/* Same model, different firmware revision -> the dex/art that shipped with it
 * is a few KB/tens of KB apart.  A deviation inside this window is treated as
 * "another revision", anything beyond it still means tampering. */
#define ART_SIZE_SLACK  (64 * 1024)

/* Order matters: modelKey is matched with strstr(), so the longest key of a
 * family must come first or "IRON" would swallow "IRON_PRO" / "IRON_PLUS". */
static const DevProfile DEV_PROFILES[] = {
    { "WEGOO",     "WEGOO",     22471080LL,   -1,  (off_t)0x5c34e42c, (off_t)0x5cd3a5ae, (off_t)0x5cd39658 },
    { "IRON_PRO",  "IRON_PRO",  22524328LL,   -1,  (off_t)0x5c350444, (off_t)0x5d30e8be, (off_t)0x5d30dd68 },
    { "IRON_PLUS", "IRON_PLUS", 22524328LL,   -1,  (off_t)0x5c350444, (off_t)0x5d30e8be, (off_t)0x5d30dd68 },
    { "IRON",      "IRON",      22524328LL,   -1,  (off_t)0x5c350444, (off_t)0x5d30e8be, (off_t)0x5d30dd68 },
};
static const int DEV_PROFILE_COUNT = 4;

static long long fileSizeOf(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long long)st.st_size;
}

/* Detection: ro.product.model is the ONLY reliable discriminator (both boxes
 * share ro.product.device=Hi3798MV200 and ro.board.platform=bigfish).
 * Size mismatches are logged but do NOT block detection — a wrong-size dex is
 * exactly the contamination we are here to repair. The hard size check lives
 * in gate 2 (source file) and gate 3 (post-write verify). */
static void detectDevice() {
    FILE* fp = popen("getprop ro.product.model", "r");
    if (fp) {
        if (fgets(g_modelName, sizeof(g_modelName) - 1, fp)) {
            for (int i = 0; g_modelName[i]; i++) {
                if (g_modelName[i] == '\r' || g_modelName[i] == '\n') g_modelName[i] = '\0';
            }
        }
        pclose(fp);
    }

    long long dex = fileSizeOf(DALVIK_TARGET);
    long long art = fileSizeOf(ART_PATH);
    g_dexSize = dex;
    g_artSize = art;

    const DevProfile* match = nullptr;
    /* Match against a normalised copy so "Icone Iron Pro", "IRON PRO" and
     * "iron_pro" all reach the same profile - the raw string stays untouched
     * in g_modelName because that is what the report and the log show. */
    char modelKey[64];
    snprintf(modelKey, sizeof(modelKey), "%s", g_modelName);
    for (int i = 0; modelKey[i]; i++) {
        unsigned char c = (unsigned char)modelKey[i];
        if (c == ' ' || c == '-') modelKey[i] = '_';
        else modelKey[i] = (char)toupper(c);
    }
    for (int i = (int)strlen(modelKey) - 1; i >= 0 && modelKey[i] == '_'; --i)
        modelKey[i] = '\0';

    for (int i = 0; i < DEV_PROFILE_COUNT; ++i) {
        if (strstr(modelKey, DEV_PROFILES[i].modelKey) != nullptr) { match = &DEV_PROFILES[i]; break; }
    }
    if (!match) {
        dbg("[device] model='%s' (key '%s') matches no known profile -> UNKNOWN",
            g_modelName, modelKey);
        g_devType = DEV_UNKNOWN;
        return;
    }

    snprintf(g_profileName, sizeof(g_profileName), "%s", match->name);
    g_devType = (strncmp(match->name, "IRON", 4) == 0) ? DEV_IRON : DEV_WEGOO;
    /* -1 = "no reference measured for this profile yet": the apply path then
     * falls back to the size found on this box instead of refusing every file. */
    g_nativeDexSize = (match->dexSize > 0) ? match->dexSize : -1;
    g_tvSvcOff = decOff((uint32_t)match->tvSvcOff);
    g_patchAOff = decOff((uint32_t)match->patchAOff);
    g_patchBOff = decOff((uint32_t)match->patchBOff);

    /* art is never written by this plugin, so a big deviation means something
     * else tampered with it -> refuse.  A deviation inside ART_SIZE_SLACK is
     * another firmware revision of the same model, and an unknown reference
     * (-1) only means this profile still has to be measured: both are logged
     * and detection continues. */
    if (match->artSize > 0) {
        long long diff = art - match->artSize;
        if (diff < 0) diff = -diff;
        if (art < 0 || diff > ART_SIZE_SLACK) {
            dbg("[device] REFUSED: art=%lld but profile %s expects %lld (slack %d)",
                art, match->name, match->artSize, ART_SIZE_SLACK);
            g_devType = DEV_UNKNOWN;
            return;
        }
        if (diff != 0)
            dbg("[device] art=%lld != profile %lld but within slack -> firmware revision, accepted",
                art, match->artSize);
    } else {
        dbg("[device] profile %s has no art reference yet (art=%lld) -> art size check skipped",
            match->name, art);
    }

    if (g_nativeDexSize < 0) {
        dbg("[device] model='%s' profile=%s CONFIRMED (no dex reference yet - apply will use the size found on this box, %lld)",
            g_modelName, match->name, dex);
    } else if (dex != match->dexSize) {
        dbg("[device] model='%s' profile=%s CONFIRMED (dex=%lld != native %lld — contaminated, will repair on apply)",
            g_modelName, match->name, dex, match->dexSize);
    } else {
        dbg("[device] confirmed: model='%s' profile=%s dex=%lld art=%lld",
            g_modelName, match->name, dex, art);
    }
}

/* ---------- Debug logging (persistent handle) ---------- */
static void dbgInit() {
    if (!g_dbgFile) {
        g_dbgFile = fopen(DBG_FILE, "a");
        if (g_dbgFile) setvbuf(g_dbgFile, nullptr, _IOLBF, 0);
    }
}

static void dbg(const char* fmt, ...) {
    if (!g_dbgFile) { dbgInit(); if (!g_dbgFile) return; }
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    fprintf(g_dbgFile, "[%06ld.%03ld] ",
            (long)(tv.tv_sec % 1000000), (long)(tv.tv_usec / 1000));
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_dbgFile, fmt, ap);
    va_end(ap);
    fputc('\n', g_dbgFile);
}

/* ---------- Signal handlers ---------- */
static void sigTermHandler(int) { g_running = 0; }
static void sigPipeHandler(int) { /* ignore */ }
static void sigChldHandler(int) {
    /* waitpid() overwrites errno; a handler must leave it exactly as it found
     * it or the interrupted poll()/read()/open() in the main loop and the
     * stop() path would report a bogus error code. */
    int saved = errno;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}
    errno = saved;
}

/* ---------- Key classification ---------- */
#ifndef KEY_RED
#define KEY_RED 398
#endif

static inline bool isRedKey(unsigned code) {
    /* 59 = KEY_F1 (Red button on Icone), 398 = KEY_RED, 207/126 = KEY_PLAY */
    return code == 59 || code == 398 || code == KEY_RED || code == KEY_PLAY || code == 207 || code == 126;
}

static inline int64_t nowMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

static inline bool isExitKey(unsigned code) {
    return code == KEY_EXIT || code == KEY_ESC || code == KEY_BACK ||
           code == 249 || code == 223;
}

static inline bool isOkKey(unsigned code) {
    return code == KEY_OK || code == KEY_ENTER || code == 28 || code == 352 || code == KEY_1;
}

static bool setAndroidProp(const char* key, const char* value) {
    if (__system_property_set(key, value) == 0) return true;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "/system/bin/setprop %s %s 2>/dev/null", key, value);
    return system(cmd) == 0;
}

/* ---------- File descriptor cleanup helpers ---------- */
static void closeAllFdsAbove(int minFd = 3, int keep1 = -1, int keep2 = -1, int keep3 = -1) {
    DIR* d = opendir("/proc/self/fd");
    if (!d) {
        int maxFd = (int)sysconf(_SC_OPEN_MAX);
        if (maxFd < 0 || maxFd > 4096) maxFd = 1024;
        for (int fd = minFd; fd < maxFd; ++fd) {
            if (fd != keep1 && fd != keep2 && fd != keep3) close(fd);
        }
        return;
    }
    int dfd = dirfd(d);
    std::vector<int> toClose;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_name[0] == '.') continue;
        int fd = atoi(de->d_name);
        if (fd >= minFd && fd != dfd && fd != keep1 && fd != keep2 && fd != keep3) {
            toClose.push_back(fd);
        }
    }
    closedir(d);
    for (int fd : toClose) {
        close(fd);
    }
}

static void closeInheritedFds(int keepFd = -1) {
    DIR* d = opendir("/proc/self/fd");
    if (!d) return;
    int dfd = dirfd(d);
    struct ClosedItem {
        int fd;
        std::string target;
        bool isHi;
    };
    std::vector<ClosedItem> list;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_name[0] == '.') continue;
        int fd = atoi(de->d_name);
        if (fd < 3 || fd == dfd || fd == keepFd) continue;

        char path[64];
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
        char tgt[256];
        ssize_t n = readlink(path, tgt, sizeof(tgt) - 1);
        if (n > 0) tgt[n] = '\0';
        else snprintf(tgt, sizeof(tgt), "(unknown)");

        bool isHi = (strncmp(tgt, "/dev/hi_", 8) == 0 ||
                     strncmp(tgt, "/dev/mmz", 8) == 0 ||
                     strncmp(tgt, "/dev/vinput", 11) == 0);
        list.push_back({ fd, std::string(tgt), isHi });
    }
    closedir(d);

    for (const auto& it : list) {
        close(it.fd);
    }

    if (!list.empty()) {
        dbgInit();
        for (const auto& it : list) {
            dbg("[init] closed inherited fd %d -> '%s'%s",
                it.fd, it.target.c_str(), it.isHi ? " [HI DEVICE RELEASED]" : "");
        }
    }
}

/* ---------- File I/O helpers ---------- */
static bool writeFile(const char* path, const std::string& content, bool fsyncNow = false) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) return false;
    bool ok = true;
    if (!content.empty()) {
        ssize_t n = write(fd, content.data(), content.size());
        ok = (n == (ssize_t)content.size());
    }
    if (fsyncNow) fsync(fd);
    close(fd);
    return ok;
}

static std::string readFile(const char* path, size_t maxLen = 256) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return std::string();
    char buf[1024];
    ssize_t n = read(fd, buf, std::min((size_t)sizeof(buf) - 1, maxLen));
    close(fd);
    if (n <= 0) return std::string();
    buf[n] = '\0';
    std::string s(buf, (size_t)n);
    size_t cr = s.find_first_of("\r\n");
    if (cr != std::string::npos) s.erase(cr);
    return s;
}

/* Whole file, newlines kept (readFile above stops at the first line). */
static std::string readFileAll(const char* path, size_t maxLen = 4096) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return std::string();
    std::string out;
    char buf[512];
    while (out.size() < maxLen) {
        size_t want = sizeof(buf);
        if (out.size() + want > maxLen) want = maxLen - out.size();
        ssize_t n = read(fd, buf, want);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        out.append(buf, (size_t)n);
    }
    close(fd);
    return out;
}

/* Plain copy with fsync - no shell, so no waitpid()/sigChldHandler races. */
static bool copyFileTo(const char* src, const char* dst) {
    int in = open(src, O_RDONLY | O_CLOEXEC);
    if (in < 0) return false;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (out < 0) { close(in); return false; }
    bool ok = true;
    char buf[32768];
    while (ok) {
        ssize_t n = read(in, buf, sizeof(buf));
        if (n < 0) { if (errno == EINTR) continue; ok = false; break; }
        if (n == 0) break;
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if (w < 0) { if (errno == EINTR) continue; ok = false; break; }
            off += w;
        }
    }
    if (ok) fsync(out);
    close(out);
    close(in);
    if (!ok) unlink(dst);
    return ok;
}

/* ---------- Update Lock helpers ---------- */
#define UPDATE_LOCK_TIMEOUT_SEC 600

static bool isUpdateLocked() {
    if (access(UPDATE_LOCK_FILE, F_OK) != 0) return false;

    std::string content = readFile(UPDATE_LOCK_FILE, 256);
    int lockPid = -1;
    time_t lockTime = 0;

    size_t ppos = content.find("pid=");
    if (ppos != std::string::npos) {
        lockPid = atoi(content.c_str() + ppos + 4);
    }
    size_t tpos = content.find("time=");
    if (tpos != std::string::npos) {
        lockTime = (time_t)strtoull(content.c_str() + tpos + 5, nullptr, 10);
    }

    time_t now = time(nullptr);

    // Stale check 1: age > 10 minutes (600 seconds)
    if (lockTime > 0 && now >= lockTime && (now - lockTime) > UPDATE_LOCK_TIMEOUT_SEC) {
        dbg("[up] stale lock detected (age=%ld s > %d s), clearing lock",
            (long)(now - lockTime), UPDATE_LOCK_TIMEOUT_SEC);
        unlink(UPDATE_LOCK_FILE);
        return false;
    }

    // Stale check 2: holder PID is dead
    if (lockPid > 0 && kill((pid_t)lockPid, 0) != 0 && errno == ESRCH) {
        dbg("[up] stale lock detected (pid=%d is dead), clearing lock", lockPid);
        unlink(UPDATE_LOCK_FILE);
        return false;
    }

    // Corrupt / empty lock file
    if (lockPid <= 0 && lockTime <= 0) {
        dbg("[up] corrupt lock file detected, clearing lock");
        unlink(UPDATE_LOCK_FILE);
        return false;
    }

    return true;
}

static bool acquireUpdateLock() {
    if (isUpdateLocked()) return false;
    char buf[128];
    snprintf(buf, sizeof(buf), "pid=%d\ntime=%ld\n", (int)getpid(), (long)time(nullptr));
    return writeFile(UPDATE_LOCK_FILE, buf, true);
}

static void releaseUpdateLock() {
    unlink(UPDATE_LOCK_FILE);
}

struct ScopedUpdateLock {
    bool acquired;
    ScopedUpdateLock() : acquired(false) {}
    bool acquire() {
        acquired = acquireUpdateLock();
        return acquired;
    }
    void release() {
        if (acquired) {
            releaseUpdateLock();
            acquired = false;
        }
    }
    ~ScopedUpdateLock() {
        release();
    }
};

/* ---------- Overlay launcher (fork + execvp, no shell) ---------- */
static void launchOverlay(const char* path, int seconds) {
    pid_t pid = fork();
    if (pid < 0) {
        dbg("[overlay] fork failed errno=%d", errno);
        return;
    }
    if (pid == 0) {
        char secStr[16];
        snprintf(secStr, sizeof(secStr), "%d", seconds);

        const char* actualJar = HUD_JAR;
        if (access(HUD_JAR_ALT, F_OK) == 0) actualJar = HUD_JAR_ALT;
        else if (access(HUD_JAR, F_OK) == 0) actualJar = HUD_JAR;
        else if (access(HUD_JAR_DATA, F_OK) == 0) actualJar = HUD_JAR_DATA;

        char cpArg[256];
        snprintf(cpArg, sizeof(cpArg), "-Djava.class.path=%s", actualJar);

        char* const argv[] = {
            (char*)"/system/bin/app_process",
            cpArg,
            (char*)"/system/bin",
            (char*)"lab.s1.OverlayHud",
            (char*)path,
            secStr,
            nullptr
        };

        setenv("CLASSPATH", actualJar, 1);

        int logfd = open("/data/local/tmp/overlay_child.log", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
        if (logfd >= 0) {
            dup2(logfd, STDOUT_FILENO);
            dup2(logfd, STDERR_FILENO);
            close(logfd);
        }
        closeAllFdsAbove(3);
        execv("/system/bin/app_process", argv);
        _exit(127);
    }
    g_overlayPid = (int)pid;
}

/* Kill every overlay watcher that is not ours.
 * Each instance keeps exactly one JVM watching HUD_TXT.  When a second
 * instance starts while the first is still running (seen during deploys: the
 * loser is then SIGKILLed by stopAllInstances) the loser's watcher is
 * orphaned and keeps running, and two JVMs would both draw the same menu;
 * "stop" would leave one behind for the same reason - a stopped
 * daemon cannot take its own JVM with it.  Only our own class is matched, so
 * no other app_process use on the box is touched. */
static void killStaleWatchers() {
    for (int pass = 0; pass < 2; ++pass) {
        int sig = pass ? SIGKILL : SIGTERM;
        DIR* d = opendir("/proc");
        if (!d) return;
        struct dirent* de;
        int hits = 0;
        while ((de = readdir(d)) != nullptr) {
            if (de->d_type != DT_DIR) continue;
            if (!isdigit((unsigned char)de->d_name[0])) continue;
            pid_t p = (pid_t)atoi(de->d_name);
            if (p <= 1 || p == getpid()) continue;
            char cmdPath[64];
            snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", de->d_name);
            std::string cl = readFile(cmdPath, 512);
            if (cl.find("lab.s1.OverlayHud") == std::string::npos) continue;
            if (cl.find(HUD_TXT) == std::string::npos) continue;   /* not another plugin's overlay */
            if (kill(p, sig) == 0) {
                dbg("[overlay] stale watcher pid=%d sent SIG%s", (int)p, pass ? "KILL" : "TERM");
                ++hits;
            }
        }
        closedir(d);
        if (hits == 0) return;
        usleep(250000);
    }
}

/* Keep one JVM alive that watches HUD_TXT; writing the file updates the menu */
static void startOverlayWatcher() {
    killStaleWatchers();
    writeFile(HUD_TXT, " ", false);
    dbg("[overlay] starting persistent watcher");
    launchOverlay(HUD_TXT, 0);     /* 0 = live forever (no timeout exit) */
    dbg("[overlay] persistent watcher pid=%d", g_overlayPid);
    usleep(1500000);
    unlink(HUD_TXT);      /* hide window (jar hides when file missing) */
    dbg("[overlay] persistent watcher initial hide done");
}

/* ---------- Overlay watchdog ----------
 * The remote is grabbed (EVIOCGRAB 1) for as long as a menu is on screen.
 * If the overlay process has died in the meantime, nothing will ever draw or
 * dismiss anything again and every key stays swallowed -> the receiver looks
 * frozen.  So before grabbing we check the watcher is really alive (and that
 * it is not a zombie); when it is not, we restart it, and when even that
 * fails we must NOT grab - an ungrabbed remote can always be used by hand. */
static bool overlayAlive() {
    if (g_overlayPid <= 0) return false;
    if (kill(g_overlayPid, 0) == 0) {
        char st[64];
        snprintf(st, sizeof(st), "/proc/%d/stat", (int)g_overlayPid);
        std::string s = readFile(st, 256);
        size_t r = s.rfind(')');
        if (r != std::string::npos && r + 2 < s.size() && s[r + 2] == 'Z') {
            dbg("[overlay] watchdog: watcher pid=%d is a zombie", (int)g_overlayPid);
            return false;
        }
        return true;
    }
    dbg("[overlay] watchdog: watcher pid=%d is gone (errno=%d)", (int)g_overlayPid, errno);
    return false;
}

/* Grab the remote only while the watcher can answer for it.
 * g_grabbed remembers a grab we already hold: the menu screens call this on
 * every redraw, and grabbing an already-grabbed device returns EBUSY - which
 * used to fill the log with "EVIOCGRAB 1 failed" lines although nothing was
 * wrong.  A real EBUSY (some other client is holding the device) is now
 * reported as such and retried on the next redraw instead of being swallowed. */
static bool grabRemote() {
    if (g_fd < 0) return false;
    if (!overlayAlive()) {
        /* watcher dead: try to bring it back, then re-check */
        g_overlayPid = -1;
        startOverlayWatcher();
        if (g_overlayPid <= 0 || kill(g_overlayPid, 0) != 0) {
            /* safety net: never leave the input device locked without a live overlay */
            if (ioctl(g_fd, EVIOCGRAB, 0) == 0)
                dbg("[overlay] SAFETY: no watcher -> EVIOCGRAB 0 (remote left usable)");
            else
                dbg("[overlay] SAFETY: no watcher and EVIOCGRAB 0 failed errno=%d", errno);
            g_grabbed = false;
            return false;
        }
    }
    if (g_grabbed) return true;              /* menu -> channels: still holding */
    if (ioctl(g_fd, EVIOCGRAB, 1) == 0) {
        g_grabbed = true;
    } else if (errno == EBUSY) {
        dbg("[overlay] EVIOCGRAB 1: device busy (another client is holding it)");
    } else {
        dbg("[overlay] EVIOCGRAB 1 failed errno=%d", errno);
    }
    return true;
}

/* ---------- UTF-8 strings (Arabic + emoji, byte-exact) ---------- */
static const char* T_CURRENT_DEF = "\xD8\xBA\xD9\x8A\xD8\xB1\x20\xD9\x85\xD8\xAD\xD8\xAF\xD8\xAF";

static const char* M_TITLE    = "\xD8\xA7\xD8\xAE\xD8\xAA\xD9\x8A\xD8\xA7\xD8\xB1\x20\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\x20\xD8\xA7\xD9\x84\xD9\x85\xD9\x83\xD8\xB3\xD9\x88\xD8\xB1\xD8\xA9";
static const char* M_L1       = "[1] \xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xB5\xD9\x81\xD8\xB1\x20\xD8\xA7\xD9\x84\xD8\xB0\xD9\x87\xD8\xA8\xD9\x8A\x20(\x47\x6F\x6C\x64\x20\xF0\x9F\x9F\xA1\x29";
static const char* M_L2       = "[2] \xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xAE\xD8\xB6\xD8\xB1\x20\xD8\xA7\xD9\x84\xD9\x86\xD9\x8A\xD9\x88\xD9\x86\x20(\x4E\x65\x6F\x6E\x20\x47\x72\x65\x65\x6E\x20\xF0\x9F\x9F\xA2\x29";
static const char* M_L3       = "[3] \xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xAD\xD9\x85\xD8\xB1\x20\xD8\xA7\xD9\x84\xD9\x85\xD9\x84\xD9\x83\xD9\x8A\x20(\x56\x69\x76\x69\x64\x20\x52\x65\x64\x20\xF0\x9F\x94\xB4\x29";
static const char* M_L4       = "[4] \xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xB2\xD8\xB1\xD9\x82\x20\xD8\xA7\xD9\x84\xD8\xB3\xD9\x85\xD8\xA7\xD9\x88\xD9\x8A\x20(\x43\x79\x61\x6E\x20\x42\x6C\x75\x65\x20\xF0\x9F\x94\xB5\x29";
static const char* M_L5       = "[5] \xD8\xA7\xD9\x84\xD9\x88\xD8\xB6\xD8\xB9\x20\xD8\xA7\xD9\x84\xD8\xA7\xD9\x81\xD8\xAA\xD8\xB1\xD8\xA7\xD8\xB6\xD9\x8A\x20\xD9\x84\xD9\x84\xD9\x85\xD8\xB5\xD9\x86\xD8\xB9\x20(\x44\x65\x66\x61\x75\x6C\x74\x20\xE2\x9A\xAA\x29";
static const char* M_CURR_LBL = "\xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xAD\xD8\xA7\xD9\x84\xD9\x8A\x3A\x20";
static const char* M_HINT1    = "\xD8\xA7\xD8\xB6\xD8\xBA\xD8\xB7\x20\xD8\xB1\xD9\x82\xD9\x85\x20\xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA8\xD8\xA7\xD9\x84\xD8\xB1\xD9\x8A\xD9\x85\xD9\x88\xD8\xAA\x20(\x31\x20\xD8\xA5\xD9\x84\xD9\x89\x20\x35\x29";
static const char* M_HINT2    = "\xD8\xA7\xD8\xB6\xD8\xBA\xD8\xB7\x20\xD8\xA7\xD9\x84\xD8\xB2\xD8\xB1\x20\x45\x58\x49\x54\x20\xD9\x84\xD9\x84\xD8\xA5\xD9\x84\xD8\xBA\xD8\xA7\xD8\xA1";

static const char* S_START    = "\xE2\x8F\xB3\x20";
static const char* S_START2   = "\x20\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xAA\xD8\xB7\xD8\xA8\xD9\x8A\xD9\x82\x20";
static const char* S_OK       = "\xE2\x9C\x85\x20\xD8\xAA\xD9\x85\x20\xD8\xAA\xD9\x81\xD8\xB9\xD9\x8A\xD9\x84\x20";
static const char* S_OK2      = "\x20\xD8\xA8\xD9\x86\xD8\xAC\xD8\xA7\xD8\xAD\x21";
static const char* S_ERR      = "\xE2\x9D\x8C\x20\xD8\xAE\xD8\xB7\xD8\xA3\x3A\x20\xD9\x85\xD9\x84\xD9\x81\x20";
static const char* S_ERR2     = "\x20\xD8\xBA\xD9\x8A\xD8\xB1\x20\xD9\x85\xD9\x88\xD8\xAC\xD9\x88\xD8\xAF\x21";
static const char* S_DOT      = "...";

/* Device-confirmation screen + refusal messages */
static const char* M_CONFIRM_TITLE = "\xD8\xAA\xD8\xA3\xD9\x83\xD9\x8A\xD8\xAF \xD8\xA7\xD9\x84\xD8\xAC\xD9\x87\xD8\xA7\xD8\xB2";
static const char* M_CONFIRM_D1   = "\xD8\xA7\xD9\x84\xD8\xAC\xD9\x87\xD8\xA7\xD8\xB2\x3A";                                  /* الجهاز: */
static const char* M_CONFIRM_D2   = "\xD8\xAD\xD8\xAC\xD9\x85\x20\xD8\xA7\xD9\x84\xD9\x85\xD9\x84\xD9\x81\x3A";                /* حجم الملف: */
static const char* M_CONFIRM_H1   = "\xD8\xA7\xD8\xB6\xD8\xBA\xD8\xB7\x20\x5B\x31\x5D\x20\xD9\x84\xD9\x84\xD8\xAA\xD8\xA3\xD9\x83\xD9\x8A\xD8\xAF";  /* اضغط [1] للتأكيد */
static const char* M_CONFIRM_H2   = "\x45\x58\x49\x54\x20\x3D\x20\xD8\xA5\xD9\x84\xD8\xBA\xD8\xA7\xD8\xA1";                     /* EXIT = إلغاء */
static const char* M_REFUSE      = "\xD9\x84\xD9\x85 \xD9\x8A\xD8\xAA\xD8\xB9\xD8\xB1\xD9\x81 \xD8\xA7\xD9\x84\xD8\xAC\xD9\x87\xD8\xA7\xD8\xB2!";
static const char* M_WRONGDEV    = "\xD9\x85\xD9\x84\xD9\x81 \xD9\x87\xD8\xB0\xD8\xA7 \xD9\x84\xD8\xAC\xD9\x87\xD8\xA7\xD8\xB2 \xD8\xA2\xD8\xAE\xD8\xB1!";
static const char* M_POSTFAIL    = "\xD9\x81\xD8\xB4\xD9\x84 \xD8\xA7\xD9\x84\xD8\xAA\xD8\xB7\xD8\xA8\xD9\x8A\xD9\x82\x21";      /* فشل التطبيق! */
static const char* M_CANCELLED   = "\xD8\xA7\xD9\x84\xD8\xAA\xD8\xB7\xD8\xA8\xD9\x8A\xD9\x82\x20\xD9\x85\xD9\x84\xD8\xBA\xD9\x8A"; /* التطبيق ملغي */

static const char* N_GOLD     = "\xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xB0\xD9\x87\xD8\xA8\xD9\x8A\x20(\x47\x6F\x6C\x64\xF0\x9F\x9F\xA1\x29";
static const char* N_GREEN    = "\xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xAE\xD8\xB6\xD8\xB1\x20(\x47\x72\x65\x65\x6E\xF0\x9F\x9F\xA2\x29";
static const char* N_RED      = "\xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xAD\xD9\x85\xD8\xB1\x20(\x52\x65\x64\xF0\x9F\x94\xB4\x29";
static const char* N_CYAN     = "\xD8\xA7\xD9\x84\xD9\x84\xD9\x88\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xB3\xD9\x85\xD8\xA7\xD9\x88\xD9\x8A\x20(\x43\x79\x61\x6E\xF0\x9F\x94\xB5\x29";
static const char* N_DEFAULT  = "\xD8\xA7\xD9\x84\xD9\x88\xD8\xB6\xD8\xB9\x20\xD8\xA7\xD9\x84\xD8\xA7\xD9\x81\xD8\xAA\xD8\xB1\xD8\xA7\xD8\xB6\xD9\x8A\x20\xD9\x84\xD9\x84\xD9\x85\xD8\xB5\xD9\x86\xD8\xB9\x20(\x44\x65\x66\x61\x75\x6C\x74\xE2\x9A\xAA\x29";

/* ---------- "نوع القنوات" screen strings ---------- */
static const char* M_CH_ENTRY    = "[6] \xD9\x86\xD9\x88\xD8\xB9\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA";
static const char* M_CHCURR_LBL  = "\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\x20\xD8\xA7\xD9\x84\xD8\xAD\xD8\xA7\xD9\x84\xD9\x8A\xD8\xA9\x3A\x20";
static const char* M_CH_TITLE    = "\xD9\x86\xD9\x88\xD8\xB9\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA";
static const char* M_CH_L1       = "[1] \xD9\x83\xD9\x84\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA";
static const char* M_CH_L2       = "[2] \xD8\xA7\xD9\x84\xD9\x85\xD8\xAC\xD8\xA7\xD9\x86\xD9\x8A\xD8\xA9\x20\xD9\x81\xD9\x82\xD8\xB7";
static const char* M_CH_L3       = "[3] \xD8\xA7\xD9\x84\xD9\x85\xD8\xAC\xD8\xA7\xD9\x86\xD9\x8A\xD8\xA9\x20\x2B\x20\xD8\xA7\xD9\x84\xD9\x85\xD9\x81\xD8\xAA\xD9\x88\xD8\xAD\xD8\xA9";
static const char* M_CH_L4       = "[4] \xD8\xA7\xD9\x84\xD9\x85\xD9\x81\xD8\xAA\xD9\x88\xD8\xAD\xD8\xA9\x20\xD9\x81\xD9\x82\xD8\xB7";
static const char* M_CH_CURR     = "\xD8\xA7\xD9\x84\xD8\xAD\xD8\xA7\xD9\x84\xD9\x8A\x3A\x20";
static const char* M_CH_HINT     = "\xD8\xA7\xD8\xB6\xD8\xBA\xD8\xB7\x20\xD8\xA7\xD9\x84\xD8\xB2\xD8\xB1\x20\x45\x58\x49\x54\x20\xD9\x84\xD9\x84\xD8\xB1\xD8\xAC\xD9\x88\xD8\xB9";
static const char* M_CH_BUSY     = "\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xAA\xD8\xAC\xD9\x87\xD9\x8A\xD8\xB2\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA";
static const char* M_CH_FAIL     = "\xD8\xAA\xD8\xB9\xD8\xB0\xD8\xB1\x20\xD8\xAA\xD8\xA8\xD8\xAF\xD9\x8A\xD9\x84\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA";   /* تعذر تبديل القنوات */
static const char* M_CH_NEED_RESCAN = "\xD8\xAA\xD9\x86\xD8\xA8\xD9\x8A\xD9\x87\x3A\x20\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\x20\xD9\x85\xD8\xB3\xD8\xAD\x20\xD8\xA3\xD8\xAD\xD8\xAF\xD8\xAB\x20\xD8\xBA\xD9\x8A\xD8\xB1\x20\xD9\x85\xD8\xAD\xD9\x81\xD9\x88\xD8\xB8\xD8\xA9\x20\x2D\x20\xD9\x86\xD9\x81\xD9\x91\xD8\xB0\x20\x72\x65\x73\x63\x61\x6E\x20\xD8\xA3\xD9\x88\xD9\x84\xD8\xA7\xD9\x8B";
static const char* N_ALL         = "\xD9\x83\xD9\x84\x20\xD8\xA7\xD9\x84\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA";
static const char* N_FREE        = "\xD8\xA7\xD9\x84\xD9\x85\xD8\xAC\xD8\xA7\xD9\x86\xD9\x8A\xD8\xA9\x20\xD9\x81\xD9\x82\xD8\xB7";
static const char* N_FREE_OPEN   = "\xD8\xA7\xD9\x84\xD9\x85\xD8\xAC\xD8\xA7\xD9\x86\xD9\x8A\xD8\xA9\x20\x2B\x20\xD8\xA7\xD9\x84\xD9\x85\xD9\x81\xD8\xAA\xD9\x88\xD8\xAD\xD8\xA9";
static const char* N_ENC_OPEN    = "\xD8\xA7\xD9\x84\xD9\x85\xD9\x81\xD8\xAA\xD9\x88\xD8\xAD\xD8\xA9\x20\xD9\x81\xD9\x82\xD8\xB7";
static const char* N_ENC_CLOSED  = "\xD8\xA7\xD9\x84\xD9\x85\xD8\xB4\xD9\x81\xD8\xB1\xD8\xA9\x20\xD8\xA7\xD9\x84\xD9\x85\xD8\xBA\xD9\x84\xD9\x82\xD8\xA9";

/* ---------- HUD palette (jar renders text through Html.fromHtml) ---------- */
static const char* H_BOX   = "#FFFFFF";  /* box-drawing frame      */
static const char* H_DIV   = "#264A5E";  /* section dividers       */
static const char* M_DIV_LINE = "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80";
static const char* H_TITLE = "#37D6F0";  /* menu title: cyan light */
static const char* H_ITEM  = "#FFFFFF";  /* item label             */
static const char* H_HINT  = "#8E96A8";  /* english hint / label   */
static const char* H_SUB   = "#C9D1E0";  /* footer hints: light    */
static const char* H_VAL   = "#DCE1EA";  /* current choice text    */
static const char* H_BUSY  = "#FFC107";  /* "applying..."          */
static const char* H_OK    = "#5BD76A";  /* success                */
static const char* H_WARN  = "#FFA726";  /* warning                */
static const char* H_ERR   = "#FF5252";  /* error                  */

/* swatch dots: one real colored circle per choice (emoji not rendered here) */
static const char* H_SW_GOLD   = "#FFD700";
static const char* H_SW_GREEN  = "#4CDA64";
static const char* H_SW_RED    = "#FF5252";
static const char* H_SW_CYAN   = "#37D6F0";
static const char* H_SW_NORMAL = "#B9BEC9";
static const char* SW_DOT      = "\xE2\x97\x8F";   /* U+25CF black circle */

/* Update screen strings */
static const char* M_SNR_ENTRY   = "[8] \xD9\x85\xD8\xA4\xD8\xB4\xD8\xB1\x20\xD8\xA7\xD9\x84\xD8\xA5\xD8\xB4\xD8\xA7\xD8\xB1\xD8\xA9\x20\xD9\x88\x20\x53\x4E\x52\x20\x64\x42";
static const char* M_UPDATE_ENTRY       = "\x5B\x37\x5D\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD9\x82\xD9\x82\x20\xD9\x85\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB"; /* [7] التحقق من التحديث */
static const char* M_UPDATE_TITLE       = "\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD9\x82\xD9\x82\x20\xD9\x85\xD9\x86\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB"; /* التحقق من التحديث */
static const char* M_UPDATE_AVAIL       = "\xD9\x8A\xD8\xAA\xD9\x88\xD9\x81\xD8\xB1\x20\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20"; /* يتوفر تحديث  */
static const char* M_UPDATE_CHECKING    = "\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xA7\xD9\x84\xD9\x81\xD8\xAD\xD8\xB5\x2E\x2E\x2E"; /* جاري الفحص... */
static const char* M_UPDATE_LATEST      = "\xD8\xA3\xD9\x86\xD8\xAA\x20\xD8\xB9\xD9\x84\xD9\x89\x20\xD8\xA2\xD8\xAE\xD8\xB1\x20\xD8\xA5\xD8\xB5\xD8\xAF\xD8\xA7\xD8\xB1"; /* أنت على آخر إصدار */
static const char* M_UPDATE_FOUND       = "\xD9\x8A\xD8\xAA\xD9\x88\xD9\x81\xD8\xB1\x20\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD8\xAC\xD8\xAF\xD9\x8A\xD8\xAF\x21"; /* يتوفر تحديث جديد! */
static const char* M_UPDATE_NOW_BTN     = "\x5B\x31\x5D\x20\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD8\xA7\xD9\x84\xD8\xA2\xD9\x86"; /* [1] تحديث الآن */
static const char* M_UPDATE_ERR_NET     = "\xD8\xAA\xD8\xB9\xD8\xB0\xD8\xB1\x20\xD8\xA7\xD9\x84\xD8\xA7\xD8\xAA\xD8\xB5\xD8\xA7\xD9\x84\x20\xD8\xA8\xD8\xA7\xD9\x84\xD8\xAE\xD8\xA7\xD8\xAF\xD9\x85"; /* تعذر الاتصال بالخادم */
static const char* M_UPDATE_ERR_SIG     = "\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD8\xBA\xD9\x8A\xD8\xB1\x20\xD9\x85\xD9\x88\xD8\xAB\xD9\x88\xD9\x82\x20\x28\xD9\x81\xD8\xB4\xD9\x84\x20\xD8\xA7\xD9\x84\xD8\xAA\xD9\x88\xD9\x82\xD9\x8A\xD8\xB9\x29"; /* تحديث غير موثوق (فشل التوقيع) */
static const char* M_UPDATE_DISABLED    = "\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD9\x85\xD8\xAA\xD9\x88\xD9\x82\xD9\x81\x3A\x20\xD9\x84\xD8\xA7\x20\xD9\x8A\xD9\x88\xD8\xAC\xD8\xAF\x20\xD9\x85\xD9\x81\xD8\xAA\xD8\xA7\xD8\xAD\x20\xD9\x85\xD9\x87\xD9\x8A\xD8\xA3"; /* تحديث متوقف: لا يوجد مفتاح مهيأ */
static const char* M_UPDATE_CURR_VER    = "\xD8\xA7\xD9\x84\xD8\xA5\xD8\xB5\xD8\xAF\xD8\xA7\xD8\xB1\x20\xD8\xA7\xD9\x84\xD8\xAD\xD8\xA7\xD9\x84\xD9\x8A\x3A\x20"; /* الإصدار الحالي:  */
static const char* M_UPDATE_NEW_VER     = "\xD8\xA7\xD9\x84\xD8\xA5\xD8\xB5\xD8\xAF\xD8\xA7\xD8\xB1\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xAD\xD8\xAF\xD8\xAB\x3A\x20"; /* الإصدار الأحدث:  */
static const char* M_UPDATE_NOTES_LBL   = "\xD8\xA7\xD9\x84\xD9\x85\xD9\x84\xD8\xA7\xD8\xAD\xD8\xB8\xD8\xA7\xD8\xAA\x3A\x20"; /* الملاحظات:  */
static const char* M_UPDATE_HINT_EXIT   = "\xD8\xA7\xD8\xB6\xD8\xBA\xD8\xB7\x20\xD8\xA7\xD9\x84\xD8\xB2\xD8\xB1\x20\x45\x58\x49\x54\x20\xD9\x84\xD9\x84\xD8\xB1\xD8\xAC\xD9\x88\xD8\xB9"; /* اضغط الزر EXIT للرجوع */
static const char* M_UPDATE_DOWNLOADING = "\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xA7\xD9\x84\xD8\xAA\xD9\x86\xD8\xB2\xD9\x8A\xD9\x84\x2E\x2E\x2E"; /* جاري التنزيل... */
static const char* M_UPDATE_INSTALLING  = "\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAB\xD8\xA8\xD9\x8A\xD8\xAA\x2E\x2E\x2E"; /* جاري التثبيت... */
static const char* M_UPDATE_RESTARTING  = "\xD8\xAA\xD9\x85\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD9\x88\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xA5\xD8\xB9\xD8\xA7\xD8\xAF\xD8\xA9\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xB4\xD8\xBA\xD9\x8A\xD9\x84\x2E\x2E\x2E"; /* تم التحديث وجاري إعادة التشغيل... */
static const char* M_UPDATE_ERR_SHA     = "\xD9\x81\xD8\xB4\xD9\x84\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD9\x82\xD9\x82\x20\xD9\x85\xD9\x86\x20\xD8\xA8\xD8\xB5\xD9\x85\xD8\xA9\x20\xD8\xA7\xD9\x84\xD9\x85\xD9\x84\xD9\x81"; /* فشل التحقق من بصمة الملف */
static const char* M_UPDATE_ERR_INST    = "\xD9\x81\xD8\xB4\xD9\x84\x20\xD8\xAA\xD8\xAB\xD8\xA8\xD9\x8A\xD8\xAA\x20\xD8\xA7\xD9\x84\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB"; /* فشل تثبيت التحديث */

static std::string hcol(const char* color, const std::string& s) {
    std::string o;
    o.reserve(s.size() + 48);
    o += FONT_COLOR_OPEN;
    o += color;
    o += "\">";
    o += s;
    o += "</font>";
    return o;
}

static const char* colorHexOf(uint8_t colorByte) {
    switch (colorByte) {
        case 0xd9: return H_SW_GOLD;
        case 0xda: return H_SW_GREEN;
        case 0xd7: return H_SW_RED;
        case 0xcf: return H_SW_CYAN;
        case 0x2f: return "#FFFFFF";
        default:   return H_SW_GOLD;
    }
}

/* drop emoji the device font draws badly (4-byte ones + white circle) + tidy " )" */
static void stripEmoji(std::string& s) {
    static const char* WHITE_CIRCLE = "\xE2\x9A\xAA";   /* U+26AA */
    std::string o;
    o.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) { i += 4; continue; }
        if (i + 3 <= s.size() && s.compare(i, 3, WHITE_CIRCLE) == 0) { i += 3; continue; }
        o += (char)c;
        i++;
    }
    size_t p;
    while ((p = o.find(" )")) != std::string::npos) o.erase(p, 1);
    s = o;
}

/* "[1] <arabic label> (Gold ...)" -> [1] + swatch dot + white label + hint */
static std::string htmlItem(const char* line, const char* sw) {
    std::string s(line);
    stripEmoji(s);
    size_t br = s.find(']');
    std::string head = (br == std::string::npos) ? s : s.substr(0, br + 1);
    std::string rest = (br == std::string::npos) ? std::string() : s.substr(br + 1);
    while (!head.empty() && head[0] == ' ') head.erase(0, 1);
    while (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    size_t p = rest.rfind(" (");
    std::string hint;
    if (p != std::string::npos) {
        hint = rest.substr(p);
        rest.resize(p);
    }
    return hcol(H_ITEM, head) + " " + hcol(sw, std::string("<big>") + SW_DOT + "</big>") + " " + hcol(H_ITEM, rest) + hcol(H_HINT, hint);
}

static const char* swatchOf(const std::string& cur) {
    if (cur == N_GOLD)    return H_SW_GOLD;
    if (cur == N_GREEN)   return H_SW_GREEN;
    if (cur == N_RED)     return H_SW_RED;
    if (cur == N_CYAN)    return H_SW_CYAN;
    return H_SW_NORMAL;
}

/* ---------- Channel variants ----------
 * Order matters: the first CHANNEL_MENU_COUNT entries are exactly what the
 * sub-screen lists, mapped to KEY_1..KEY_4.  service_encrypted_orca_closed.db
 * is still built by buildVariants() but never shown (show = false).
 * delCond == nullptr means "no filtering": a plain copy of service_original.db.
 * The four conditions are the ones the old shell builder used, verbatim.
 */
struct ChannelVariant {
    std::string file;     /* name inside DATA_DIR                          */
    std::string label;    /* Arabic name, also what current_channels.txt holds */
    const char* delCond;  /* DELETE ... WHERE clause (nullptr = keep all)  */
    bool show;            /* listed on the sub-screen                      */
};
static const ChannelVariant VARIANTS[] = {
    { VARF_ORIGINAL, N_ALL,
      nullptr, true },
    { VARF_FREE, N_FREE,
      "svc_is_cas <> 0", true },
    { VARF_FREE_OPEN, N_FREE_OPEN,
      "svc_is_cas <> 0 AND NOT EXISTS (SELECT 1 FROM _o k WHERE _svcInfo.svc_id=k.svc_id AND _svcInfo.svc_tp_index=k.svc_tp_index)",
      true },
    { VARF_ENC_OPEN, N_ENC_OPEN,
      "svc_is_cas = 0 OR NOT EXISTS (SELECT 1 FROM _o k WHERE _svcInfo.svc_id=k.svc_id AND _svcInfo.svc_tp_index=k.svc_tp_index)",
      true },
    { VARF_ENC_CLOSED, N_ENC_CLOSED,
      "svc_is_cas = 0 OR EXISTS (SELECT 1 FROM _o k WHERE _svcInfo.svc_id=k.svc_id AND _svcInfo.svc_tp_index=k.svc_tp_index)",
      false },
};
static const int VARIANTS_N = (int)(sizeof(VARIANTS) / sizeof(VARIANTS[0]));
static const int CHANNEL_MENU_COUNT = 4;   /* KEY_1..KEY_4 on the sub-screen */

/* "service_free.db=1234" -> "1234"; empty string when the file is missing
 * (the menu then shows the option without a count - no sqlite3 is run here). */
static std::string variantCountOf(const char* file) {
    std::string all = readFileAll(VAR_COUNTS, 8192);
    if (all.empty()) return std::string();
    std::string key = std::string(file) + "=";
    size_t pos = 0;
    while ((pos = all.find(key, pos)) != std::string::npos) {
        if (pos == 0 || all[pos - 1] == '\n') {
            size_t s = pos + key.size();
            size_t e = all.find('\n', s);
            std::string v = all.substr(s, (e == std::string::npos) ? std::string::npos : e - s);
            while (!v.empty() && (v[v.size() - 1] == '\r' || v[v.size() - 1] == ' ')) v.erase(v.size() - 1);
            if (!v.empty()) return v;
        }
        pos += key.size();
    }
    return std::string();
}

/* ---------- Current choice ---------- */
static std::string readCurrent() {
    std::string s = readFile(CUR_CHOICE_FILE);
    return s.empty() ? std::string(T_CURRENT_DEF) : s;
}

/* name of the channel set that is believed to be live in /data/db/service.db */
static std::string readCurrentChannels() {
    std::string s = readFile(CUR_CHANNELS);
    return s.empty() ? std::string(N_ALL) : s;
}

/* ---------- Update State Helpers (local file, no network) ---------- */
struct UpdateStateInfo {
    time_t timestamp;
    std::string latestVersion;
    std::string notes;
    std::string status; // UP_TO_DATE, UPDATE_AVAILABLE, CHECK_FAILED, DISABLED
    UpdateStateInfo() : timestamp(0) {}
};

static bool saveUpdateState(time_t ts, int latestVer, const std::string& notes, const std::string& status) {
    /* update_state.txt is human readable -> "1.0", NOT the numeric 10000 */
    char buf[512];
    snprintf(buf, sizeof(buf), "timestamp=%ld\nlatest_version=%s\nnotes=%s\nstatus=%s\n",
             (long)ts, versionToString(latestVer).c_str(), notes.c_str(), status.c_str());
    return writeFile(UPDATE_STATE_FILE, buf, true);
}

static bool readUpdateState(UpdateStateInfo* out) {
    if (!out || access(UPDATE_STATE_FILE, F_OK) != 0) return false;
    std::string content = readFileAll(UPDATE_STATE_FILE, 1024);
    if (content.empty()) return false;

    out->timestamp = 0;
    out->latestVersion.clear();
    out->notes.clear();
    out->status.clear();

    size_t pos = 0;
    while (pos < content.size()) {
        size_t end = content.find('\n', pos);
        std::string line = content.substr(pos, (end == std::string::npos) ? std::string::npos : end - pos);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (!line.empty()) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);
                if (k == "timestamp") out->timestamp = (time_t)strtoull(v.c_str(), nullptr, 10);
                else if (k == "latest_version") out->latestVersion = v;
                else if (k == "notes") out->notes = v;
                else if (k == "status") out->status = v;
            }
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    return true;
}

/* ---------- Show menu ---------- */


/* ---------- Tinyalsa beeper (played in-process, non-blocking fork) ---------- */

#include <dlfcn.h>
#include <math.h>
#include <stdint.h>

struct BpPcmConfig {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    int format;
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    int avail_min;
};

static void playBeepAsync(int freq_hz, int duration_ms) {
    /* Fork so the beep does not block the HUD refresh loop */
    pid_t pid = fork();
    if (pid != 0) return; /* parent returns immediately */

    /* ---- child ---- */
    void* handle = dlopen("/system/lib/libtinyalsa.so", RTLD_NOW);
    if (!handle) _exit(1);

    typedef void* (*pcm_open_f)(unsigned, unsigned, unsigned, BpPcmConfig*);
    typedef int   (*pcm_write_f)(void*, const void*, unsigned);
    typedef int   (*pcm_close_f)(void*);
    typedef int   (*pcm_is_ready_f)(void*);

    auto fn_open  = (pcm_open_f)    dlsym(handle, "pcm_open");
    auto fn_write = (pcm_write_f)   dlsym(handle, "pcm_write");
    auto fn_close = (pcm_close_f)   dlsym(handle, "pcm_close");
    auto fn_ready = (pcm_is_ready_f)dlsym(handle, "pcm_is_ready");
    if (!fn_open || !fn_write || !fn_close || !fn_ready) { dlclose(handle); _exit(1); }

    BpPcmConfig cfg = {};
    cfg.channels     = 2;
    cfg.rate         = 48000; /* HiSilicon AIAO strictly expects 48000 Hz */
    cfg.period_size  = 512;
    cfg.period_count = 4;
    cfg.format       = 0; /* PCM_FORMAT_S16_LE */

    void* pcm = fn_open(0, 0, 0 /* PCM_OUT */, &cfg);
    if (!pcm || !fn_ready(pcm)) { if (pcm) fn_close(pcm); dlclose(handle); _exit(1); }

    int samples = (48000 * duration_ms) / 1000;
    int16_t* buf = (int16_t*)malloc(samples * 2 * sizeof(int16_t));
    if (!buf) { fn_close(pcm); dlclose(handle); _exit(1); }

    int ramp = (48000 * 5) / 1000; /* 5 ms fade in/out to avoid speaker pops */
    for (int i = 0; i < samples; ++i) {
        double t = (double)i / 48000.0;
        double s = sin(2.0 * M_PI * (double)freq_hz * t);
        double env = 1.0;
        if (i < ramp)             env = (double)i       / (double)ramp;
        else if (i > samples - ramp) env = (double)(samples - i) / (double)ramp;
        int16_t v = (int16_t)(s * env * 24000.0);
        buf[i*2] = v; buf[i*2+1] = v;
    }
    fn_write(pcm, buf, (unsigned)(samples * 2 * sizeof(int16_t)));
    free(buf);
    /* Allow hardware DMA to finish outputting all audio frames before close */
    usleep((duration_ms + 40) * 1000);
    fn_close(pcm);
    dlclose(handle);
    _exit(0);
}

/* Return beep interval in ms based on quality (0=mute) */
static int beepIntervalMs(int quality) {
    if (quality < 40) return 1200;
    if (quality < 60) return 600;
    if (quality < 75) return 300;
    if (quality < 88) return 150;
    return 70;
}

static int beepFreqHz(int quality) {
    if (quality < 50) return 1200;
    if (quality < 70) return 1500;
    if (quality < 85) return 1800;
    return 2200;
}

static int beepDurationMs(int quality) {
    if (quality >= 88) return 40;
    if (quality >= 75) return 50;
    return 60;
}

static std::string makeSolidBar(int pct, int length, const char* fillColor, const char* emptyColor) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int filled = (pct * length) / 100;
    int empty = length - filled;

    std::string res;
    res.reserve(length * 35);
    res += "<font color='"; res += fillColor; res += "'>";
    for (int i = 0; i < filled; ++i) {
        res += "\xE2\x96\x88";
    }
    res += "</font>";
    if (empty > 0) {
        res += "<font color='"; res += emptyColor; res += "'>";
        for (int i = 0; i < empty; ++i) {
            res += "\xE2\x96\x88";
        }
        res += "</font>";
    }
    return res;
}

static void showSnrMonitor() {
    std::string snr_db = "0.00";
    int snr_raw = 0;
    int quality = 0;
    int strength = 0;
    int ber = 0;

    FILE* fp = fopen("/data/.snr_value.txt", "r");
    if (fp) {
        char line[128];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "snr_db=", 7) == 0) {
                char* p = line + 7;
                while (*p == ' ') p++;
                char* e = p; while (*e > ' ') e++; *e = '\0';
                snr_db = p;
            } else if (strncmp(line, "snr_raw=", 8) == 0) {
                snr_raw = atoi(line + 8);
            } else if (strncmp(line, "quality_pct=", 12) == 0) {
                int q = atoi(line + 12);
                quality = (q * 100) / 255;
            } else if (strncmp(line, "strength_pct=", 13) == 0) {
                int s = atoi(line + 13);
                strength = (s * 100) / 255;
            } else if (strncmp(line, "ber=", 4) == 0) {
                ber = atoi(line + 4);
            }
        }
        fclose(fp);
    }
    if (snr_db.empty()) snr_db = "0.00";
    if (quality > 100) quality = 100;
    g_beepQuality = quality; /* cache for beeper interval */
    if (strength > 100) strength = 100;

    int snr_pct = (snr_raw * 100) / 1600;
    if (snr_pct > 100) snr_pct = 100;

    const int BAR_LEN = 22;
    std::string barSnr = makeSolidBar(snr_pct, BAR_LEN, "#00E5FF", "#22303D");
    std::string barStr = makeSolidBar(strength, BAR_LEN, "#FFD700", "#22303D");
    std::string barQ   = makeSolidBar(quality, BAR_LEN, "#00E676", "#22303D");

    char qBuf[16], sBuf[16], pBuf[16], bBuf[16];
    snprintf(qBuf, sizeof(qBuf), "%d%%", quality);
    snprintf(sBuf, sizeof(sBuf), "%d%%", strength);
    snprintf(pBuf, sizeof(pBuf), "%d%%", snr_pct);
    snprintf(bBuf, sizeof(bBuf), "%d", ber);

    std::string body;
    body.reserve(2048);
    body += "<big><b><font color='#00E5FF'>[ SIGNAL &amp; HARDWARE SNR ]</font></b></big><br/><br/>\n";
    body += "<font color='#CFD8DC'><b>SNR:</b></font>&nbsp;&nbsp;<font color='#00E5FF'><b>" + snr_db + " dB</b></font>&nbsp;&nbsp;<font color='#80DEEA'><small>(" + pBuf + ")</small></font><br/>\n";
    body += barSnr + "<br/><br/>\n";
    body += "<font color='#CFD8DC'><b>Level:</b></font>&nbsp;&nbsp;<font color='#FFD700'><b>" + std::string(sBuf) + "</b></font><br/>\n";
    body += barStr + "<br/><br/>\n";
    body += "<font color='#CFD8DC'><b>Quality:</b></font>&nbsp;&nbsp;<font color='#00E676'><b>" + std::string(qBuf) + "</b></font><br/>\n";
    body += barQ + "<br/><br/>\n";
    body += "<font color='#90A4AE'><small>Tuner 0 (AVL6261) &bull; BER: " + std::string(bBuf) + " &bull; Lock: OK</small></font><br/>\n";
    const char* beepIcon = g_beepMute ? "\xF0\x9F\x94\x87 MUTE" : "\xF0\x9F\x94\x8A BEEP:ON";
    body += "<font color='#78909C'><small>[EXIT/OK/8] close  [0] " + std::string(beepIcon) + "</small></font>\n";

    grabRemote();
    writeFile(HUD_TXT, body, true);
}

static void showMenu() {
    std::string cur = readCurrent();
    std::string body;
    body.reserve(3072);
    body += "<big><b>"; body += hcol(H_TITLE, M_TITLE); body += "</b></big>"; body += "<br/>\n";
    body += htmlItem(M_L1, H_SW_GOLD);  body += "<br/>\n";
    body += htmlItem(M_L2, H_SW_GREEN); body += "<br/>\n";
    body += htmlItem(M_L3, H_SW_RED);   body += "<br/>\n";
    body += htmlItem(M_L4, H_SW_CYAN);  body += "<br/>\n";
    body += htmlItem(M_L5, H_SW_NORMAL);body += "<br/>\n";
    body += htmlItem(M_CH_ENTRY, H_SW_NORMAL); body += "<br/>\n";
    body += htmlItem(M_UPDATE_ENTRY, H_SW_NORMAL); body += "<br/>\n";
    body += htmlItem(M_SNR_ENTRY, H_SW_CYAN); body += "<br/>\n";
    /* Dynamic badge if an update is available (purely local file read, NO network call) */
    UpdateStateInfo usi;
    if (readUpdateState(&usi) && usi.status == "UPDATE_AVAILABLE" && compareVersions(usi.latestVersion, CC_VERSION_STRING) > 0) {
        char avBuf[64];
        snprintf(avBuf, sizeof(avBuf), "%s%s", M_UPDATE_AVAIL, usi.latestVersion.c_str());
        body += "    "; body += hcol(H_BUSY, avBuf); body += "<br/>\n";
    }

    body += hcol(H_DIV, M_DIV_LINE);   body += "<br/>\n";
    body += hcol(H_HINT, M_CURR_LBL);  body += hcol(swatchOf(cur), std::string("<big>") + SW_DOT + "</big>");
    { std::string t = cur; stripEmoji(t); body += hcol(H_VAL, t); }
    body += "<br/>\n";
    body += hcol(H_HINT, M_CHCURR_LBL); body += hcol(H_SW_NORMAL, std::string("<big>") + SW_DOT + "</big>");
    { std::string t = readCurrentChannels(); stripEmoji(t); body += hcol(H_VAL, t); }
    body += "<br/>\n";
    body += hcol(H_SUB, M_HINT1);      body += "<br/>\n";
    body += hcol(H_SUB, M_HINT2);      body += "<br/>\n";

    bool grabbed = grabRemote();
    writeFile(HUD_TXT, body, true);
    dbg("[tm] HUD file written for persistent watcher (grabbed=%d)", grabbed ? 1 : 0);
}

/* ---------- Show the "نوع القنوات" sub-screen ----------
 * Counts come from variant_counts.txt (written by buildVariants); sqlite3 is
 * never run while the menu is on screen.  A missing file just drops the
 * "(n)" suffix from that line. */
static void showChannels() {
    static const char* lines[CHANNEL_MENU_COUNT] = {
        M_CH_L1, M_CH_L2, M_CH_L3, M_CH_L4
    };
    std::string cur = readCurrentChannels();
    std::string body;
    body.reserve(2048);
    body += "<big><b>"; body += hcol(H_TITLE, M_CH_TITLE); body += "</b></big>"; body += "<br/>\n";
    for (int i = 0; i < CHANNEL_MENU_COUNT && i < VARIANTS_N; ++i) {
        std::string item(lines[i]);
        std::string cnt = variantCountOf(VARIANTS[i].file.c_str());
        if (!cnt.empty()) { item += " ("; item += cnt; item += ")"; }
        body += htmlItem(item.c_str(), H_SW_NORMAL);
        body += "<br/>\n";
    }
    body += hcol(H_DIV, M_DIV_LINE);   body += "<br/>\n";
    body += hcol(H_HINT, M_CH_CURR);   body += hcol(H_SW_NORMAL, std::string("<big>") + SW_DOT + "</big>");
    { std::string t = cur; stripEmoji(t); body += hcol(H_VAL, t); }
    body += "<br/>\n";
    body += hcol(H_SUB, M_CH_HINT);    body += "<br/>\n";

    bool grabbed = grabRemote();
    writeFile(HUD_TXT, body, true);
    dbg("[tm] HUD file written (cur=%s grabbed=%d)", cur.c_str(), grabbed ? 1 : 0);
}

static void hideMenu() {
    if (g_fd >= 0) ioctl(g_fd, EVIOCGRAB, 0);
    g_grabbed = false;
    writeFile(CLEAR_TXT, " ", false);
    unlink(HUD_TXT);
}

/* ---------- Kill processes matching any of the cmdline patterns ----------
 * A pattern that matches nothing is NOT an error: the loop simply kills
 * nobody (see the dbg() below) and the caller keeps going.  Self and our
 * sqlite3 children are never eligible, so a sloppy pattern cannot make the
 * daemon kill itself. */
static void killByCmdline(const char* const* patterns, size_t npat, int sig) {
    DIR* d = opendir("/proc");
    if (!d) return;
    size_t hits = 0;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR) continue;
        const char* name = de->d_name;
        if (!isdigit((unsigned char)name[0])) continue;

        char cmdPath[64];
        snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", name);
        std::string cl = readFile(cmdPath, 1024);
        if (cl.empty()) continue;

        pid_t p = (pid_t)atoi(name);
        if (p <= 0 || p == getpid()) continue;                 /* never ourselves */
        if (cl.compare(0, strlen(SQLITE3_BIN), SQLITE3_BIN) == 0) continue;  /* our children */

        bool match = false;
        for (size_t i = 0; i < npat; ++i) {
            if (cl.find(patterns[i]) != std::string::npos) { match = true; break; }
        }
        if (!match) continue;

        hits++;
        kill(p, sig);
        dbg("[kill] sent signal %d to pid %d (cmdline=%s)", sig, p, cl.c_str());
    }
    closedir(d);
    if (hits == 0)
        dbg("[kill] no process matched any of %d pattern(s) - nothing was killed", (int)npat);
}

/* ---------- sqlite3 driver: SQL on stdin, never through a temp file ----------
 * sigChldHandler() reaps with waitpid(-1) and would swallow sqlite3's exit
 * status, so SIGCHLD is blocked from before fork() until after waitpid() on
 * this exact pid.  Callers still do not trust the status alone - they also
 * check what sqlite3 actually printed (see sqliteCheckOk). */
static int runSqliteSql(const char* dbPath, const std::string& sql, std::string* out) {
    if (out) out->clear();
    if (access(SQLITE3_BIN, X_OK) != 0) {
        dbg("[sql] not executable: %s", SQLITE3_BIN);
        return -1;
    }
    int pin[2], pout[2];
    if (pipe2(pin, O_CLOEXEC) != 0)  { dbg("[sql] pipe2(in) errno=%d", errno);  return -1; }
    if (pipe2(pout, O_CLOEXEC) != 0) { close(pin[0]); close(pin[1]);
                           dbg("[sql] pipe2(out) errno=%d", errno); return -1; }

    sigset_t block, oldmask;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &oldmask);

    pid_t pid = fork();
    if (pid < 0) {
        sigprocmask(SIG_SETMASK, &oldmask, nullptr);
        close(pin[0]); close(pin[1]); close(pout[0]); close(pout[1]);
        dbg("[sql] fork errno=%d", errno);
        return -1;
    }
    if (pid == 0) {
        dup2(pin[0],  STDIN_FILENO);
        dup2(pout[1], STDOUT_FILENO);
        dup2(pout[1], STDERR_FILENO);
        close(pin[0]); close(pin[1]); close(pout[0]); close(pout[1]);
        closeAllFdsAbove(3);
        /* the daemon has no HOME, which would make sqlite3 print
         * "cannot read ~/.sqliterc" on stderr before every answer - that text
         * would then pollute the parsed output (and contains digits). */
        setenv("HOME", LOCAL_TMP_DIR, 1);
        /* ...and a temp directory sqlite3 can really write to (VACUUM and
         * CREATE INDEX sort into it when temp_store is not MEMORY). */
        setenv("SQLITE_TMPDIR", LOCAL_TMP_DIR, 1);
        char* const argv[] = { (char*)SQLITE3_BIN, (char*)dbPath, nullptr };
        execv(SQLITE3_BIN, argv);
        _exit(127);
    }
    close(pin[0]);
    close(pout[1]);

    size_t off = 0;
    bool writing = true;
    bool timedOut = false;
    char buf[1024];
    for (;;) {
        struct pollfd pf[2];
        pf[0].fd = writing ? pin[1] : -1;
        pf[0].events = (short)(writing ? (POLLOUT | POLLERR | POLLHUP) : 0);
        pf[0].revents = 0;
        pf[1].fd = pout[0];
        pf[1].events = POLLIN | POLLERR | POLLHUP;
        pf[1].revents = 0;

        int pr = poll(pf, 2, SQL_RUN_TIMEOUT_MS);
        if (pr < 0) {
            if (errno == EINTR) continue;
            dbg("[sql] poll errno=%d", errno);
            break;
        }
        if (pr == 0) { timedOut = true; break; }

        if (writing && (pf[0].revents & (POLLOUT | POLLERR | POLLHUP))) {
            if (pf[0].revents & (POLLERR | POLLHUP)) {
                close(pin[1]); pin[1] = -1; writing = false;
            } else {
                ssize_t w = write(pin[1], sql.data() + off, sql.size() - off);
                if (w < 0) {
                    if (errno != EINTR && errno != EAGAIN) { close(pin[1]); pin[1] = -1; writing = false; }
                } else {
                    off += (size_t)w;
                    if (off >= sql.size()) { close(pin[1]); pin[1] = -1; writing = false; }
                }
            }
        }
        if (pf[1].revents & (POLLIN | POLLERR | POLLHUP)) {
            ssize_t n = read(pout[0], buf, sizeof(buf));
            if (n > 0) {
                if (out) out->append(buf, (size_t)n);
            } else if (n == 0) {
                break;
            } else if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        }
    }
    if (pin[1] >= 0) close(pin[1]);
    close(pout[0]);

    if (timedOut) { dbg("[sql] timeout after %d ms -> killed", SQL_RUN_TIMEOUT_MS); kill(pid, SIGKILL); }
    int status = 0;
    pid_t r = waitpid(pid, &status, 0);
    int werr = errno;                       /* sigprocmask below must not clobber it */
    sigprocmask(SIG_SETMASK, &oldmask, nullptr);
    if (r != pid) { dbg("[sql] waitpid r=%d errno=%d child=%d", (int)r, werr, (int)pid); return -1; }
    if (timedOut) return -1;
    if (!WIFEXITED(status)) { dbg("[sql] child did not exit normally"); return -1; }
    return WEXITSTATUS(status);
}

/* true when sqlite3 printed a line "ok" and nothing that looks like a
 * failure.  The exit status alone is never trusted: sigChldHandler() may have
 * reaped the child already, and a broken run can still exit 0. */
static bool outputSaysOk(const std::string& out) {
    std::string low = out;
    for (size_t i = 0; i < low.size(); ++i)
        low[i] = (char)tolower((unsigned char)low[i]);

    bool hasOk = false;
    for (size_t p = 0; p < low.size(); ) {
        size_t e = low.find('\n', p);
        std::string line = low.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
        while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == ' '))
            line.erase(line.size() - 1);
        if (line == "ok") { hasOk = true; break; }
        if (e == std::string::npos) break;
        p = e + 1;
    }
    if (!hasOk) return false;
    if (low.find("error") != std::string::npos) return false;
    if (low.find("malformed") != std::string::npos) return false;
    if (low.find("not a database") != std::string::npos) return false;
    return true;
}

/* PRAGMA integrity_check must print a line "ok"; rc alone is not trusted. */
static bool sqliteCheckOk(const char* dbPath, std::string* why) {
    std::string out;
    int rc = runSqliteSql(dbPath, "PRAGMA integrity_check;\n", &out);
    if (rc != 0 || !outputSaysOk(out)) {
        std::string one = out;
        for (size_t i = 0; i < one.size() && i < 120; ++i)
            if (one[i] == '\n' || one[i] == '\r') one[i] = ' ';
        if (one.size() > 120) one.resize(120);
        dbg("[sql] integrity FAILED rc=%d out='%s'", rc, one.c_str());
        if (why) *why = one;
        return false;
    }
    return true;
}

/* SELECT COUNT(*) FROM _svcInfo -> number, for variant_counts.txt.
 * sqlite3 prints whatever it likes before the answer (the ~/.sqliterc warning,
 * a "Run Time" note, ...), so the output is walked line by line and ONLY a
 * line that is nothing but digits is accepted - the first one wins.  A stray
 * "10685" hidden inside a sentence can therefore never be read as the count. */
static bool sqliteCount(const char* dbPath, long* count) {
    std::string out;
    int rc = runSqliteSql(dbPath, "SELECT COUNT(*) FROM _svcInfo;\n", &out);
    if (rc != 0) { dbg("[sql] count query rc=%d", rc); return false; }

    size_t p = 0;
    while (p < out.size()) {
        size_t e = out.find('\n', p);
        if (e == std::string::npos) e = out.size();
        std::string line = out.substr(p, e - p);
        size_t b = line.find_first_not_of(" \t\r");
        size_t t = line.find_last_not_of(" \t\r");
        if (b != std::string::npos) {
            std::string v = line.substr(b, t - b + 1);
            bool digits = !v.empty();
            for (size_t i = 0; i < v.size(); ++i)
                if (!isdigit((unsigned char)v[i])) { digits = false; break; }
            if (digits) {
                *count = strtol(v.c_str(), nullptr, 10);
                return true;
            }
        }
        p = e + 1;
    }

    std::string one = out;
    for (size_t i = 0; i < one.size() && i < 120; ++i)
        if (one[i] == '\n' || one[i] == '\r') one[i] = ' ';
    if (one.size() > 120) one.resize(120);
    dbg("[sql] count: no pure-numeric line in output '%s'", one.c_str());
    return false;
}

/* ---------- Apply color ----------
 * Two hard gates before anything is written:
 *   1. the box must be identified with certainty (model + sizes agree);
 *   2. the candidate .oat must be exactly the native size for that box.
 * Either gate failing aborts without touching the launcher. */
static bool confirmDevice(const char* devName) {
    /* hcol() already opens and closes its own <font> tag: wrapping its result
     * in another <font> (as this used to) nested two tags per line, which
     * Html.fromHtml resolves for the first line and silently drops the colour
     * of the ones after it.  One level only. */
    std::string dexShown = (g_nativeDexSize > 0)
        ? std::to_string(g_nativeDexSize)
        : std::string("?");
    char body[2048];
    int n = snprintf(body, sizeof(body),
        "<big><b>%s</b></big><br/>\n"
        "%s %s<br/>\n"
        "%s %s<br/>\n"
        "%s<br/>\n"
        "%s",
        hcol(H_TITLE, M_CONFIRM_TITLE).c_str(),
        hcol(H_ITEM, M_CONFIRM_D1).c_str(), hcol(H_VAL, devName).c_str(),
        hcol(H_ITEM, M_CONFIRM_D2).c_str(), hcol(H_VAL, dexShown).c_str(),
        hcol(H_SUB, M_CONFIRM_H1).c_str(),
        hcol(H_SUB, M_CONFIRM_H2).c_str());
    if (n > 0) writeFile(HUD_TXT, std::string(body, (size_t)n), true);

    /* Wait for OK (=1) or EXIT/confirm-cancel. Any other key is ignored. */
    time_t start = time(nullptr);
    struct pollfd pfd = { g_fd, POLLIN, 0 };
    struct input_event ev;
    while (g_running && time(nullptr) - start <= POLL_TIMEOUT_SEC) {
        if (poll(&pfd, 1, POLL_TIMEOUT_MS) <= 0) continue;
        if (read(g_fd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev)) continue;
        if (ev.type != EV_KEY || ev.value != 1) continue;
        unsigned code = ev.code;
        dbg("[confirm] key code=%u", code);
        if (code == KEY_1 || code == KEY_OK) {
            dbg("[confirm] user accepted device=%s", devName);
            return true;
        }
        if (isExitKey(code)) {
            dbg("[confirm] user cancelled");
            return false;
        }
    }
    dbg("[confirm] timed out -> abort");
    return false;
}

static std::vector<pid_t> findPidsByName(bool matchFServer, bool matchLauncher);

static void buildPatchA(uint8_t colorByte, uint8_t out[38]) {
    memset(out, 0, 38);
    // movw r2, #colorByte
    out[0] = 0x40; out[1] = 0xf2; out[2] = colorByte; out[3] = 0x02;
    // movt r2, #0x7f06
    out[4] = 0xc7; out[5] = 0xf6; out[6] = 0x06; out[7] = 0x72;
    // movw r1, #0x2f
    out[8] = 0x40; out[9] = 0xf2; out[10] = 0x2f; out[11] = 0x01;
    // movt r1, #0x7f06
    out[12] = 0xc7; out[13] = 0xf6; out[14] = 0x06; out[15] = 0x71;
    // ldrb.w r0, [r6, #0x39]
    out[16] = 0x96; out[17] = 0xf8; out[18] = 0x39; out[19] = 0x00;
    // b +0x07 (b.w -> end of patch)
    out[20] = 0x07; out[21] = 0xe0;
    // nop (0x00, 0xbf) padding
    for (int i = 22; i < 38; i += 2) {
        out[i] = 0x00; out[i + 1] = 0xbf;
    }
}

static void buildPatchB(uint8_t colorByte, uint8_t out[38]) {
    memset(out, 0, 38);
    // movw r1, #colorByte
    out[0] = 0x40; out[1] = 0xf2; out[2] = colorByte; out[3] = 0x01;
    // movt r1, #0x7f06
    out[4] = 0xc7; out[5] = 0xf6; out[6] = 0x06; out[7] = 0x71;
    // movw r5, #0x2f
    out[8] = 0x40; out[9] = 0xf2; out[10] = 0x2f; out[11] = 0x05;
    // movt r5, #0x7f06
    out[12] = 0xc7; out[13] = 0xf6; out[14] = 0x06; out[15] = 0x75;
    // ldr r0, [sp, #0x20]
    out[16] = 0x08; out[17] = 0x98;
    // cbz r0, +0x05
    out[18] = 0x40; out[19] = 0xb1;
    // ldrb.w r0, [r0, #0x39]
    out[20] = 0x90; out[21] = 0xf8; out[22] = 0x39; out[23] = 0x00;
    // b +0x05 (b.w -> end of patch)
    out[24] = 0x05; out[25] = 0xe0;
    // nop (0x00, 0xbf) padding
    for (int i = 26; i < 38; i += 2) {
        out[i] = 0x00; out[i + 1] = 0xbf;
    }
}

/* Automatic speed AOT compiler: if the box still has the stock uncompiled (11MB)
 * Launcher or any size != g_nativeDexSize, dex2oat compiles it on the box itself
 * with --compiler-filter=speed into the full native ARM binary. Zero manual intervention. */
static bool compileLauncherAot(const char* colHex, bool showHud) {
    if (showHud) {
        std::string msg = hcol(colHex ? colHex : "#FFFFFF",
            "\xE2\x8F\xB3\x20\xD8\xAC\xD8\xA7\xD8\xB1\xD9\x8A\x20\xD8\xAA\xD9\x87\xD9\x8A\xD8\xA6\xD8\xA9\x20\xD9\x88\xD8\xAA\xD8\xB3\xD8\xB1\xD9\x8A\xD8\xB9\x20\xD8\xA7\xD9\x84\xD9\x86\xD8\xB8\xD8\xA7\xD9\x85\x20\xD8\xAA\xD9\x84\xD9\x82\xD8\xA7\xD8\xA6\xD9\x8A\xD8\xA7\xD9\x8B\x2E\x2E\x2E"); /* ⏳ جاري تهيئة وتسريع النظام تلقائياً... */
        writeFile(HUD_TXT, msg);
    }
    dbg("[aot] starting auto dex2oat speed compilation for Launcher.apk (want %lld bytes)...", g_nativeDexSize);

    sigset_t block, oldmask;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &oldmask);

    pid_t pid = fork();
    if (pid == 0) {
        sigprocmask(SIG_SETMASK, &oldmask, nullptr);
        int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > 2) close(devnull);
        }
        closeAllFdsAbove(3);
        char* const argv[] = {
            (char*)"/system/bin/dex2oat",
            (char*)"--dex-file=/system/priv-app/Launcher/Launcher.apk",
            (char*)CMD_OAT_FILE_ARG,
            (char*)"--instruction-set=arm",
            (char*)"--compiler-filter=speed",
            (char*)"--runtime-arg", (char*)"-Xms64m",
            (char*)"--runtime-arg", (char*)"-Xmx512m",
            nullptr
        };
        execv("/system/bin/dex2oat", argv);
        _exit(127);
    }

    if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
    }
    sigprocmask(SIG_SETMASK, &oldmask, nullptr);

    chmod(DALVIK_TARGET, 0644);
    chown(DALVIK_TARGET, 1000, 1000);
    sync();

    long long sz = fileSizeOf(DALVIK_TARGET);
    dbg("[aot] finished dex2oat: size=%lld", sz);
    return (sz >= 18 * 1024 * 1024);
}

/* Dynamically locate patchA and patchB offsets by byte signature.
 * Works seamlessly across stock, patched, and updated Launchers of any size! */
static bool findDynamicPatchOffsets(off_t* outA, off_t* outB) {
    if (!outA || !outB) return false;
    *outA = 0; *outB = 0;

    int fd = open(DALVIK_TARGET, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;

    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < 12 * 1024 * 1024) {
        close(fd);
        return false;
    }

    size_t sz = (size_t)st.st_size;
    void* map = mmap(nullptr, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map == MAP_FAILED) return false;

    static const uint8_t sigA[16] = {
        0x88, 0x46, 0x08, 0x68, 0xd0, 0xf8, 0xb8, 0x01,
        0xd0, 0xf8, 0x20, 0xe0, 0xf0, 0x47, 0x48, 0xb9
    };
    static const uint8_t sigB[16] = {
        0x0d, 0x46, 0x08, 0x68, 0xd0, 0xf8, 0xb8, 0x01,
        0xd0, 0xf8, 0x20, 0xe0, 0xf0, 0x47, 0x48, 0xb9
    };

    const uint8_t* pA = (const uint8_t*)memmem(map, sz, sigA, sizeof(sigA));
    const uint8_t* pB = (const uint8_t*)memmem(map, sz, sigB, sizeof(sigB));

    bool ok = false;
    if (pA && pB) {
        *outA = (off_t)((uintptr_t)pA - (uintptr_t)map) + 18;
        *outB = (off_t)((uintptr_t)pB - (uintptr_t)map) + 18;
        dbg("[dy] discovered dynamic offsets: offA=0x%lx offB=0x%lx (size=%zu)",
            (long)*outA, (long)*outB, sz);
        ok = true;
    } else {
        dbg("[dy] dynamic offset search: sigA=%s sigB=%s (file size=%zu)",
            pA ? "found" : "MISSING", pB ? "found" : "MISSING", sz);
    }

    munmap(map, sz);
    return ok;
}

static bool patchColorInPlace(uint8_t colorByte, const char* name, bool showHud = true) {
    if (isUpdateLocked()) {
        dbg("[pt] REFUSED: update in progress (lock active)");
        return false;
    }

    const char* colHex = colorHexOf(colorByte);

    /* ---- Gate 1: device must be known with certainty ---- */
    if (g_devType == DEV_UNKNOWN) {
        dbg("[pt] REFUSED: device unknown (model='%s')", g_modelName);
        if (showHud) {
            writeFile(HUD_TXT, hcol(colHex, std::string(S_ERR) + M_REFUSE));
            sleep(ERROR_DELAY_SEC);
            hideMenu();
        }
        return false;
    }

    const char* devName = g_profileName;

    /* ---- Gate 2: Dalvik cache target must exist ---- */
    long long nowSize = fileSizeOf(DALVIK_TARGET);
    if (nowSize < 0) {
        dbg("[pt] REFUSED: target file missing: %s", DALVIK_TARGET);
        if (showHud) {
            writeFile(HUD_TXT, hcol(colHex, std::string(S_ERR) + DALVIK_TARGET + S_ERR2));
            sleep(ERROR_DELAY_SEC);
            hideMenu();
        }
        return false;
    }

    /* Dynamic offset discovery: locates hooks in ANY compiled Launcher (stock, patched, updated) */
    off_t dynA = 0, dynB = 0;
    if (!findDynamicPatchOffsets(&dynA, &dynB)) {
        dbg("[pt] sig not found (size=%lld), compiling AOT...", nowSize);
        if (compileLauncherAot(colHex, showHud)) {
            findDynamicPatchOffsets(&dynA, &dynB);
        }
    }

    if (dynA > 0 && dynB > 0) {
        g_patchAOff = dynA;
        g_patchBOff = dynB;
    } else if (g_patchAOff == 0 || g_patchBOff == 0) {
        dbg("[pt] REFUSED: offsets missing for dev=%s (size=%lld)", devName, fileSizeOf(DALVIK_TARGET));
        if (showHud) {
            writeFile(HUD_TXT, hcol(colHex, std::string(S_ERR) + M_WRONGDEV));
            sleep(ERROR_DELAY_SEC);
            hideMenu();
        }
        return false;
    }

    if (showHud) {
        std::string msg = hcol(colHex, std::string(S_START) + S_START2 + name + S_DOT);
        writeFile(HUD_TXT, msg);
        usleep(APPLY_DELAY_US);
    }

    /* ---- Gate 3: Kill launcher BEFORE opening file to avoid SIGBUS crash ---- */
    std::vector<pid_t> lnPids = findPidsByName(false, true);
    for (pid_t p : lnPids) {
        kill(p, SIGKILL);
        dbg("[pt] killed launcher pid=%d before pwrite", (int)p);
    }
    usleep(150000);   /* allow file descriptors and mmap pages to be fully released */

    /* ---- Gate 4: Open file for atomic in-place patching ---- */
    int fd = open(DALVIK_TARGET, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        dbg("[pt] open %s O_RDWR failed errno=%d", DALVIK_TARGET, errno);
        if (showHud) {
            writeFile(HUD_TXT, hcol(colHex, std::string(S_ERR) + M_POSTFAIL));
            sleep(ERROR_DELAY_SEC);
            hideMenu();
        }
        return false;
    }

    /* ---- Gate 5: Write the full 38-byte hooks (auto-hooks raw stock files & updates color) ---- */
    uint8_t patchA[38];
    uint8_t patchB[38];
    buildPatchA(colorByte, patchA);
    buildPatchB(colorByte, patchB);

    off_t startA = g_patchAOff - 2;
    off_t startB = g_patchBOff - 2;

    ssize_t w1 = pwrite(fd, patchA, 38, startA);
    ssize_t w2 = pwrite(fd, patchB, 38, startB);
    if (w1 != 38 || w2 != 38) {
        dbg("[pt] pwrite failed w1=%zd w2=%zd errno=%d", w1, w2, errno);
        close(fd);
        if (showHud) {
            writeFile(HUD_TXT, hcol(colHex, std::string(S_ERR) + M_POSTFAIL));
            sleep(ERROR_DELAY_SEC);
            hideMenu();
        }
        return false;
    }
    fsync(fd);
    close(fd);

    /* ---- Gate 6: Read-back post-verification ---- */
    fd = open(DALVIK_TARGET, O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        uint8_t read_a = 0, read_b = 0;
        pread(fd, &read_a, 1, g_patchAOff);
        pread(fd, &read_b, 1, g_patchBOff);
        close(fd);
        if (read_a != colorByte || read_b != colorByte) {
            dbg("[pt] POST-CHECK FAILED: read_a=0x%02x read_b=0x%02x want=0x%02x", read_a, read_b, colorByte);
            if (showHud) {
                writeFile(HUD_TXT, hcol(colHex, std::string(S_ERR) + M_POSTFAIL));
                sleep(ERROR_DELAY_SEC);
                hideMenu();
            }
            return false;
        }
    }

    dbg("[pt] hooked (0x%02x) at 0x%lx & 0x%lx",
        colorByte, (long)startA, (long)startB);

    writeFile(CUR_CHOICE_FILE, std::string(name) + "\n", true);

    /* Wait for fresh launcher instance to appear */
    for (int i = 0; i < 30; ++i) {
        std::vector<pid_t> curLn = findPidsByName(false, true);
        if (!curLn.empty()) {
            dbg("[pt] fresh launcher running with pid=%d (after %.1fs)", (int)curLn[0], i * 0.1);
            break;
        }
        usleep(100000);
    }

    if (showHud) {
        writeFile(HUD_TXT, hcol(colHex, std::string(S_OK) + name + S_OK2));
        sleep(SUCCESS_DELAY_SEC);
        hideMenu();
    }
    return true;
}

static void healColorFromChoice() {
    std::string cur = readFile(CUR_CHOICE_FILE);
    uint8_t targetByte = 0xd9;
    const char* targetName = N_GOLD;

    if (!cur.empty()) {
        if (cur.find("Gold") != std::string::npos || cur.find("\xD8\xB0\xD9\x87\xD8\xA8\xD9\x8A") != std::string::npos) {
            targetByte = 0xd9; targetName = N_GOLD;
        } else if (cur.find("Green") != std::string::npos || cur.find("\xD8\xA3\xD8\xAE\xD8\xB6\xD8\xB1") != std::string::npos) {
            targetByte = 0xda; targetName = N_GREEN;
        } else if (cur.find("Red") != std::string::npos || cur.find("\xD8\xA3\xD8\xAD\xD9\x85\xD8\xB1") != std::string::npos) {
            targetByte = 0xd7; targetName = N_RED;
        } else if (cur.find("Cyan") != std::string::npos || cur.find("\xD8\xB3\xD9\x85\xD8\xA7\xD9\x88\xD9\x8A") != std::string::npos) {
            targetByte = 0xcf; targetName = N_CYAN;
        } else if (cur.find("Default") != std::string::npos || cur.find("\xD8\xA7\xD9\x81\xD8\xAA\xD8\xB1\xD8\xA7\xD8\xB6\xD9\x8A") != std::string::npos) {
            targetByte = 0x2f; targetName = N_DEFAULT;
        }
    }

    if (targetByte == 0 || g_patchAOff == 0 || g_patchBOff == 0) return;

    int fd = open(DALVIK_TARGET, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;

    uint8_t curByte = 0;
    if (pread(fd, &curByte, 1, g_patchAOff) == 1) {
        close(fd);
        if (curByte != targetByte) {
            dbg("[heal] dalvik byte (0x%02x) != choice (0x%02x) -> healing in place",
                curByte, targetByte);
            patchColorInPlace(targetByte, targetName, false /* silent */);
        } else {
            dbg("[heal] dalvik byte matches choice (0x%02x)", curByte);
        }
    } else {
        close(fd);
    }
}

/* ---------- Process identification & f_server tracking helpers ---------- */
struct FServerProcState {
    pid_t pid = -1;
    pid_t ppid = -1;
    uid_t uid = 0;
    gid_t gid = 0;
    std::string exe;
    std::string cwd;
    std::string cmdline;
    std::vector<std::string> argv;
    std::vector<std::string> env;
    std::string selinuxAttr;
};

static bool isFServerPid(const char* pidStr) {
    if (!pidStr || !isdigit((unsigned char)pidStr[0])) return false;
    
    // Check 1: /proc/PID/exe symlink target basename
    char exePath[64];
    snprintf(exePath, sizeof(exePath), "/proc/%s/exe", pidStr);
    char exeTarget[256];
    ssize_t n = readlink(exePath, exeTarget, sizeof(exeTarget) - 1);
    if (n > 0) {
        exeTarget[n] = '\0';
        const char* base = strrchr(exeTarget, '/');
        const char* exeBase = base ? (base + 1) : exeTarget;
        if (strcmp(exeBase, PROC_NAME_F_SERVER) == 0) return true;
    }

    // Check 2: argv[0] basename from /proc/PID/cmdline
    char cmdPath[64];
    snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", pidStr);
    FILE* fp = fopen(cmdPath, "rb");
    if (fp) {
        char buf[256];
        size_t rd = fread(buf, 1, sizeof(buf) - 1, fp);
        fclose(fp);
        if (rd > 0) {
            buf[rd] = '\0';
            const char* base = strrchr(buf, '/');
            const char* arg0Base = base ? (base + 1) : buf;
            if (strcmp(arg0Base, PROC_NAME_F_SERVER) == 0) return true;
        }
    }
    return false;
}

static bool isLauncherPid(const char* pidStr) {
    if (!pidStr || !isdigit((unsigned char)pidStr[0])) return false;
    char cmdPath[64];
    snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", pidStr);
    FILE* fp = fopen(cmdPath, "rb");
    if (!fp) return false;
    char buf[256];
    size_t rd = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    if (rd == 0) return false;
    buf[rd] = '\0';
    if (strcmp(buf, PROC_NAME_LAUNCHER_DVB) == 0 ||
        strcmp(buf, PROC_NAME_LAUNCHER_HISI) == 0) {
        return true;
    }
    return false;
}

static std::vector<pid_t> findPidsByName(bool checkFServer, bool checkLauncher) {
    std::vector<pid_t> pids;
    DIR* d = opendir("/proc");
    if (!d) return pids;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR && de->d_type != DT_UNKNOWN) continue;
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        pid_t p = (pid_t)atoi(de->d_name);
        if (p <= 0 || p == getpid()) continue;
        if (checkFServer && isFServerPid(de->d_name)) {
            pids.push_back(p);
        } else if (checkLauncher && isLauncherPid(de->d_name)) {
            pids.push_back(p);
        }
    }
    closedir(d);
    return pids;
}

static bool recordFServerProcState(const char* pidStr, FServerProcState* st) {
    if (!st || !pidStr) return false;
    st->pid = (pid_t)atoi(pidStr);
    if (st->pid <= 0) return false;

    char path[128];
    snprintf(path, sizeof(path), "/proc/%s/exe", pidStr);
    char buf[512];
    ssize_t n = readlink(path, buf, sizeof(buf) - 1);
    if (n > 0) { buf[n] = '\0'; st->exe = buf; }

    snprintf(path, sizeof(path), "/proc/%s/cwd", pidStr);
    n = readlink(path, buf, sizeof(buf) - 1);
    if (n > 0) { buf[n] = '\0'; st->cwd = buf; }

    snprintf(path, sizeof(path), "/proc/%s/status", pidStr);
    FILE* fp = fopen(path, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "PPid:", 5) == 0) {
                st->ppid = (pid_t)atoi(line + 5);
            } else if (strncmp(line, "Uid:", 4) == 0) {
                st->uid = (uid_t)atoi(line + 4);
            } else if (strncmp(line, "Gid:", 4) == 0) {
                st->gid = (gid_t)atoi(line + 4);
            }
        }
        fclose(fp);
    }

    snprintf(path, sizeof(path), "/proc/%s/attr/current", pidStr);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp)) {
            size_t l = strlen(buf);
            while (l > 0 && (buf[l-1] == '\n' || buf[l-1] == '\r')) buf[--l] = '\0';
            st->selinuxAttr = buf;
        }
        fclose(fp);
    }

    snprintf(path, sizeof(path), "/proc/%s/cmdline", pidStr);
    fp = fopen(path, "rb");
    if (fp) {
        char raw[2048];
        size_t rd = fread(raw, 1, sizeof(raw), fp);
        fclose(fp);
        st->cmdline.clear();
        st->argv.clear();
        size_t p = 0;
        while (p < rd) {
            size_t len = strlen(raw + p);
            std::string arg(raw + p, len);
            st->argv.push_back(arg);
            if (!st->cmdline.empty()) st->cmdline += " ";
            st->cmdline += arg;
            p += len + 1;
        }
    }

    snprintf(path, sizeof(path), "/proc/%s/environ", pidStr);
    fp = fopen(path, "rb");
    if (fp) {
        char raw[4096];
        size_t rd = fread(raw, 1, sizeof(raw), fp);
        fclose(fp);
        st->env.clear();
        size_t p = 0;
        while (p < rd) {
            size_t len = strlen(raw + p);
            std::string e(raw + p, len);
            st->env.push_back(e);
            p += len + 1;
        }
    }

    return true;
}

static void scanServiceDbHolders(std::string* out);   /* defined with the report */
static std::string oneLine(std::string s, size_t max);

struct HiHolder {
    pid_t pid;
    pid_t ppid;
    std::string comm;
    std::string cmdline;
    std::vector<std::string> devices;
    bool isFServer;
    bool isSystem;
};
static std::vector<HiHolder> scanHiHolders(bool foreignOnly = false);
static int countDmesgDmxErrors();

static bool isDvbApiReady() {
    FILE* fp = popen(CMD_CHECK_DVB, "r");
    if (!fp) return false;
    char buf[128];
    bool found = false;
    while (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, "found") && !strstr(buf, "not found")) {
            found = true;
            break;
        }
    }
    pclose(fp);
    return found;
}

static std::string getFileMd5(const char* path) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "/system/bin/md5sum %s 2>/dev/null", path);
    FILE* fp = popen(cmd, "r");
    if (!fp) return "";
    char buf[128];
    std::string md5;
    if (fgets(buf, sizeof(buf), fp)) {
        char hash[64];
        if (sscanf(buf, "%63s", hash) == 1) md5 = hash;
    }
    pclose(fp);
    return md5;
}

static void ensureUninstallWatchdog();

static bool ensureUserChannelsBackup(bool forceUpdate = false) {
    if (!forceUpdate && access(CC_BACKUP_SERVICE_DB, F_OK) == 0) {
        dbg("[backup] already exists at %s", CC_BACKUP_DIR);
        ensureUninstallWatchdog();
        return true;
    }

    if (access(SERVICE_DB, F_OK) != 0) {
        dbg("[backup] WARNING: source %s does not exist, cannot backup", SERVICE_DB);
        return false;
    }

    long long srcSz = fileSizeOf(SERVICE_DB);
    if (srcSz < CHANNEL_DB_MIN_BYTES) {
        dbg("[backup] WARNING: %s is too small (%lld bytes), refusing backup", SERVICE_DB, srcSz);
        return false;
    }

    mkdir("/data/.ColorPro_d", 0700);
    mkdir(CC_BACKUP_DIR, 0700);
    chmod(CC_BACKUP_DIR, 0700);

    /* Atomic copy of service.db to service.db.orig */
    std::string tmpDb = std::string(CC_BACKUP_SERVICE_DB) + ".tmp";
    unlink(tmpDb.c_str());
    if (!copyFileTo(SERVICE_DB, tmpDb.c_str())) {
        dbg("[backup] ERROR: copy %s -> %s failed", SERVICE_DB, tmpDb.c_str());
        unlink(tmpDb.c_str());
        return false;
    }
    chmod(tmpDb.c_str(), 0600);
    if (rename(tmpDb.c_str(), CC_BACKUP_SERVICE_DB) != 0) {
        dbg("[backup] ERROR: rename -> %s failed errno=%d", CC_BACKUP_SERVICE_DB, errno);
        unlink(tmpDb.c_str());
        return false;
    }

    /* Copy orca keys if present */
    std::string orcaPath1 = std::string(DATA_DIR) + ORCA_KEYS_SLASH;
    const char* const ORCA_KEY_PATHS[] = {
        orcaPath1.c_str(),
        ORCA_KEYS_PLUGIN,
        nullptr
    };
    for (int i = 0; ORCA_KEY_PATHS[i]; ++i) {
        if (access(ORCA_KEY_PATHS[i], F_OK) == 0) {
            std::string tmpK = std::string(CC_BACKUP_ORCA_KEYS) + ".tmp";
            if (copyFileTo(ORCA_KEY_PATHS[i], tmpK.c_str())) {
                chmod(tmpK.c_str(), 0600);
                rename(tmpK.c_str(), CC_BACKUP_ORCA_KEYS);
                dbg("[backup] backed up orca keys from %s", ORCA_KEY_PATHS[i]);
            }
            break;
        }
    }

    /* Write backup_meta.txt */
    std::string md5 = getFileMd5(CC_BACKUP_SERVICE_DB);
    char meta[512];
    snprintf(meta, sizeof(meta),
             "timestamp=%ld\n"
             "size=%lld\n"
             "md5=%s\n"
             "version=%s\n",
             (long)time(nullptr), srcSz, md5.c_str(), CC_VERSION_STRING);
    writeFile(CC_BACKUP_META, meta, true);
    chmod(CC_BACKUP_META, 0600);

    /* Write restore.sh */
    static const char* RESTORE_SH_CONTENT =
        "#!/system/bin/sh\n"
        "# \xD8\xA7\xD8\xB3\xD8\xAA\xD8\xB1\xD8\xac\xD8\xA7\xD8\xB9\x20\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\x20\xD8\xA7\xD9\x84\xD9\x85\xD8\xB3\xD8\xAA\xD8\xAE\xD8\xAF\xD9\x85\x20\xD8\xA7\xD9\x84\xD8\xA3\xD8\xB5\xD9\x84\xD9\x8A\n"
        "if [ -f /data/.ColorPro_d/backup/service.db.orig ]; then\n"
        "    cp /data/.ColorPro_d/backup/service.db.orig /data/db/service.db\n"
        "    rm -f /data/db/service.db-journal /data/db/service.db-wal /data/db/service.db-shm\n"
        "    sync\n"
        "    echo \"Restored. Rebooting...\"\n"
        "    reboot\n"
        "else\n"
        "    echo \"Error: /data/.ColorPro_d/backup/service.db.orig not found!\"\n"
        "fi\n";
    writeFile(CC_BACKUP_RESTORE_SH, RESTORE_SH_CONTENT, true);
    chmod(CC_BACKUP_RESTORE_SH, 0755);

    int dirFd = open(CC_BACKUP_DIR, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dirFd >= 0) {
        fsync(dirFd);
        close(dirFd);
    }
    sync();

    dbg("[backup] %s at %s", forceUpdate ? "updated" : "created", CC_BACKUP_DIR);
    ensureUninstallWatchdog();
    return true;
}

static bool performSilentRestoreAndExit() {
    dbg("[uninstall] silent restore triggered (plugin removed from plugin list)");
    if (access(CC_BACKUP_SERVICE_DB, F_OK) == 0) {
        std::string tmpDst = std::string(SERVICE_DB) + ".restore";
        unlink(tmpDst.c_str());
        if (copyFileTo(CC_BACKUP_SERVICE_DB, tmpDst.c_str())) {
            chmod(tmpDst.c_str(), 0644);
            chown(tmpDst.c_str(), 1000, 1000);
            rename(tmpDst.c_str(), SERVICE_DB);
        }
        unlink(SERVICE_DB_JOURNAL);
        unlink(SERVICE_DB_WAL);
        unlink(SERVICE_DB_SHM);
        if (access("/system/bin/restorecon", X_OK) == 0) {
            system(CMD_RESTORECON);
        }
    }
    if (access(CC_BACKUP_ORCA_KEYS, F_OK) == 0) {
        copyFileTo(CC_BACKUP_ORCA_KEYS, ORCA_KEYS_PLUGIN);
        chmod(ORCA_KEYS_PLUGIN, 0644);
    }
    unlink(TARGET_BIN_PATH);
    unlink(TARGET_BIN_PNG);
    unlink(TARGET_BIN_PREV);
    unlink(UPDATE_GUARD_BIN);
    unlink(WARM_TXT);
    unlink(DBG_FILE);
    unlink(HUD_TXT);
    unlink(CLEAR_TXT);
    unlink(PID_FILE);

    DIR* d = opendir(DATA_DIR);
    if (d) {
        struct dirent* de;
        while ((de = readdir(d)) != nullptr) {
            if (de->d_name[0] == '.') continue;
            std::string p = std::string(DATA_DIR) + "/" + de->d_name;
            unlink(p.c_str());
        }
        closedir(d);
        rmdir(DATA_DIR);
    }
    sync();
    system("/system/bin/sync; /system/bin/reboot");
    _exit(0);
    return true;
}

static void ensureUninstallWatchdog() {
    mkdir("/data/.ColorPro_d", 0700);
    mkdir(CC_BACKUP_DIR, 0700);

    static const char* WATCHDOG_SCRIPT =
        "#!/system/bin/sh\n"
        "# Silent restore watchdog - ColorPro\n"
        "PIDFILE=\"/data/.ColorPro_d/watchdog.pid\"\n"
        "if [ -f \"$PIDFILE\" ]; then\n"
        "    OLDPID=$(cat \"$PIDFILE\" 2>/dev/null)\n"
        "    if [ -n \"$OLDPID\" ] && [ -d \"/proc/$OLDPID\" ]; then\n"
        "        exit 0\n"
        "    fi\n"
        "fi\n"
        "echo $$ > \"$PIDFILE\"\n"
        "\n"
        "BACKUP=\"/data/.ColorPro_d/backup/service.db.orig\"\n"
        "PLUGIN=\"/data/plugin/ColorPro\"\n"
        "LOCK=\"/data/plugin/ColorPro_data/update.lock\"\n"
        "\n"
        "while true; do\n"
        "    if [ ! -f \"$PLUGIN\" ] && [ ! -f \"$LOCK\" ]; then\n"
        "        sleep 2\n"
        "        if [ ! -f \"$PLUGIN\" ] && [ ! -f \"$LOCK\" ]; then\n"
        "            break\n"
        "        fi\n"
        "    fi\n"
        "    sleep 2\n"
        "done\n"
        "\n"
        "if [ -f \"$BACKUP\" ]; then\n"
        "    cp -f \"$BACKUP\" /data/db/service.db\n"
        "    chmod 644 /data/db/service.db\n"
        "    chown 1000:1000 /data/db/service.db\n"
        "    rm -f /data/db/service.db-journal /data/db/service.db-wal /data/db/service.db-shm\n"
        "    if [ -f /data/.ColorPro_d/backup/keys.bin.orig ]; then\n"
        "        cp -f /data/.ColorPro_d/backup/keys.bin.orig /data/plugin/orca_open_keys.bin\n"
        "        chmod 644 /data/plugin/orca_open_keys.bin\n"
        "    fi\n"
        "    rm -rf /data/plugin/ColorPro_data\n"
        "    rm -f /data/plugin/ColorPro.*\n"
        "    rm -f /data/local/tmp/*colorpro*\n"
        "    rm -rf /data/.ColorPro_d\n"
        "    sync\n"
        "    reboot\n"
        "fi\n";

    const char* path = "/data/.ColorPro_d/watchdog.sh";
    writeFile(path, WATCHDOG_SCRIPT, true);
    chmod(path, 0755);

    system("setsid /system/bin/sh /data/.ColorPro_d/watchdog.sh >/dev/null 2>&1 </dev/null &");
    dbg("[watchdog] silent uninstall watchdog ensured");
}

static bool performAddonUninstall() {
    dbg("[uninstall] user confirmed addon removal & original channels restore");
    if (access(CC_BACKUP_SERVICE_DB, F_OK) != 0) {
        dbg("[uninstall] REFUSED: backup file %s does not exist!", CC_BACKUP_SERVICE_DB);
        writeFile(HUD_TXT, hcol(H_ERR, M_BACKUP_RESTORE_ERR), true);
        sleep(ERROR_DELAY_SEC + 2);
        return false;
    }

    std::string body;
    body += "<big><b>";
    body += hcol(H_BUSY, M_BACKUP_RESTORING_NOTIF);
    body += "</b></big><br/>\n";
    body += hcol(H_SUB, "Restoring channels & removing addon... Rebooting...");
    body += "<br/>\n";
    writeFile(HUD_TXT, body, true);

    /* 1. Restore service.db from backup atomically */
    std::string tmpDst = std::string(SERVICE_DB) + ".restore";
    unlink(tmpDst.c_str());
    if (!copyFileTo(CC_BACKUP_SERVICE_DB, tmpDst.c_str())) {
        dbg("[uninstall] copy backup -> %s failed", tmpDst.c_str());
        writeFile(HUD_TXT, hcol(H_ERR, M_BACKUP_RESTORE_ERR), true);
        sleep(ERROR_DELAY_SEC + 2);
        return false;
    }
    chmod(tmpDst.c_str(), 0644);
    chown(tmpDst.c_str(), 1000, 1000);
    if (rename(tmpDst.c_str(), SERVICE_DB) != 0) {
        dbg("[uninstall] rename %s -> %s failed errno=%d", tmpDst.c_str(), SERVICE_DB, errno);
        writeFile(HUD_TXT, hcol(H_ERR, M_BACKUP_RESTORE_ERR), true);
        sleep(ERROR_DELAY_SEC + 2);
        return false;
    }

    /* 2. Delete journal/wal/shm */
    unlink(SERVICE_DB_JOURNAL);
    unlink(SERVICE_DB_WAL);
    unlink(SERVICE_DB_SHM);

    if (access("/system/bin/restorecon", X_OK) == 0) {
        system(CMD_RESTORECON);
    }
    sync();
    dbg("[uninstall] restored original service.db");

    /* 3. Restore orca keys if present */
    if (access(CC_BACKUP_ORCA_KEYS, F_OK) == 0) {
        copyFileTo(CC_BACKUP_ORCA_KEYS, ORCA_KEYS_PLUGIN);
        chmod(ORCA_KEYS_PLUGIN, 0644);
    }

    /* 4. Delete addon files (stealth identity) */
    unlink(TARGET_BIN_PATH);
    unlink(TARGET_BIN_PNG);
    unlink(TARGET_BIN_PREV);
    unlink(UPDATE_GUARD_BIN);
    unlink(WARM_TXT);
    unlink(DBG_FILE);

    /* Remove DATA_DIR recursively */
    DIR* d = opendir(DATA_DIR);
    if (d) {
        struct dirent* de;
        while ((de = readdir(d)) != nullptr) {
            if (de->d_name[0] == '.') continue;
            std::string p = std::string(DATA_DIR) + "/" + de->d_name;
            unlink(p.c_str());
        }
        closedir(d);
        rmdir(DATA_DIR);
    }
    unlink(HUD_TXT);
    unlink(CLEAR_TXT);
    unlink(PID_FILE);

    dbg("[uninstall] addon files removed, issuing sync & reboot");
    sync();
    usleep(500000);
    system("/system/bin/sync; /system/bin/reboot");
    return true;
}

static void showConfirmRemoveHud() {
    std::string body;
    body.reserve(1024);
    body += "<big><b>";
    body += hcol(H_ERR, M_BACKUP_REMOVE_BTN);
    body += "</b></big><br/>\n";
    body += hcol(H_WARN, M_BACKUP_CONFIRM_REMOVE);
    body += "<br/><br/>\n";
    body += hcol(H_SW_GREEN, "OK / [1]: ") + hcol(H_VAL, "\xD9\x86\xD8\xB9\xD9\x85\xD8\x8C\x20\xD8\xA7\xD8\xB3\xD8\xAA\xD8\xB1\xD8\xac\xD8\xB9\x20\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\xD9\x8a\x20\xD9\x88\xD8\xA7\xD8\xad\xD8\xB0\xD9\x81\x20\xD8\xA7\xD9\x84\xD8\xA5\xD8\xb6\xD8\xA7\xD9\x81\xD8\xA9"); /* نعم، استرجع قنواتي واحذف الإضافة */
    body += "<br/>\n";
    body += hcol(H_SUB, "EXIT: ") + hcol(H_VAL, "\xD8\xA5\xD9\x84\xD8\xBA\xD8\xA7\xD8\xA1\x20\xD9\x88\xD8\xA7\xD9\x84\xD8\xB9\xD9\x88\xD8\xAF\xD8\xA9\x20\xD9\x84\xD9\x84\xD9\x82\xD8\xA7\xD8\xA6\xD9\x85\xD8\xA9"); /* إلغاء والعودة للقائمة */
    body += "<br/>\n";
    body += hcol(H_DIV, M_DIV_LINE);
    body += "<br/>\n";
    body += hcol(H_HINT, M_BACKUP_CONFIRM_HINT);
    body += "<br/>\n";
    grabRemote();
    writeFile(HUD_TXT, body, true);
}

static void showConfirmUpdateBackupHud() {
    std::string body;
    body.reserve(1024);
    body += "<big><b>";
    body += hcol(H_SW_GOLD, M_BACKUP_UPDATE_BTN);
    body += "</b></big><br/>\n";
    body += hcol(H_WARN, "\xD9\x87\xD9\x84\x20\xD8\xAA\xD8\xB1\xD9\x8A\xD8\xAF\x20\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD8\xA7\xD9\x84\xD9\x86\xD8\xB3\xD8\xAE\xD8\xA9\x20\xD8\xA7\xD9\x84\xD8\xA7\xD8\xAD\xD8\xAA\xD9\x8A\xD8\xA7\xD8\xB7\xD9\x8A\xD8\xA9\x20\xD8\xA8\xD9\x82\xD9\x86\xD9\x88\xD8\xA7\xD8\xAA\xD9\x83\x20\xD8\xA7\xD9\x84\xD8\xAD\xD8\xA7\xD9\x84\xD9\x8A\xD8\xA9\xD8\x9F"); /* هل تريد تحديث النسخة الاحتياطية بقنواتك الحالية؟ */
    body += "<br/><br/>\n";
    body += hcol(H_SW_GREEN, "OK / [1]: ") + hcol(H_VAL, "\xD9\x86\xD8\xB9\xD9\x85\xD8\x8C\x20\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD8\xA7\xD9\x84\xD9\x86\xD8\xB3\xD8\xAE\xD8\xA9\x20\xD8\xA7\xD9\x84\xD8\xA7\xD8\xAD\xD8\xAA\xD9\x8A\xD8\xA7\xD8\xB7\xD9\x8A\xD8\xA9"); /* نعم، تحديث النسخة الاحتياطية */
    body += "<br/>\n";
    body += hcol(H_SUB, "EXIT: ") + hcol(H_VAL, "\xD8\xA5\xD9\x84\xD8\xBA\xD8\xA7\xD8\xA1\x20\xD9\x88\xD8\xA7\xD9\x84\xD8\xB9\xD9\x88\xD8\xAF\xD8\xA9\x20\xD9\x84\xD9\x84\xD9\x82\xD8\xA7\xD8\xA6\xD9\x85\xD8\xA9"); /* إلغاء والعودة للقائمة */
    body += "<br/>\n";
    body += hcol(H_DIV, M_DIV_LINE);
    body += "<br/>\n";
    body += hcol(H_HINT, M_BACKUP_CONFIRM_HINT);
    body += "<br/>\n";
    grabRemote();
    writeFile(HUD_TXT, body, true);
}

/* ---------- Apply a channel-variant database ---------- */
/* ============================================================================
 * applyChannels - atomic channel-set switch followed by a clean reboot.
 *
 * Design decision (explicit user requirement): NO hot swap.
 * Nothing is stopped, killed, restarted or resumed here - not f_server, not the
 * Launcher, not STBSetting, no onPause/onResume tricks. The database is replaced
 * atomically with rename() while the kernel may still hold the old inode open;
 * that is safe because rename() only swaps the directory entry, and the new
 * contents are parsed from scratch during the following boot.
 *
 * Order: gates -> already-in-use check -> copy + fsync + rename (atomic)
 *        -> unlink -journal/-wal/-shm -> restorecon -> fsync(dir)
 *        -> save CUR_CHANNELS -> 3 s countdown -> sync + reboot
 * ========================================================================== */
static void applyChannels(const char* file, const char* name) {
    std::string curFile(file);
    std::string curName(name);
    const std::string startName = readCurrentChannels();   /* set in use before this call */
    char numBuf[16];

    for (;;) {
        char src[512];
        snprintf(src, sizeof(src), "%s/%s", DATA_DIR, curFile.c_str());

        if (isUpdateLocked()) {
            dbg("[channels] REFUSED: update in progress (lock active)");
            return;
        }

        /* ---- gate 1: the variant must exist and have a plausible size ---- */
        long long srcSize = fileSizeOf(src);
        if (srcSize < 0 || srcSize < CHANNEL_DB_MIN_BYTES) {
            dbg("[channels] REFUSED: %s missing or too small (size=%lld)", src, srcSize);
            writeFile(HUD_TXT, hcol(H_ERR, std::string(S_ERR) + curFile + S_ERR2));
            sleep(ERROR_DELAY_SEC);
            return;
        }

        /* ---- gate 2: sqlite3 PRAGMA integrity_check must say ok ---- */
        std::string why;
        if (!sqliteCheckOk(src, &why)) {
            dbg("[channels] REFUSED: %s failed integrity_check (%s)", src, why.c_str());
            writeFile(HUD_TXT, hcol(H_ERR, std::string(S_ERR) + M_CH_BADDB_AR));
            sleep(ERROR_DELAY_SEC);
            return;
        }

        /* ---- gate 3: read the live database's own mode/owner (do not guess) ---- */
        struct stat stLive;
        if (stat(SERVICE_DB, &stLive) != 0) {
            dbg("[channels] REFUSED: %s missing errno=%d", SERVICE_DB, errno);
            writeFile(HUD_TXT, hcol(H_ERR, std::string(S_ERR) + M_CH_FAIL));
            sleep(ERROR_DELAY_SEC);
            return;
        }

        /* ---- gate 4: never wipe a scan the receiver did after our restore point ---- */
        if (strcmp(curFile.c_str(), VARF_ORIGINAL) == 0) {
            long live = -1, seed = -1;
            if (sqliteCount(SERVICE_DB, &live) && sqliteCount(VAR_ORIGINAL, &seed)) {
                if (live > seed) {
                    dbg("[channels] REFUSED: holds %ld items but restore point has %ld - run rescan first",
                        live, seed);
                    writeFile(HUD_TXT, hcol(H_ERR, M_CH_NEED_RESCAN), true);
                    sleep(ERROR_DELAY_SEC + 1);
                    return;
                }
                dbg("[channels] restore check: live=%ld seed=%ld -> allowed", live, seed);
            } else {
                dbg("[channels] restore check skipped: cannot count live/seed (proceeding)");
            }
        }

        /* ---- already using this set: say so and reboot nothing ---- */
        if (startName == curName) {
            dbg("[channels] '%s' is already the active set -> no swap, no reboot", curName.c_str());
            std::string body;
            body += hcol(H_WARN, M_CH_ALREADY_AR); body += "<br/>\n";
            body += hcol(H_HINT, M_CH_ALREADY_EN); body += "<br/>\n";
            writeFile(HUD_TXT, body, true);
            sleep(SUCCESS_DELAY_SEC);
            hideMenu();
            return;
        }

        /* ---- state snapshot for the log only: we deliberately touch nothing ---- */
        std::vector<HiHolder> foreignHolders = scanHiHolders(true);
        if (!foreignHolders.empty()) {
            dbg("[channels] [holders] %zu foreign /dev/hi_* holder(s) present (left running)",
                foreignHolders.size());
            for (const auto& h : foreignHolders) {
                std::string devs;
                for (size_t i = 0; i < h.devices.size(); ++i) {
                    if (i > 0) devs += ", ";
                    devs += h.devices[i];
                }
                dbg("[channels]   holder pid=%d cmd='%s' devs=[%s]",
                    (int)h.pid, h.cmdline.empty() ? h.comm.c_str() : h.cmdline.c_str(), devs.c_str());
            }
        } else {
            dbg("[channels] [holders] no foreign /dev/hi_* holders detected");
        }
        dbg("[channels] [dmesg] DMX key/buffer errors before swap: %d", countDmesgDmxErrors());

        /* ---- swap the database: copy to SERVICE_TMP (copyFileTo fsyncs), then rename ---- */
        unlink(SERVICE_TMP);
        if (!copyFileTo(src, SERVICE_TMP)) {
            dbg("[channels] copy %s -> %s failed errno=%d", src, SERVICE_TMP, errno);
            writeFile(HUD_TXT, hcol(H_ERR, std::string(S_ERR) + M_CH_FAIL));
            sleep(ERROR_DELAY_SEC);
            return;
        }
        mode_t wantMode = stLive.st_mode & 07777;
        if (chmod(SERVICE_TMP, wantMode) != 0 || chown(SERVICE_TMP, stLive.st_uid, stLive.st_gid) != 0) {
            dbg("[channels] chmod/chown failed errno=%d mode=%o uid=%d gid=%d",
                errno, (unsigned)wantMode, (int)stLive.st_uid, (int)stLive.st_gid);
        }
        if (rename(SERVICE_TMP, SERVICE_DB) != 0) {
            dbg("[channels] rename -> %s failed errno=%d", SERVICE_DB, errno);
            unlink(SERVICE_TMP);
            writeFile(HUD_TXT, hcol(H_ERR, std::string(S_ERR) + M_CH_FAIL));
            sleep(ERROR_DELAY_SEC);
            return;
        }
        dbg("[channels] [replace] atomically swapped %s -> %s (%lld bytes)", src, SERVICE_DB, srcSize);

        /* ---- the -journal/-wal/-shm files belong to the old inode: remove them ---- */
        static const char* const EXTRA_FILES[] = {
            SERVICE_DB_JOURNAL,
            SERVICE_DB_WAL,
            SERVICE_DB_SHM
        };
        for (const char* ef : EXTRA_FILES) {
            if (unlink(ef) == 0) {
                dbg("[channels] [cleanup] deleted obsolete %s", ef);
            } else if (errno != ENOENT) {
                dbg("[channels] [cleanup] unlink %s failed errno=%d", ef, errno);
            }
        }

        /* ---- restore SELinux label and make the rename itself durable ---- */
        if (access("/system/bin/restorecon", X_OK) == 0) {
            int rcon = system(CMD_RESTORECON);
            dbg("[channels] [restorecon] ran restorecon rc=%d", rcon);
        }
        int dirFd = open(SERVICE_DIR, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (dirFd >= 0) {
            fsync(dirFd);
            close(dirFd);
        }
        sync();

        /* ---- post-check: the new file must really be in place and complete ---- */
        long long nowSize = fileSizeOf(SERVICE_DB);
        if (nowSize != srcSize) {
            dbg("[channels] POST-CHECK FAILED: %s is %lld bytes, expected %lld - NOT rebooting",
                SERVICE_DB, nowSize, srcSize);
            writeFile(HUD_TXT, hcol(H_ERR, std::string(S_ERR) + M_POSTFAIL));
            sleep(ERROR_DELAY_SEC);
            return;
        }

        /* ---- remember which set is now active ---- */
        writeFile(CUR_CHANNELS, curName + "\n", true);
        dbg("[channels] applied '%s' (%s)", curName.c_str(), SERVICE_DB);

        /* ---- 3 s countdown: EXIT cancels the reboot, 1-4 re-targets the swap ---- */
        int remaining = CH_REBOOT_COUNT_SEC;
        bool cancelled = false, reselect = false;
        while (remaining > 0 && !cancelled && !reselect) {
            snprintf(numBuf, sizeof(numBuf), "%d", remaining);
            std::string body;
            body += "<big><b>";
            body += hcol(H_BUSY, std::string(M_CH_SWITCHING_AR) + numBuf + M_CH_SECONDS_AR);
            body += "</b></big>";
            writeFile(HUD_TXT, body, true);

            for (int tick = 0; tick < 5; ++tick) {          /* 5 x 200 ms = 1 second */
                if (g_fd < 0) { usleep(200000); continue; }
                struct pollfd pfd;
                pfd.fd = g_fd; pfd.events = POLLIN; pfd.revents = 0;
                if (poll(&pfd, 1, 200) > 0 && (pfd.revents & POLLIN)) {
                    struct input_event ev;
                    if (read(g_fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev) &&
                        ev.type == EV_KEY && ev.value == 1) {
                        if (isExitKey(ev.code)) {
                            dbg("[channels] EXIT during countdown -> reboot cancelled");
                            cancelled = true;
                            break;
                        }
                        int idx = -1;
                        if      (ev.code == KEY_1) idx = 0;
                        else if (ev.code == KEY_2) idx = 1;
                        else if (ev.code == KEY_3) idx = 2;
                        else if (ev.code == KEY_4) idx = 3;
                        if (idx >= 0 && idx < CHANNEL_MENU_COUNT && idx < VARIANTS_N) {
                            dbg("[channels] key %d during countdown -> re-target to '%s'",
                                idx + 1, VARIANTS[idx].label.c_str());
                            curFile = VARIANTS[idx].file;
                            curName = VARIANTS[idx].label;
                            reselect = true;
                            break;
                        }
                    }
                }
            }
            if (!cancelled && !reselect) remaining--;
        }

        if (cancelled) {
            std::string body;
            body += "<big><b>"; body += hcol(H_WARN, M_CH_CANCEL_AR); body += "</b></big>";
            writeFile(HUD_TXT, body, true);
            dbg("[channels] reboot cancelled by user - '%s' stays applied until next restart", curName.c_str());
            sleep(SUCCESS_DELAY_SEC);
            hideMenu();
            return;
        }
        if (reselect) continue;              /* re-run every gate for the new target */

        /* ---- countdown finished: clean reboot, nothing was ever stopped ---- */
        std::string body;
        body += "<big><b>"; body += hcol(H_BUSY, M_CH_REBOOT_NOW_AR); body += "</b></big>";
        writeFile(HUD_TXT, body, true);
        dbg("[channels] countdown finished for '%s' -> sync + reboot", curName.c_str());
        int rc = system("/system/bin/sync; /system/bin/reboot");
        dbg("[channels] reboot command rc=%d", rc);
        sleep(3);
        return;
    }
}
/* ==================================================================== *
 *  Channel-variant builder - the old build_variants.sh, rewritten here  *
 *  so no .sh and no .sql file ever has to live on the device.           *
 *                                                                       *
 *  buildVariants(false)  first run: seed service_original.db + build    *
 *  buildVariants(true)   "rescan": re-seed + rebuild       *
 * ==================================================================== */

/* ---- Dynamic Orca Open Keys: extracted directly from live f_server memory (_tvSvc) ---- */
static std::unordered_set<uint32_t> g_orcaOpenKeys; // key = ((uint32_t)svc_id << 16) | (tp_index & 0xFFFF)

static pid_t findFServerPid() {
    std::vector<pid_t> pids = findPidsByName(true, false);
    return pids.empty() ? -1 : pids[0];
}

static bool fetchFServerOrcaKeys(std::vector<std::pair<uint16_t, uint16_t>>& out_keys) {
    pid_t pid = findFServerPid();
    if (pid <= 0) {
        dbg("[op] f_server process not found in /proc");
        return false;
    }

    char mapspath[128];
    snprintf(mapspath, sizeof(mapspath), "/proc/%d/maps", pid);
    FILE* fp = fopen(mapspath, "r");
    if (!fp) return false;

    uintptr_t base = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "/system/bin/f_server")) {
            sscanf(line, "%x-", &base);
            break;
        }
    }
    fclose(fp);
    if (!base) {
        dbg("[op] could not determine f_server base address");
        return false;
    }

    char mempath[128];
    snprintf(mempath, sizeof(mempath), "/proc/%d/mem", pid);
    int mem_fd = open(mempath, O_RDONLY | O_CLOEXEC);
    if (mem_fd < 0) {
        dbg("[op] open mem failed: errno=%d", mempath, errno);
        return false;
    }

    // _tvSvc global object offset from f_server base (Wegoo=0xe9598, Iron Pro=0xf75f0)
    uintptr_t tvSvc_addr = base + (g_tvSvcOff ? g_tvSvcOff : 0xe9598);
    if (lseek64(mem_fd, (off64_t)tvSvc_addr, SEEK_SET) == (off64_t)-1) {
        close(mem_fd);
        return false;
    }

    uint8_t tvSvc_buf[88];
    if (read(mem_fd, tvSvc_buf, sizeof(tvSvc_buf)) != sizeof(tvSvc_buf)) {
        close(mem_fd);
        return false;
    }

    uint32_t count = *(uint32_t*)(tvSvc_buf + 0x10);
    uint32_t ptr = *(uint32_t*)(tvSvc_buf + 0x54);
    if (count < 1000 || count > 25000 || ptr == 0) {
        dbg("[op] tvSvc count=%u, ptr=0x%x", count, ptr);
        close(mem_fd);
        return false;
    }

    size_t total_bytes = count * 56;
    uint8_t* svcs = (uint8_t*)malloc(total_bytes);
    if (!svcs) {
        close(mem_fd);
        return false;
    }

    lseek64(mem_fd, (off64_t)ptr, SEEK_SET);
    if (read(mem_fd, svcs, total_bytes) != (ssize_t)total_bytes) {
        dbg("[op] read %zu bytes from ptr 0x%x failed", total_bytes, ptr);
        free(svcs);
        close(mem_fd);
        return false;
    }
    close(mem_fd);

    for (uint32_t i = 0; i < count; i++) {
        uint8_t* item = svcs + i * 56;
        uint8_t flag25 = item[0x25];
        if (flag25 & 0x10) { // isDescr == true (Yellow dollar)
            uint16_t sid = *(uint16_t*)(item + 4);
            uint16_t tp = *(uint16_t*)(item + 26);
            out_keys.push_back({sid, tp});
        }
    }
    free(svcs);
    dbg("[op] successfully extracted %zu items from live f_server (PID %d)",
        out_keys.size(), pid);
    return !out_keys.empty();
}

/* Fallback: parse svclist.json (from f_server download) and match caid in service_original.db */
static bool fetchSvclistOrcaKeys(std::vector<std::pair<uint16_t, uint16_t>>& out_keys) {
    const char* paths[] = {
        SVCLIST_JSON_TMP,
        SVCLIST_JSON
    };
    std::string json;
    for (size_t i = 0; i < sizeof(paths)/sizeof(paths[0]); ++i) {
        json = readFileAll(paths[i], 1024 * 1024);
        if (!json.empty()) break;
    }
    if (json.empty()) {
        dbg("[op] svclist.json not found in any path");
        return false;
    }

    std::string insertRules = SQL_CREATE_TEMP_BEGIN;
    size_t pos = 0;
    int ruleCount = 0;
    while ((pos = json.find("\"sid\"", pos)) != std::string::npos) {
        size_t colon = json.find(':', pos);
        if (colon == std::string::npos) break;
        int sid = atoi(json.c_str() + colon + 1);

        size_t caidPos = json.find("\"caid\"", colon);
        if (caidPos == std::string::npos || caidPos - colon > 50) {
            pos = colon + 1;
            continue;
        }
        size_t ccolon = json.find(':', caidPos);
        if (ccolon == std::string::npos) break;
        int caid = atoi(json.c_str() + ccolon + 1);

        char rBuf[64];
        snprintf(rBuf, sizeof(rBuf), SQL_INSERT_RULE_NL, sid, caid);
        insertRules += rBuf;
        ruleCount++;
        pos = ccolon + 1;
    }
    insertRules += "COMMIT;\n";
    if (ruleCount == 0) {
        dbg("[op] parsed 0 rules from svclist.json");
        return false;
    }

    std::string q = "PRAGMA temp_store=MEMORY;\n";
    q += insertRules;
    q += SQL_CREATE_INDEX;
    q += SQL_QUERY_RULES;
    q += "WHERE s.svc_is_cas <> 0 AND instr(',' || s.svc_reserved3, ',' || r.caid || ',') > 0;\n";

    std::string out;
    int rc = runSqliteSql(VAR_ORIGINAL, q, &out);
    if (rc != 0 || out.empty()) {
        dbg("[op] fallback query failed rc=%d", rc);
        return false;
    }

    for (size_t p = 0; p < out.size(); ) {
        size_t e = out.find('\n', p);
        std::string ln = out.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
        p = (e == std::string::npos) ? out.size() : e + 1;
        while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
        if (ln.empty()) continue;
        size_t sep = ln.find('|');
        if (sep != std::string::npos) {
            uint16_t sid = (uint16_t)atoi(ln.substr(0, sep).c_str());
            uint16_t tp = (uint16_t)atoi(ln.substr(sep + 1).c_str());
            out_keys.push_back({sid, tp});
        }
    }
    dbg("[op] fallback matched %zu items via svc rules and svc_reserved3", out_keys.size());
    return !out_keys.empty();
}

// ORCA_KEYS_CACHE is OBF

static bool saveOrcaKeysCache(const std::vector<std::pair<uint16_t, uint16_t>>& keys) {
    if (keys.empty()) return false;
    mkdir(DATA_DIR, 0755);
    FILE* fp = fopen(ORCA_KEYS_CACHE, "wb");
    if (!fp) return false;
    uint32_t count = (uint32_t)keys.size();
    fwrite(&count, sizeof(count), 1, fp);
    fwrite(keys.data(), sizeof(std::pair<uint16_t, uint16_t>), count, fp);
    fclose(fp);
    dbg("[op] saved %u keys to cache", count, ORCA_KEYS_CACHE);
    return true;
}

static bool loadOrcaKeysCache(std::vector<std::pair<uint16_t, uint16_t>>& out_keys) {
    FILE* fp = fopen(ORCA_KEYS_CACHE, "rb");
    if (!fp) return false;
    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, fp) != 1 || count < 1000 || count > 25000) {
        fclose(fp);
        return false;
    }
    out_keys.resize(count);
    if (fread(out_keys.data(), sizeof(std::pair<uint16_t, uint16_t>), count, fp) != count) {
        out_keys.clear();
        fclose(fp);
        return false;
    }
    fclose(fp);
    dbg("[op] loaded %u keys from cache", count, ORCA_KEYS_CACHE);
    return true;
}

static bool getOrcaOpenKeys(std::vector<std::pair<uint16_t, uint16_t>>& out_keys) {
    out_keys.clear();
    // 1. Try live f_server memory (highest authority)
    if (fetchFServerOrcaKeys(out_keys) && out_keys.size() >= SQL_MIN_KEYS) {
        saveOrcaKeysCache(out_keys);
        g_orcaOpenKeys.clear();
        for (const auto& kv : out_keys) {
            g_orcaOpenKeys.insert(((uint32_t)kv.first << 16) | (kv.second & 0xFFFF));
        }
        return true;
    }
    // 2. Try persistent cache previously extracted from f_server memory
    if (loadOrcaKeysCache(out_keys) && out_keys.size() >= SQL_MIN_KEYS) {
        g_orcaOpenKeys.clear();
        for (const auto& kv : out_keys) {
            g_orcaOpenKeys.insert(((uint32_t)kv.first << 16) | (kv.second & 0xFFFF));
        }
        return true;
    }
    dbg("[op] live f_server and cache unavailable; trying svclist.json fallback...");
    if (fetchSvclistOrcaKeys(out_keys) && out_keys.size() >= SQL_MIN_KEYS) {
        g_orcaOpenKeys.clear();
        for (const auto& kv : out_keys) {
            g_orcaOpenKeys.insert(((uint32_t)kv.first << 16) | (kv.second & 0xFFFF));
        }
        return true;
    }
    return false;
}

static std::string orcaInsertSql(const std::vector<std::pair<uint16_t, uint16_t>>& keys) {
    std::string sql;
    sql.reserve(keys.size() * 32 + 256);
    char buf[64];
    for (size_t i = 0; i < keys.size(); ++i) {
        if (i % 500 == 0) {
            if (i > 0) sql += ";\n";
            sql += "INSERT OR REPLACE INTO _o VALUES ";
        } else {
            sql += ",";
        }
        snprintf(buf, sizeof(buf), "(%u,%u)", (unsigned)keys[i].first, (unsigned)keys[i].second);
        sql += buf;
    }
    if (!keys.empty()) sql += ";\n";
    return sql;
}

/* SQL for one variant, built in memory and piped straight into sqlite3. */
static std::string variantSql(const std::string& orcaInsert, const char* delCond) {
    std::string sql;
    sql.reserve(orcaInsert.size() + 2048);
    sql += "PRAGMA journal_mode=MEMORY;\n";
    sql += "PRAGMA synchronous=OFF;\n";
    sql += "PRAGMA temp_store=MEMORY;\n";   /* sort space for the index below */
    sql += "BEGIN;\n";
    sql += "DROP TABLE IF EXISTS _o;\n";
    sql += "CREATE TABLE _o(svc_id INT, svc_tp_index INT);\n";
    if (!orcaInsert.empty()) {
        sql += orcaInsert;
        sql += "CREATE INDEX IF NOT EXISTS idx_o ON _o(svc_id, svc_tp_index);\n";
    }
    sql += "CREATE INDEX IF NOT EXISTS idx_svc_key ON _svcInfo(svc_id, svc_tp_index);\n";
    sql += "DELETE FROM _svcInfo WHERE ";
    sql += delCond;
    sql += ";\n";
    sql += "CREATE INDEX IF NOT EXISTS idx_svc ON _svcInfo(svc_type, id);\n";
    sql += "UPDATE _svcInfo SET svc_idx = (SELECT COUNT(*) FROM _svcInfo x WHERE x.svc_type=0 AND x.id < _svcInfo.id) WHERE svc_type=0;\n";
    sql += "UPDATE _svcInfo SET svc_idx = (SELECT COUNT(*) FROM _svcInfo x WHERE x.svc_type<>0 AND x.id < _svcInfo.id) WHERE svc_type<>0;\n";
    sql += "UPDATE _satInfo SET sat_is_used = 0;\n";
    sql += "UPDATE _satInfo SET sat_is_used = 1 WHERE id IN (SELECT DISTINCT t.tp_sat_index FROM _svcInfo s JOIN _tpInfo t ON s.svc_tp_index=t.id);\n";
    sql += "DROP TABLE IF EXISTS _o;\n";
    sql += "COMMIT;\n";
    sql += "VACUUM;\n";
    sql += "PRAGMA integrity_check;\n";
    return sql;
}
/* true when any variant or the counts file is missing/undersized */
static bool variantsMissing() {
    if (fileSizeOf(VAR_COUNTS) <= 0) return true;
    for (int i = 0; i < VARIANTS_N; ++i) {
        char p[512];
        snprintf(p, sizeof(p), "%s/%s", DATA_DIR, VARIANTS[i].file.c_str());
        if (fileSizeOf(p) < CHANNEL_DB_MIN_BYTES) return true;
    }
    return false;
}

/* service_original.db is the restore point.  It is seeded from service.db on
 * the first run and only refreshed by "rescan". */
static bool ensureOriginal(bool refresh) {
    if (!refresh && fileSizeOf(VAR_ORIGINAL) >= CHANNEL_DB_MIN_BYTES) return true;
    long long live = fileSizeOf(SERVICE_DB);
    if (live < CHANNEL_DB_MIN_BYTES) {
        dbg("[bd] %s missing or too small (%lld) -> cannot seed %s",
            SERVICE_DB, live, VAR_ORIGINAL);
        return false;
    }
    long countLive = -1, countOrig = -1;
    sqliteCount(SERVICE_DB, &countLive);
    sqliteCount(VAR_ORIGINAL, &countOrig);
    if (refresh && countOrig > 3000 && countLive > 0 && countLive < countOrig) {
        dbg("[bd] preserving %s (%ld items) because live %s has %ld items",
            VAR_ORIGINAL, countOrig, SERVICE_DB, countLive);
        return true;
    }
    unlink(VAR_BUILD_TMP);
    if (!copyFileTo(SERVICE_DB, VAR_BUILD_TMP)) {
        dbg("[bd] copy %s -> %s failed errno=%d", SERVICE_DB, VAR_BUILD_TMP, errno);
        return false;
    }
    chmod(VAR_BUILD_TMP, 0644);
    if (rename(VAR_BUILD_TMP, VAR_ORIGINAL) != 0) {
        dbg("[bd] rename -> %s failed errno=%d", VAR_ORIGINAL, errno);
        unlink(VAR_BUILD_TMP);
        return false;
    }
    dbg("[bd] %s %s from %s (%lld bytes)", VARF_ORIGINAL,
        refresh ? "refreshed" : "created", SERVICE_DB, live);
    return true;
}

/* Build every variant from service_original.db, always.  Each copy is made in
 * VAR_BUILD_TMP and renamed, so an interrupted run never leaves a half-built
 * file with the final name. */
static bool buildVariants(bool rescan) {
    if (isUpdateLocked()) {
        dbg("[bd] REFUSED: update in progress (lock active)");
        return false;
    }
    mkdir(DATA_DIR, 0755);                    /* ignore EEXIST */

    if (!ensureOriginal(rescan)) return false;

    std::vector<std::pair<uint16_t, uint16_t>> orcaKeys;
    bool hasOrca = getOrcaOpenKeys(orcaKeys) && (orcaKeys.size() > 0);
    std::string insertSql;
    if (hasOrca) {
        insertSql = orcaInsertSql(orcaKeys);
        dbg("[bd] using %zu active keys", orcaKeys.size());
    } else {
        dbg("[bd] WARN: no active keys available; building base variants");
    }

    /* a partially written counts file must trigger a rebuild next time */
    unlink(VAR_COUNTS);

    std::string counts;
    for (int i = 0; i < VARIANTS_N; ++i) {
        const ChannelVariant* v = &VARIANTS[i];
        char dst[512];
        snprintf(dst, sizeof(dst), "%s/%s", DATA_DIR, v->file.c_str());

        unlink(VAR_BUILD_TMP);
        if (!copyFileTo(VAR_ORIGINAL, VAR_BUILD_TMP)) {
            dbg("[bd] %s: copy of %s failed errno=%d", v->file.c_str(), VAR_ORIGINAL, errno);
            unlink(VAR_BUILD_TMP);
            return false;
        }
        chmod(VAR_BUILD_TMP, 0644);

        if (v->delCond != nullptr) {
            if (strstr(v->delCond, "_o") != nullptr && !hasOrca) {
                dbg("[bd] %s: skipped (no active keys)", v->file.c_str());
                unlink(VAR_BUILD_TMP);
                continue;
            }
            std::string out;
            int rc = runSqliteSql(VAR_BUILD_TMP, variantSql(insertSql, v->delCond), &out);
            if (rc != 0 || !outputSaysOk(out)) {
                std::string one = out;
                for (size_t k = 0; k < one.size() && k < 120; ++k)
                    if (one[k] == '\n' || one[k] == '\r') one[k] = ' ';
                if (one.size() > 120) one.resize(120);
                dbg("[bd] %s: SQL failed rc=%d out='%s'", v->file.c_str(), rc, one.c_str());
                unlink(VAR_BUILD_TMP);
                continue;
            }
        }

        long count = 0;
        if (!sqliteCount(VAR_BUILD_TMP, &count)) {
            dbg("[bd] %s: cannot count items", v->file.c_str());
            unlink(VAR_BUILD_TMP);
            return false;
        }

        if (rename(VAR_BUILD_TMP, dst) != 0) {
            dbg("[bd] rename -> %s failed errno=%d", dst, errno);
            unlink(VAR_BUILD_TMP);
            return false;
        }

        char line[256];
        snprintf(line, sizeof(line), "%s=%ld\n", v->file.c_str(), count);
        counts += line;
        writeFile(VAR_COUNTS, counts, true);
        dbg("[bd] %s -> %ld items", v->file.c_str(), count);
    }
    dbg("[bd] %d variants ready%s", VARIANTS_N, rescan ? " (rescan)" : "");
    return true;
}

/* Called once at startup: rebuilds anything that is missing and shows
 * "جاري تجهيز القنوات..." while it runs. */
static void ensureChannelVariants() {
    ensureUserChannelsBackup(false);
    if (!variantsMissing()) return;
    dbg("[bd] variant data missing -> building now");
    writeFile(HUD_TXT, hcol(H_BUSY, std::string(M_CH_BUSY) + S_DOT), true);
    bool ok = buildVariants(false);
    hideMenu();                               /* drop the busy message */
    dbg("[bd] %s", ok ? "variants ready" : "variant build FAILED");
}

/* ---------- Startup report: DATA_DIR/report.txt ----------
 * Written at every start (right after ensureChannelVariants) and after a
 * rescan, so counts, integrity, service.db state and the process pictures
 * can be pulled over FTP - no shell is needed for the test step. */

static bool endsWithPath(const char* s, const char* suffix) {
    if (!s || !suffix) return false;
    size_t ls = strlen(s), lf = strlen(suffix);
    if (lf > ls) return false;
    return strcmp(s + (ls - lf), suffix) == 0;
}

static std::string procCmdlineOf(const char* pidName) {
    char p[64];
    snprintf(p, sizeof(p), "/proc/%s/cmdline", pidName);
    std::string cl = readFile(p, 1024);
    for (size_t i = 0; i < cl.size(); ++i) if (cl[i] == '\0') cl[i] = ' ';
    while (!cl.empty() && cl[cl.size() - 1] == ' ') cl.erase(cl.size() - 1);
    if (cl.empty()) cl = "(none)";
    return cl;
}

static std::string oneLine(std::string s, size_t max) {
    for (size_t i = 0; i < s.size(); ++i) if (s[i] == '\n' || s[i] == '\r') s[i] = ' ';
    while (!s.empty() && s[0] == ' ') s.erase(0, 1);
    if (s.size() > max) s.resize(max);
    return s;
}

static bool allDigits(const std::string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (!isdigit((unsigned char)s[i])) return false;
    return true;
}

/* First line = PRAGMA integrity_check result, first numeric line = COUNT(*) */
static void parseFacts(const std::string& out, int rc, long* count,
                       std::string* integrity, std::string* err) {
    *count = -1;
    integrity->clear();
    err->clear();
    for (size_t p = 0; p < out.size(); ) {
        size_t e = out.find('\n', p);
        std::string line = out.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
        while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == ' '))
            line.erase(line.size() - 1);
        if (!line.empty() && !allDigits(line) &&
            line.compare(0, 2, "--") != 0 && line.find("warning") == std::string::npos) {
            /* sqlite3 chatter (warnings, notices) is skipped; the first real
             * answer of integrity_check - "ok" or the error report - wins. */
            if (integrity->empty() || line == "ok") *integrity = line;
        }
        if (allDigits(line) && *count < 0) *count = strtol(line.c_str(), nullptr, 10);
        if (e == std::string::npos) break;
        p = e + 1;
    }
    if (integrity->empty()) *integrity = (rc == 0) ? "unknown" : "sqlite3 failed";
    if (*count < 0) *err = oneLine(out.empty() ? std::string("no output") : out, 200);
}

static bool dbFacts(const char* path, long* count, std::string* integrity, std::string* err) {
    *count = -1; integrity->clear(); err->clear();
    if (fileSizeOf(path) < 0) { *err = "file missing"; return false; }
    std::string out;
    int rc = runSqliteSql(path, "PRAGMA integrity_check;\nSELECT COUNT(*) FROM _svcInfo;\n", &out);
    parseFacts(out, rc, count, integrity, err);
    if (*count < 0) return false;
    return true;
}

/* Re-derives what a variant must contain: the old builder did
 * "DELETE FROM _svcInfo WHERE cond", so what survives is "cond is not true"
 * (rows where cond is NULL survive a DELETE as well -> "IS NOT 1").
 * Counting that on a throw-away copy of service_original.db gives the same
 * number as the built file without replaying DELETE/VACUUM (seconds faster),
 * and the copy keeps service_original.db read-only no matter what. */
static bool expectedCount(const ChannelVariant* v, long* count,
                          std::string* integrity, std::string* err) {
    *count = -1; integrity->clear(); err->clear();
    if (fileSizeOf(VAR_ORIGINAL) < 0) { *err = "original missing"; return false; }
    if (v->delCond == nullptr)
        return dbFacts(VAR_ORIGINAL, count, integrity, err);

    unlink(CHECK_TMP);
    if (!copyFileTo(VAR_ORIGINAL, CHECK_TMP)) {
        *err = "copy of original failed";
        unlink(CHECK_TMP);
        return false;
    }
    chmod(CHECK_TMP, 0644);
    std::string sql;
    sql += "CREATE TEMP TABLE _o(svc_id INT, svc_tp_index INT);\n";
    std::vector<std::pair<uint16_t, uint16_t>> dKeys;
    if (getOrcaOpenKeys(dKeys)) {
        sql += orcaInsertSql(dKeys);
    }
    sql += "CREATE INDEX IF NOT EXISTS _idx_o ON _o(svc_id, svc_tp_index);\n";
    sql += "SELECT COUNT(*) FROM _svcInfo WHERE (";
    sql += v->delCond;
    sql += ") IS NOT 1;\n";
    std::string out;
    int rc = runSqliteSql(CHECK_TMP, sql, &out);
    parseFacts(out, rc, count, integrity, err);
    unlink(CHECK_TMP);
    if (*count < 0) return false;
    return true;
}

/* Every process that currently has /data/db/service.db open, as
 * "pid=.. fd=.. -> path" + its cmdline.  Used by the report and, right after
 * the kill attempt, by applyChannels() so the log shows who kept the file. */
static void scanServiceDbHolders(std::string* out) {
    out->clear();
    DIR* d = opendir("/proc");
    if (!d) return;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR) continue;
        const char* name = de->d_name;
        if (!isdigit((unsigned char)name[0])) continue;
        pid_t p = (pid_t)atoi(name);
        if (p <= 0 || p == getpid()) continue;

        char fdDir[64];
        snprintf(fdDir, sizeof(fdDir), "/proc/%s/fd", name);
        DIR* fd = opendir(fdDir);
        if (!fd) continue;
        struct dirent* fe;
        while ((fe = readdir(fd)) != nullptr) {
            if (fe->d_name[0] == '.') continue;
            char lp[96];
            snprintf(lp, sizeof(lp), "%s/%s", fdDir, fe->d_name);
            char tgt[256];
            ssize_t n = readlink(lp, tgt, sizeof(tgt) - 1);
            if (n <= 0) continue;
            tgt[n] = '\0';
            if (endsWithPath(tgt, SLASH_SERVICE_DB)) {
                char b[768];
                snprintf(b, sizeof(b), "pid=%d fd=%s -> %s\n      cmd=%s\n",
                         (int)p, fe->d_name, tgt, procCmdlineOf(name).c_str());
                out->append(b);
            }
        }
        closedir(fd);
    }
    closedir(d);
}

/* ---------- /dev/hi_* Holder Detection & Reporting ---------- */
static std::vector<HiHolder> scanHiHolders(bool foreignOnly) {
    std::vector<HiHolder> results;
    DIR* procDir = opendir("/proc");
    if (!procDir) return results;

    pid_t zygotePid = -1;
    struct dirent* de;
    while ((de = readdir(procDir)) != nullptr) {
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        char commPath[64];
        snprintf(commPath, sizeof(commPath), "/proc/%s/comm", de->d_name);
        std::string comm = readFile(commPath, 64);
        while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
        if (comm == "zygote" || comm == "zygote64") {
            zygotePid = (pid_t)atoi(de->d_name);
            break;
        }
    }
    rewinddir(procDir);

    while ((de = readdir(procDir)) != nullptr) {
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        pid_t p = (pid_t)atoi(de->d_name);
        if (p <= 0) continue;

        char fdDirPath[64];
        snprintf(fdDirPath, sizeof(fdDirPath), "/proc/%s/fd", de->d_name);
        DIR* fdDir = opendir(fdDirPath);
        if (!fdDir) continue;

        std::set<std::string> devSet;
        struct dirent* fde;
        while ((fde = readdir(fdDir)) != nullptr) {
            if (fde->d_name[0] == '.') continue;
            char lpath[128];
            snprintf(lpath, sizeof(lpath), "%s/%s", fdDirPath, fde->d_name);
            char tgt[256];
            ssize_t n = readlink(lpath, tgt, sizeof(tgt) - 1);
            if (n <= 0) continue;
            tgt[n] = '\0';
            if (strncmp(tgt, "/dev/hi_", 8) == 0 ||
                strncmp(tgt, "/dev/mmz", 8) == 0 ||
                strncmp(tgt, "/dev/vinput", 11) == 0) {
                devSet.insert(tgt);
            }
        }
        closedir(fdDir);

        if (devSet.empty()) continue;

        HiHolder h;
        h.pid = p;
        for (const auto& d : devSet) h.devices.push_back(d);

        char commPath[64];
        snprintf(commPath, sizeof(commPath), "/proc/%s/comm", de->d_name);
        h.comm = readFile(commPath, 64);
        while (!h.comm.empty() && (h.comm.back() == '\n' || h.comm.back() == '\r')) h.comm.pop_back();

        h.cmdline = procCmdlineOf(de->d_name);

        h.ppid = 0;
        char statPath[64];
        snprintf(statPath, sizeof(statPath), "/proc/%s/stat", de->d_name);
        std::string statStr = readFile(statPath, 256);
        size_t closeParen = statStr.rfind(')');
        if (closeParen != std::string::npos && closeParen + 2 < statStr.size()) {
            h.ppid = (pid_t)atoi(statStr.c_str() + closeParen + 2);
        }

        h.isFServer = (h.comm == "f_server" || h.cmdline.find("f_server") != std::string::npos);

        bool knownSysName = (h.comm == "hiavplayer" || h.comm == TEST_HIAOSVC ||
                             h.comm == "displaysetting" || h.comm == "pqsetting" ||
                             h.comm == "hisysmanager" || h.comm == "surfaceflinger" ||
                             h.comm == "mediaserver" || h.comm == "audioserver" ||
                             h.comm == "zygote" || h.comm == "system_server" ||
                             h.comm == "android_ir_user" || h.comm == "dlnaserver");

        bool underZygote = (zygotePid > 0 && h.ppid == zygotePid);
        bool androidPkg = (h.cmdline.find("com.hisilicon.") != std::string::npos ||
                           h.cmdline.find("com.android.") != std::string::npos);

        h.isSystem = h.isFServer || knownSysName || underZygote || androidPkg;

        if (foreignOnly && h.isSystem) continue;
        results.push_back(h);
    }
    closedir(procDir);

    std::sort(results.begin(), results.end(), [](const HiHolder& a, const HiHolder& b) {
        int rA = a.isFServer ? 0 : (a.isSystem ? 1 : 2);
        int rB = b.isFServer ? 0 : (b.isSystem ? 1 : 2);
        if (rA != rB) return rA < rB;
        return a.pid < b.pid;
    });

    return results;
}

static std::string formatHiHoldersTable(const std::vector<HiHolder>& holders) {
    std::string out;
    char buf[1024];
    out += "=== /dev/hi_* File Descriptor Holders ===\n";
    snprintf(buf, sizeof(buf), "%-12s %-7s %-32s %s\n", "TYPE", "PID", "COMMAND", "DEVICES");
    out += buf;
    out += "--------------------------------------------------------------------------------\n";
    int nFs = 0, nSys = 0, nFor = 0;
    for (const auto& h : holders) {
        std::string typeStr;
        if (h.isFServer) { typeStr = "[F_SERVER]"; nFs++; }
        else if (h.isSystem) { typeStr = "[SYSTEM]"; nSys++; }
        else { typeStr = "[FOREIGN!]"; nFor++; }

        std::string devs;
        for (size_t i = 0; i < h.devices.size(); ++i) {
            if (i > 0) devs += ", ";
            devs += h.devices[i];
        }

        std::string cmd = h.cmdline.empty() ? h.comm : h.cmdline;
        if (cmd.length() > 32) cmd = cmd.substr(0, 29) + "...";

        snprintf(buf, sizeof(buf), "%-12s %-7d %-32s %s\n",
                 typeStr.c_str(), (int)h.pid, cmd.c_str(), devs.c_str());
        out += buf;
    }
    out += "--------------------------------------------------------------------------------\n";
    snprintf(buf, sizeof(buf), "Total: %zu process(es) holding /dev/hi_* (%d f_server, %d system, %d foreign)\n",
             holders.size(), nFs, nSys, nFor);
    out += buf;
    return out;
}

static int countDmesgDmxErrors() {
    FILE* fp = popen("/system/bin/dmesg", "r");
    if (!fp) return 0;
    char buf[512];
    int count = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, "DmxKeyAcquire") ||
            strstr(buf, "DMX_OsiTsBufferCreate") ||
            strstr(buf, "There is no available key") ||
            strstr(buf, "used by other process")) {
            count++;
        }
    }
    pclose(fp);
    return count;
}

static void appendProcPics(std::string* r) {
    /* 1. f_server detailed inspection */
    r->append("\n--- f_server process details ---\n");
    std::vector<pid_t> fsPids = findPidsByName(true, false);
    if (fsPids.empty()) {
        r->append("none running\n");
    } else {
        for (pid_t p : fsPids) {
            char pStr[32];
            snprintf(pStr, sizeof(pStr), "%d", (int)p);
            FServerProcState st;
            if (recordFServerProcState(pStr, &st)) {
                char b[1024];
                snprintf(b, sizeof(b),
                    "pid: %d\n"
                    "ppid: %d (init parent: %s)\n"
                    "uid: %d  gid: %d\n"
                    "exe: %s\n"
                    "cwd: %s\n"
                    "selinux_attr: %s\n"
                    "cmdline: %s\n",
                    st.pid, st.ppid, (st.ppid == 1 ? "YES" : "NO"),
                    (int)st.uid, (int)st.gid,
                    st.exe.c_str(), st.cwd.c_str(),
                    st.selinuxAttr.c_str(), st.cmdline.c_str());
                r->append(b);
            }
        }
    }

    /* 2. Open files for f_server and launcher */
    r->append("\n--- Open files ---\n");
    std::vector<pid_t> allPids = fsPids;
    std::vector<pid_t> lnPids = findPidsByName(false, true);
    allPids.insert(allPids.end(), lnPids.begin(), lnPids.end());
    if (allPids.empty()) {
        r->append("none running\n");
    } else {
        for (pid_t p : allPids) {
            char fdDir[64];
            snprintf(fdDir, sizeof(fdDir), "/proc/%d/fd", (int)p);
            DIR* d = opendir(fdDir);
            if (!d) continue;
            char pStr[32]; snprintf(pStr, sizeof(pStr), "%d", (int)p);
            std::string cl = procCmdlineOf(pStr);
            char hdr[256];
            snprintf(hdr, sizeof(hdr), "pid=%d cmd=%s\n", (int)p, cl.c_str());
            r->append(hdr);
            struct dirent* de;
            int foundFd = 0;
            while ((de = readdir(d)) != nullptr) {
                if (de->d_name[0] == '.') continue;
                char linkPath[128];
                snprintf(linkPath, sizeof(linkPath), "%s/%s", fdDir, de->d_name);
                char target[512];
                ssize_t n = readlink(linkPath, target, sizeof(target) - 1);
                if (n > 0) {
                    target[n] = '\0';
                    std::string tgt(target);
                    if (tgt.find(LOCAL_TMP_DIR) != std::string::npos ||
                        tgt.find("db") != std::string::npos ||
                        tgt.find("svc") != std::string::npos) {
                        char fb[600];
                        snprintf(fb, sizeof(fb), "  fd %s -> %s\n", de->d_name, target);
                        r->append(fb);
                        foundFd++;
                    }
                }
            }
            closedir(d);
            if (foundFd == 0) r->append("  (no matching open files)\n");
        }
    }

    /* 3. Directory listing of /data/db */
    r->append("\n--- DB dir listing ---\n");
    DIR* dbDir = opendir(SERVICE_DIR);
    if (!dbDir) {
        r->append("cannot open db dir\n");
    } else {
        struct dirent* de;
        while ((de = readdir(dbDir)) != nullptr) {
            if (de->d_name[0] == '.') continue;
            char fp[256];
            snprintf(fp, sizeof(fp), SERVICE_DIR_FMT, de->d_name);
            struct stat st;
            if (stat(fp, &st) == 0) {
                char mts[64];
                struct tm* mt = localtime(&st.st_mtime);
                strftime(mts, sizeof(mts), "%Y-%m-%d %H:%M:%S", mt);
                char lb[512];
                snprintf(lb, sizeof(lb), "%-32s size=%-9lld mode=%04o mtime=%s\n",
                         de->d_name, (long long)st.st_size, (unsigned)(st.st_mode & 07777), mts);
                r->append(lb);
            }
        }
        closedir(dbDir);
    }

    /* 4. Boot config lines matching 'f_server' */
    r->append("\n--- boot config lines matching 'f_server' (/init*.rc, /system/etc/init/*, /system/etc/*.rc) ---\n");
    DIR* rootDir = opendir("/");
    if (rootDir) {
        struct dirent* de;
        while ((de = readdir(rootDir)) != nullptr) {
            if (strncmp(de->d_name, "init", 4) == 0 && strstr(de->d_name, ".rc")) {
                char rcp[256];
                snprintf(rcp, sizeof(rcp), "/%s", de->d_name);
                FILE* fp = fopen(rcp, "r");
                if (fp) {
                    char line[512];
                    int lnum = 0;
                    while (fgets(line, sizeof(line), fp)) {
                        lnum++;
                        if (strstr(line, "f_server")) {
                            char mb[600];
                            snprintf(mb, sizeof(mb), "%s:%d: %s", rcp, lnum, line);
                            r->append(mb);
                        }
                    }
                    fclose(fp);
                }
            }
        }
        closedir(rootDir);
    }
    DIR* etcInit = opendir("/system/etc/init");
    if (etcInit) {
        struct dirent* de;
        while ((de = readdir(etcInit)) != nullptr) {
            if (strstr(de->d_name, ".rc")) {
                char rcp[256];
                snprintf(rcp, sizeof(rcp), "/system/etc/init/%s", de->d_name);
                FILE* fp = fopen(rcp, "r");
                if (fp) {
                    char line[512];
                    int lnum = 0;
                    while (fgets(line, sizeof(line), fp)) {
                        lnum++;
                        if (strstr(line, "f_server")) {
                            char mb[600];
                            snprintf(mb, sizeof(mb), "%s:%d: %s", rcp, lnum, line);
                            r->append(mb);
                        }
                    }
                    fclose(fp);
                }
            }
        }
        closedir(etcInit);
    }
    DIR* sysEtc = opendir("/system/etc");
    if (sysEtc) {
        struct dirent* de;
        while ((de = readdir(sysEtc)) != nullptr) {
            if (strstr(de->d_name, ".rc")) {
                char rcp[256];
                snprintf(rcp, sizeof(rcp), "/system/etc/%s", de->d_name);
                FILE* fp = fopen(rcp, "r");
                if (fp) {
                    char line[512];
                    int lnum = 0;
                    while (fgets(line, sizeof(line), fp)) {
                        lnum++;
                        if (strstr(line, "f_server")) {
                            char mb[600];
                            snprintf(mb, sizeof(mb), "%s:%d: %s", rcp, lnum, line);
                            r->append(mb);
                        }
                    }
                    fclose(fp);
                }
            }
        }
        closedir(sysEtc);
    }

    /* 5. Launcher cache files in /data/data/ */
    r->append("\n--- Cache files ---\n");
    static const char* const PKGS[] = {
        LAUNCHER_DVB_DIR,
        LAUNCHER_HISI_DIR
    };
    int foundCache = 0;
    for (const char* pkg : PKGS) {
        DIR* pd = opendir(pkg);
        if (!pd) continue;
        struct dirent* pde;
        while ((pde = readdir(pd)) != nullptr) {
            if (pde->d_name[0] == '.') continue;
            char sub[256];
            snprintf(sub, sizeof(sub), "%s/%s", pkg, pde->d_name);
            DIR* sd = opendir(sub);
            if (sd) {
                struct dirent* sde;
                while ((sde = readdir(sd)) != nullptr) {
                    if (sde->d_name[0] == '.') continue;
                    std::string fn = sde->d_name;
                    for (char& c : fn) c = (char)tolower((unsigned char)c);
                    if (fn.find("ch") != std::string::npos ||
                        fn.find("svc") != std::string::npos ||
                        fn.find("epg") != std::string::npos) {
                        char cb[512];
                        snprintf(cb, sizeof(cb), "  %s/%s\n", sub, sde->d_name);
                        r->append(cb);
                        foundCache++;
                    }
                }
                closedir(sd);
            }
            std::string fn = pde->d_name;
            for (char& c : fn) c = (char)tolower((unsigned char)c);
            if (fn.find("ch") != std::string::npos ||
                fn.find("svc") != std::string::npos ||
                fn.find("epg") != std::string::npos) {
                char cb[512];
                snprintf(cb, sizeof(cb), "  %s\n", sub);
                r->append(cb);
                foundCache++;
            }
        }
        closedir(pd);
    }
    if (foundCache == 0) r->append("none found\n");
}

/* ---------------- orca-open diagnosis (read-only, data only) -------------
 * Question: why does service_encrypted_orca_open.db contain channels the orca
 * server does not really open (white $ instead of yellow $)?
 * Suspected cause: the key is only (svc_id, tp_frequency), so a channel on
 * another satellite / polarization / symbol rate that happens to repeat the
 * same pair could be picked up instead of the observed one.
 * This block only REPORTS evidence.  It never touches variantSql(),
 * ORCA_KEYS[] or VARIANTS[], and it writes nothing: only SELECT / PRAGMA /
 * CREATE TEMP TABLE run against service_original.db. */
#define DIAG_MAX_NAMES 40

/* '|' separated field split, returns how many fields were filled */
static int splitFields(const std::string& s, char sep, std::string* f, int maxf) {
    int n = 0;
    size_t p = 0;
    while (n < maxf) {
        size_t e = s.find(sep, p);
        if (e == std::string::npos) { f[n++] = s.substr(p); break; }
        f[n++] = s.substr(p, e - p);
        p = e + 1;
    }
    return n;
}

/* drop control characters and the field separator, cap the length */
static std::string cleanField(std::string s, size_t maxLen) {
    std::string o;
    for (size_t i = 0; i < s.size() && o.size() < maxLen; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20 || c == '|') continue;
        o += (char)c;
    }
    return o;
}

static const char* polName(const std::string& v) {
    if (v == "0") return "V";
    if (v == "1") return "H";
    return "?";
}

/* Channel names to look up: DIAG_CHANNELS (one per line, '#' = comment),
 * otherwise the built-in list of the channels the box owner reported. */
static int loadDiagNames(std::string* names, int maxN) {
    int n = 0;
    std::string txt = readFileAll(DIAG_CHANNELS, 8192);
    for (size_t p = 0; p < txt.size() && n < maxN; ) {
        size_t e = txt.find('\n', p);
        std::string ln = txt.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
        while (!ln.empty() && (ln[ln.size() - 1] == '\r' || ln[ln.size() - 1] == ' ' ||
                               ln[ln.size() - 1] == '\t')) ln.erase(ln.size() - 1);
        size_t st = 0;
        while (st < ln.size() && (ln[st] == ' ' || ln[st] == '\t')) ++st;
        ln.erase(0, st);
        if (!ln.empty() && ln[0] != '#') names[n++] = ln;
        if (e == std::string::npos) break;
        p = e + 1;
    }
    if (n == 0) {
        static const char* def[] = {
            TEST_DISNEY, "20mediaset hd", "radio monte carlo",
            "virgin radio", "discovery turbo italy", "canal+ extra 3",
            "canal+ premium hd"
        };
        for (size_t i = 0; i < sizeof(def) / sizeof(def[0]) && n < maxN; ++i)
            names[n++] = def[i];
    }
    return n;
}

/* SQL boolean over lower(s.svc_name); quotes and SQL wildcards removed */
static std::string diagNameFilter(const std::string* names, int nn) {
    std::string w;
    for (int i = 0; i < nn; ++i) {
        std::string clean;
        for (size_t k = 0; k < names[i].size(); ++k) {
            char c = names[i][k];
            if (c == '\'' || c == '%' || c == '_' || c == '\\') continue;
            if ((unsigned char)c < 0x20) continue;
            clean += c;
        }
        if (clean.empty()) continue;
        if (!w.empty()) w += " OR ";
        w += "lower(s.svc_name) LIKE '%";
        w += clean;
        w += "%'";
    }
    if (w.empty()) w = "0";
    return w;
}

static bool orcaKeyHas(unsigned svc, unsigned tp) {
    return g_orcaOpenKeys.find(((uint32_t)svc << 16) | (tp & 0xFFFF)) != g_orcaOpenKeys.end();
}

static void appendOrcaDiag(std::string* r, std::string* errs) {
    r->append("\n--- Rules diagnosis ---\n");
    if (fileSizeOf(VAR_ORIGINAL) < CHANNEL_DB_MIN_BYTES) {
        r->append("original missing or small -> skipped\n");
        return;
    }

    char line[1024];
    std::string names[DIAG_MAX_NAMES];
    int nn = loadDiagNames(names, DIAG_MAX_NAMES);
    bool fromFile = (fileSizeOf(DIAG_CHANNELS) > 0);
    snprintf(line, sizeof(line), "names: %s (%d)\n",
             fromFile ? "from diag file" : "built-in default list", nn);
    r->append(line);
    std::string show;
    for (int i = 0; i < nn; ++i) {
        if (!show.empty()) show += ", ";
        show += cleanField(names[i], 40);
    }
    r->append("key: live f_server _tvSvc memory matching (isDescr == true) with svclist.json CAID fallback\n");
    r->append("legend: tp_polar_qam 0=V 1=H (measured on this box); n_sig>1 = same (svc_id,freq) on >1 physical TP\n");

    std::string filt = diagNameFilter(names, nn);

    std::string sql;
    sql.reserve(65536);
    sql += "PRAGMA temp_store=MEMORY;\n";
    sql += "SELECT '@@SCHEMA@@';\n";
    sql += "SELECT '--- _svcInfo ---';\nPRAGMA table_info(_svcInfo);\n";
    sql += "SELECT '--- _tpInfo ---';\nPRAGMA table_info(_tpInfo);\n";
    sql += "SELECT '--- _satInfo ---';\nPRAGMA table_info(_satInfo);\n";
    sql += "SELECT '@@STATS@@';\n";
    /* all four tables are TEMP: service_original.db is never written to */
    sql += "CREATE TEMP TABLE _o(svc_id INT, svc_tp_index INT);\n";
    std::vector<std::pair<uint16_t, uint16_t>> dKeys;
    if (getOrcaOpenKeys(dKeys)) {
        sql += orcaInsertSql(dKeys);
    }
    sql += "CREATE INDEX _o_i ON _o(svc_id, svc_tp_index);\n";
    sql += "CREATE TEMP TABLE dbk AS SELECT DISTINCT s.svc_id dsvc, t.tp_frequency dfreq "
           "FROM _svcInfo s JOIN _tpInfo t ON s.svc_tp_index=t.id;\n";
    sql += "CREATE INDEX dbk_i ON dbk(dsvc, dfreq);\n";
    sql += "CREATE TEMP TABLE amb AS SELECT s.svc_id ksvc, t.tp_frequency kfreq, "
           "COUNT(DISTINCT s.svc_tp_index) n_tp, "
           "COUNT(DISTINCT t.tp_sat_index||'/'||t.tp_polar_qam||'/'||t.tp_symbol_rate) n_sig "
           "FROM _svcInfo s JOIN _tpInfo t ON s.svc_tp_index=t.id GROUP BY 1,2;\n";
    sql += "CREATE INDEX amb_i ON amb(ksvc,kfreq);\n";
    sql += "SELECT 'keys_total', COUNT(*) FROM _o;\n";
    sql += "SELECT 'keys_with_no_db_row', COUNT(*) FROM _o k "
           "LEFT JOIN dbk d ON d.dsvc=k.svc_id AND d.dfreq=k.tp_freq WHERE d.dsvc IS NULL;\n";
    sql += "SELECT 'db_rows_matching_a_key', COUNT(*) FROM _svcInfo s "
           "JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "JOIN _o k ON k.svc_id=s.svc_id AND k.tp_freq=t.tp_frequency;\n";
    sql += "SELECT 'enc_open_rows_cas_and_match', COUNT(*) FROM _svcInfo s "
           "JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "JOIN _o k ON k.svc_id=s.svc_id AND k.tp_freq=t.tp_frequency WHERE s.svc_is_cas<>0;\n";
    sql += "SELECT 'enc_open_rows_on_ambiguous_key', COUNT(*) FROM _svcInfo s "
           "JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "JOIN _o k ON k.svc_id=s.svc_id AND k.tp_freq=t.tp_frequency "
           "JOIN amb a ON a.ksvc=s.svc_id AND a.kfreq=t.tp_frequency "
           "WHERE s.svc_is_cas<>0 AND a.n_sig>1;\n";
    sql += "SELECT 'db_distinct_keys', COUNT(*) FROM amb;\n";
    sql += "SELECT 'ambiguous_keys_in_db', COUNT(*) FROM amb WHERE n_sig>1;\n";
    sql += "SELECT 'db_rows_on_ambiguous_key', COUNT(*) FROM _svcInfo s "
           "JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "JOIN amb a ON a.ksvc=s.svc_id AND a.kfreq=t.tp_frequency WHERE a.n_sig>1;\n";

    sql += "SELECT '@@NAMED@@';\n";
    sql += "SELECT s.svc_id, REPLACE(REPLACE(REPLACE(s.svc_name, char(5), ''), char(0), ''), '|', '/'), "
           "s.svc_is_cas, t.id, t.tp_frequency, t.tp_polar_qam, t.tp_symbol_rate, t.tp_sat_index, "
           "IFNULL(sat.sat_name, ''), IFNULL(a.n_tp, 0), IFNULL(a.n_sig, 0), "
           "(SELECT COUNT(*) FROM _svcInfo s2 JOIN _tpInfo t2 ON s2.svc_tp_index=t2.id "
           " WHERE s2.svc_id=s.svc_id AND t2.tp_frequency=t.tp_frequency "
           "   AND s2.svc_tp_index<>s.svc_tp_index) "
           "FROM _svcInfo s "
           "JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "LEFT JOIN _satInfo sat ON t.tp_sat_index=sat.id "
           "LEFT JOIN amb a ON a.ksvc=s.svc_id AND a.kfreq=t.tp_frequency WHERE ";
    sql += filt;
    sql += " ORDER BY lower(s.svc_name), s.svc_id, t.id LIMIT 60;\n";

    sql += "SELECT '@@OTHERS@@';\n";
    sql += "CREATE TEMP TABLE nm AS SELECT s.svc_id nsvc, t.tp_frequency nfreq, s.svc_tp_index ntp "
           "FROM _svcInfo s JOIN _tpInfo t ON s.svc_tp_index=t.id WHERE ";
    sql += filt;
    sql += " LIMIT 60;\n";
    sql += "SELECT s.svc_id, t.tp_frequency, "
           "REPLACE(REPLACE(REPLACE(s.svc_name, char(5), ''), char(0), ''), '|', '/'), s.svc_is_cas, "
           "s.svc_tp_index, t.id, t.tp_polar_qam, t.tp_symbol_rate, t.tp_sat_index, "
           "IFNULL(sat.sat_name, '') "
           "FROM _svcInfo s JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "LEFT JOIN _satInfo sat ON t.tp_sat_index=sat.id "
           "WHERE EXISTS (SELECT 1 FROM nm WHERE nm.nsvc=s.svc_id AND nm.nfreq=t.tp_frequency "
           "              AND nm.ntp<>s.svc_tp_index) "
           "ORDER BY s.svc_id, t.tp_frequency, s.svc_tp_index LIMIT 60;\n";

    sql += "SELECT '@@AMBIG30@@';\n";
    sql += "SELECT s.svc_id, REPLACE(REPLACE(REPLACE(s.svc_name, char(5), ''), char(0), ''), '|', '/'), "
           "s.svc_is_cas, t.tp_frequency, t.tp_polar_qam, t.tp_symbol_rate, t.tp_sat_index, "
           "IFNULL(sat.sat_name, ''), a.n_tp, a.n_sig "
           "FROM _svcInfo s JOIN _tpInfo t ON s.svc_tp_index=t.id "
           "JOIN amb a ON a.ksvc=s.svc_id AND a.kfreq=t.tp_frequency "
           "LEFT JOIN _satInfo sat ON t.tp_sat_index=sat.id "
           "WHERE a.n_sig>1 ORDER BY s.svc_id, t.tp_frequency, t.id LIMIT 30;\n";

    sql += "SELECT '@@ENCFILE@@';\n";
    if (fileSizeOf(VAR_ENC_OPEN) >= CHANNEL_DB_MIN_BYTES) {
        sql += std::string("ATTACH '") + VAR_ENC_OPEN + "' AS enc;\n";
        sql += SQL_COUNT_ALL_NL;
        sql += SQL_COUNT_CAS_NL;
    } else {
        sql += SQL_ENC_MISSING_NL;
    }

    std::string out;
    int rc = runSqliteSql(VAR_ORIGINAL, sql, &out);
    if (out.find("@@SCHEMA@@") == std::string::npos) {
        snprintf(line, sizeof(line), "diagnosis query FAILED (rc=%d, see sqlite3 errors)\n", rc);
        r->append(line);
        if (!errs->empty()) *errs += "\n";
        *errs += "rules diag: ";
        *errs += oneLine(out.empty() ? std::string("no output") : out, 300);
        *errs += "\n";
        return;
    }
    if (out.find("Error:") != std::string::npos) {
        if (!errs->empty()) *errs += "\n";
        *errs += "rules diag: ";
        *errs += oneLine(out.substr(out.find("Error:")), 300);
        *errs += "\n";
    }

    int sect = 0;
    long namedN = 0, namedInKeys = 0, othersN = 0, ambigN = 0, ambigInKeys = 0;
    for (size_t p = 0; p < out.size(); ) {
        size_t e = out.find('\n', p);
        std::string ln = out.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
        p = (e == std::string::npos) ? out.size() : e + 1;
        while (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);

        if (ln.size() >= 2 && ln[0] == '@' && ln[1] == '@') {
            if (ln == "@@SCHEMA@@") {
                sect = 1;
                r->append("\n=== schema: PRAGMA table_info ===\n");
            } else if (ln == "@@STATS@@") {
                sect = 2;
                r->append("\n=== Rules stats ===\n");
            } else if (ln == "@@NAMED@@") {
                sect = 3;
                r->append("\n=== Target entries ===\n");
                r->append("svc_id | name | cas | tp | freq | pol | sr | sat | "
                          "in_keys | in_enc_open | other_rows_same_key_other_tp | n_tp | n_sig\n");
            } else if (ln == "@@OTHERS@@") {
                sect = 4;
                r->append("\n=== other rows sharing a requested (svc_id,tp_frequency) on a DIFFERENT TP ===\n");
                r->append("svc_id | freq | name | cas | svc_tp | tp | pol | sr | sat | sat_name\n");
            } else if (ln == "@@AMBIG30@@") {
                sect = 5;
                r->append("\n=== sample: first 30 rows whose (svc_id,tp_frequency) is ambiguous ===\n");
                r->append("svc_id | name | cas | freq | pol | sr | sat | sat_name | n_tp | n_sig\n");
            } else if (ln == "@@ENCFILE@@") {
                sect = 6;
                r->append("\n=== Open rules variant ===\n");
            }
            continue;
        }
        if (ln.empty()) continue;

        if (sect == 3) {
            std::string f[16];
            if (splitFields(ln, '|', f, 16) < 12) { r->append(ln); r->append("\n"); continue; }
            unsigned svc = (unsigned)strtoul(f[0].c_str(), nullptr, 10);
            unsigned tp = (unsigned)strtoul(f[3].c_str(), nullptr, 10);
            bool ink = orcaKeyHas(svc, tp);
            int cas = atoi(f[2].c_str());
            int ntp = atoi(f[9].c_str()), nsig = atoi(f[10].c_str()), noth = atoi(f[11].c_str());
            namedN++;
            if (ink) namedInKeys++;
            snprintf(line, sizeof(line),
                     "%s | %s | cas=%s | tp=%s | freq=%s | pol=%s(%s) | sr=%s | sat=%s(%s) | "
                     "in_keys=%s | in_enc_open=%s | other_tp_rows=%d | n_tp=%d | n_sig=%d%s\n",
                     f[0].c_str(), cleanField(f[1], 48).c_str(), f[2].c_str(), f[3].c_str(),
                     f[4].c_str(), f[5].c_str(), polName(f[5]), f[6].c_str(),
                     f[7].c_str(), cleanField(f[8], 32).c_str(),
                     ink ? "YES" : "no", (cas != 0 && ink) ? "YES" : "no",
                     noth, ntp, nsig,
                     (nsig > 1 || noth > 0) ? "  <-- AMBIGUOUS" : "");
            r->append(line);
        } else if (sect == 4) {
            std::string f[16];
            if (splitFields(ln, '|', f, 16) < 10) { r->append(ln); r->append("\n"); continue; }
            othersN++;
            snprintf(line, sizeof(line),
                     "%s | %s | %s | cas=%s | svc_tp=%s | tp=%s | pol=%s(%s) | sr=%s | sat=%s(%s)\n",
                     f[0].c_str(), f[1].c_str(), cleanField(f[2], 48).c_str(), f[3].c_str(),
                     f[4].c_str(), f[5].c_str(), f[6].c_str(), polName(f[6]), f[7].c_str(),
                     f[8].c_str(), cleanField(f[9], 32).c_str());
            r->append(line);
        } else if (sect == 5) {
            std::string f[16];
            if (splitFields(ln, '|', f, 16) < 10) { r->append(ln); r->append("\n"); continue; }
            unsigned svc = (unsigned)strtoul(f[0].c_str(), nullptr, 10);
            unsigned freq = (unsigned)strtoul(f[3].c_str(), nullptr, 10);
            bool ink = orcaKeyHas(svc, freq);
            ambigN++;
            if (ink) ambigInKeys++;
            snprintf(line, sizeof(line),
                     "%s | %s | cas=%s | freq=%s | pol=%s(%s) | sr=%s | sat=%s(%s) | "
                     "n_tp=%s | n_sig=%s | in_keys=%s%s\n",
                     f[0].c_str(), cleanField(f[1], 48).c_str(), f[2].c_str(), f[3].c_str(),
                     f[4].c_str(), polName(f[4]), f[5].c_str(),
                     f[6].c_str(), cleanField(f[7], 32).c_str(),
                     f[8].c_str(), f[9].c_str(), ink ? "YES" : "no",
                     ink ? "  <-- would be selected" : "");
            r->append(line);
        } else {
            r->append(ln);
            r->append("\n");
        }
    }

    snprintf(line, sizeof(line),
             "\nsummary: requested rows=%ld (in_keys=%ld) | other-TP rows=%ld | "
             "ambiguous sample rows=%ld (in_keys=%ld)\n",
             namedN, namedInKeys, othersN, ambigN, ambigInKeys);
    r->append(line);
    r->append("note: in_keys=YES matches open rules "
              "when svc_is_cas<>0\n");
}

static void writeReport(const char* argv0) {
    struct timeval tv0, tv1;
    gettimeofday(&tv0, nullptr);        /* wall time: sqlite3 runs in children */
    mkdir(DATA_DIR, 0755);

    std::string r;
    r.reserve(8192);
    char line[1024];

    time_t now = time(nullptr);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);

    snprintf(line, sizeof(line),
             "=== System report ===\n"
             "generated: %s\nversion: %s (%s)\npid: %d\n",
             ts, CC_VERSION_STRING, CC_BUILD_VERSION, (int)getpid());
    r += line;

    /* where the tool was really started from */
    char exe[256];
    ssize_t exn = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (exn > 0) exe[exn] = '\0'; else snprintf(exe, sizeof(exe), "(readlink failed)");
    snprintf(line, sizeof(line), "argv0: %s\nexe:   %s\n", argv0 ? argv0 : "(null)", exe);
    r += line;
    {
        char pp[32];
        snprintf(pp, sizeof(pp), "%d", (int)getppid());
        std::string pc = procCmdlineOf(pp);
        snprintf(line, sizeof(line), "parent pid: %d\nparent cmd: %s\n",
                 (int)getppid(), pc.c_str());
        r += line;
    }

    /* update system status */
    r += "\n--- update status ---\n";
    snprintf(line, sizeof(line), "version_num:    %s / (%d)\n",
             versionToString(CC_VERSION_NUM).c_str(), CC_VERSION_NUM);
    r += line;
    snprintf(line, sizeof(line), "update_pending: %s\n", (access(UPDATE_PENDING_FILE, F_OK) == 0) ? "YES" : "no");
    r += line;
    snprintf(line, sizeof(line), "update_ok:      %s\n", (access(UPDATE_OK_FILE, F_OK) == 0) ? "YES" : "no");
    r += line;
    snprintf(line, sizeof(line), "backup_prev:    %s\n", (access(BACKUP_BIN_PATH, F_OK) == 0) ? "YES" : "no");
    r += line;
    std::string ust = readFileAll(UPDATE_STATE_FILE, 512);
    if (ust.empty()) {
        r += "update_state:   (none / never)\n";
    } else {
        r += "update_state:\n";
        for (size_t p = 0; p < ust.size(); ) {
            size_t e = ust.find('\n', p);
            std::string ln = ust.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
            if (!ln.empty()) { r += "  "; r += ln; r += "\n"; }
            if (e == std::string::npos) break;
            p = e + 1;
        }
    }

    /* service.db */
    std::string errs;
    long liveCount = -1;
    r += "\n--- Active DB ---\n";
    struct stat st;
    if (stat(SERVICE_DB, &st) == 0) {
        char mts[64];
        struct tm* mt = localtime(&st.st_mtime);
        strftime(mts, sizeof(mts), "%Y-%m-%d %H:%M:%S", mt);
        snprintf(line, sizeof(line),
                 "size: %lld bytes\nmode: %04o  uid=%d gid=%d\nmtime: %s\n",
                 (long long)st.st_size, (unsigned)(st.st_mode & 07777),
                 (int)st.st_uid, (int)st.st_gid, mts);
        r += line;
        /* what the TV is serving right now (read-only query) */
        std::string ig, er;
        if (dbFacts(SERVICE_DB, &liveCount, &ig, &er)) {
            snprintf(line, sizeof(line), "count: %ld (integrity=%s)\n",
                     liveCount, ig.c_str());
            r += line;
        } else {
            snprintf(line, sizeof(line), "count: UNREADABLE (%s)\n", er.c_str());
            r += line;
            if (!er.empty()) { errs += "active: "; errs += er; errs += "\n"; }
        }
    } else {
        snprintf(line, sizeof(line), "MISSING (stat errno=%d)\n", errno);
        r += line;
    }

    /* every database: integrity + count */
    long builtCnt[8];
    std::string builtIg[8], builtErr[8];
    r += "\n--- DB Integrity & Counts ---\n";
    for (int i = 0; i < VARIANTS_N && i < 8; ++i) {
        char p[512];
        snprintf(p, sizeof(p), "%s/%s", DATA_DIR, VARIANTS[i].file.c_str());
        std::string ig, er;
        long cnt = -1;
        bool ok = dbFacts(p, &cnt, &ig, &er);
        builtCnt[i] = cnt; builtIg[i] = ig; builtErr[i] = er;
        if (ok)
            snprintf(line, sizeof(line), "%-32s integrity=%s count=%ld\n",
                     VARIANTS[i].file.c_str(), ig.c_str(), cnt);
        else
            snprintf(line, sizeof(line), "%-32s FAILED: %s\n",
                     VARIANTS[i].file.c_str(), er.c_str());
        r += line;
        if (!ok && !er.empty()) { errs += VARIANTS[i].file; errs += ": "; errs += er; errs += "\n"; }
    }

    /* does the live database still hold what the menu believes is active?
     * (the box rewrites service.db on boot - this line makes that visible) */
    {
        std::string cur = readCurrentChannels();
        int vi = -1;
        for (int i = 0; i < VARIANTS_N && i < 8; ++i)
            if (cur == VARIANTS[i].label) { vi = i; break; }
        if (liveCount < 0) {
            r += "live vs choice: unreadable\n";
        } else if (vi < 0) {
            snprintf(line, sizeof(line),
                     "live count=%ld ; choice '%s' unknown variant\n",
                     liveCount, cur.c_str());
            r += line;
        } else if (builtCnt[vi] < 0) {
            snprintf(line, sizeof(line),
                     "live count=%ld ; choice '%s' not built\n",
                     liveCount, cur.c_str());
            r += line;
        } else if (liveCount == builtCnt[vi]) {
            snprintf(line, sizeof(line),
                     "live count=%ld matches '%s' (%s): OK\n",
                     liveCount, cur.c_str(), VARIANTS[vi].file.c_str());
            r += line;
        } else {
            snprintf(line, sizeof(line),
                     "live count=%ld does NOT match choice '%s' "
                     "(expect %ld): database rewritten (reboot/TV)\n",
                     liveCount, cur.c_str(), builtCnt[vi]);
            r += line;
        }
    }

    /* built vs re-derived-from-pristine */
    r += "\n--- Variant count verify ---\n";
    for (int i = 0; i < VARIANTS_N && i < 8; ++i) {
        if (builtCnt[i] < 0) {
            snprintf(line, sizeof(line), "%-32s SKIPPED (cannot count built file)\n",
                     VARIANTS[i].file.c_str());
            r += line;
            continue;
        }
        long exp = -1;
        std::string ig, er;
        if (!expectedCount(&VARIANTS[i], &exp, &ig, &er)) {
            snprintf(line, sizeof(line), "%-32s CHECK FAILED: %s\n",
                     VARIANTS[i].file.c_str(), er.c_str());
            r += line;
            if (!er.empty()) { errs += VARIANTS[i].file + " expected: " + er + "\n"; }
            continue;
        }
        const char* verdict = (exp == builtCnt[i]) ? "MATCH" : "MISMATCH";
        snprintf(line, sizeof(line), "%-32s built=%ld expected=%ld %s\n",
                 VARIANTS[i].file.c_str(), builtCnt[i], exp, verdict);
        r += line;
        if (exp != builtCnt[i])
            errs += VARIANTS[i].file + ": built != expected\n";
    }

    /* why is a channel in the open variants although orca does not open it? */
    appendOrcaDiag(&r, &errs);

    /* what the old shell builder left in /data/db, if anything */
    r += "\n--- Legacy output ---\n";
    {
        const struct { const char* path; int vi; } olds[] = {
            { OLD_FREE_DB,        1 },
            { OLD_FREE_OPEN_DB,   2 },
            { OLD_ENC_OPEN_DB,    3 },
            { OLD_ENC_CLOSED_DB,  4 },
        };
        for (size_t k = 0; k < sizeof(olds) / sizeof(olds[0]); ++k) {
            if (fileSizeOf(olds[k].path) < 0) {
                snprintf(line, sizeof(line), "%-44s not present\n", olds[k].path);
                r += line;
                continue;
            }
            long cnt = -1;
            std::string ig, er;
            if (!dbFacts(olds[k].path, &cnt, &ig, &er)) {
                snprintf(line, sizeof(line), "%-44s FAILED: %s\n", olds[k].path, er.c_str());
                r += line;
                if (!er.empty()) { errs += std::string(olds[k].path) + ": " + er + "\n"; }
                continue;
            }
            const char* verdict = "n/a";
            if (olds[k].vi < 8 && builtCnt[olds[k].vi] >= 0)
                verdict = (cnt == builtCnt[olds[k].vi]) ? "MATCH" : "MISMATCH";
            snprintf(line, sizeof(line), "%-44s old=%ld ours=%ld %s\n",
                     olds[k].path, cnt,
                     (olds[k].vi < 8) ? builtCnt[olds[k].vi] : (long)-1, verdict);
            r += line;
        }
    }

    /* the files the menu reads */
    r += "\n--- files the menu reads ---\n";
    r += "current: ";
    r += readCurrentChannels();
    r += "\nvariant_counts.txt:\n";
    {
        std::string c = readFileAll(VAR_COUNTS, 4096);
        if (c.empty()) r += "  (missing)\n";
        else {
            for (size_t p = 0; p < c.size(); ) {
                size_t e = c.find('\n', p);
                std::string ln = c.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
                if (!ln.empty()) { r += "  "; r += ln; r += "\n"; }
                if (e == std::string::npos) break;
                p = e + 1;
            }
        }
    }

    /* process pictures */
    appendProcPics(&r);

    /* /dev/hi_* holders */
    r += "\n--- hi_* holders ---\n";
    std::vector<HiHolder> hlist = scanHiHolders(false);
    r += formatHiHoldersTable(hlist);

    /* sqlite3 problems */
    r += "\n--- sqlite3 errors ---\n";
    if (errs.empty()) r += "none\n";
    else r += errs;

    gettimeofday(&tv1, nullptr);
    int ms = (int)((tv1.tv_sec - tv0.tv_sec) * 1000 + (tv1.tv_usec - tv0.tv_usec) / 1000);
    snprintf(line, sizeof(line), "\nwritten by %s in %d ms\n", CC_BUILD_VERSION, ms);
    r += line;

    unlink(REPORT_TMP);
    if (writeFile(REPORT_TMP, r, true) && rename(REPORT_TMP, REPORT_PATH) == 0)
        dbg("[report] %s (%d bytes, %d ms)", REPORT_PATH, (int)r.size(), ms);
    else
        dbg("[report] FAILED to write %s errno=%d", REPORT_PATH, errno);
}

/* ---------- Instance management ---------- */
static void writePidFile() {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d\n", (int)getpid());
    writeFile(PID_FILE, buf, true);
}

static pid_t readPidFile() {
    std::string s = readFile(PID_FILE);
    if (s.empty()) return -1;
    pid_t p = (pid_t)atoi(s.c_str());
    return (p > 0) ? p : -1;
}

/* Real executable of a pid, read from /proc/<pid>/exe.
 * cmdline is free text that anything can fill with anything ("sh -c ...",
 * an editor with the file open, a log line) - matching it has already been
 * seen to stop processes that merely MENTION .ColorPro.  The exe symlink is
 * the kernel's own record of which file is actually running, so that is what
 * instances are identified by. */
static std::string procExe(pid_t p) {
    char link[64], target[256];
    snprintf(link, sizeof(link), "/proc/%d/exe", (int)p);
    ssize_t n = readlink(link, target, sizeof(target) - 1);
    if (n <= 0) return std::string();
    target[n] = '\0';
    std::string s(target);
    /* the kernel appends " (deleted)" when the binary was replaced under it */
    static const char* DEL = " (deleted)";
    size_t dl = strlen(DEL);
    if (s.size() > dl && s.compare(s.size() - dl, dl, DEL) == 0) s.erase(s.size() - dl);
    return s;
}

/* true only when that pid runs the same program we are: our own exe, or the
 * canonical install path.  Anything else - however similar its cmdline looks -
 * is somebody else and is left running. */
static bool matchInstance(const char* procName, const std::string& self, std::string* exeOut) {
    pid_t p = (pid_t)atoi(procName);
    if (p <= 0) return false;
    std::string e = procExe(p);
    if (e.empty()) return false;          /* not provably ours -> do not touch */
    if (e != self && e != TARGET_BIN_PATH) return false;
    if (exeOut) *exeOut = e;
    return true;
}

/* cmdline is "arg0\0arg1\0..."; a NUL before the last byte means the process
 * was started with arguments, i.e. it is a CLI run (update-local, rollback,
 * guard, ...) and not the daemon.  Those must survive: the updater is polling
 * our readiness pipe and the rollback CLI is waiting for the restored daemon -
 * killing them loses the OK/FAILED result and the update lock release. */
static bool hasCmdlineArgs(pid_t p) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", (int)p);
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    char buf[512];
    ssize_t n = read(fd, buf, sizeof(buf));
    close(fd);
    if (n <= 0) return false;
    for (ssize_t i = 0; i + 1 < n; ++i)
        if (buf[i] == '\0') return true;
    return false;
}

static void stopAllInstances() {
    pid_t me = getpid();
    std::string self = procExe(me);
    if (self.empty()) self = TARGET_BIN_PATH;
    DIR* d = opendir("/proc");
    if (!d) return;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR) continue;
        const char* name = de->d_name;
        if (!isdigit((unsigned char)name[0])) continue;
        std::string exe;
        if (!matchInstance(name, self, &exe)) continue;
        pid_t p = (pid_t)atoi(name);
        if (p > 0 && p != me && !hasCmdlineArgs(p)) {
            kill(p, SIGTERM);
            dbg("[init] SIGTERM old pid %d (%s)", (int)p, exe.c_str());
        }
    }
    closedir(d);
    usleep(OLD_INSTANCE_WAIT_MS * 1000);
    /* Second pass: SIGKILL any survivor */
    d = opendir("/proc");
    if (!d) return;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR) continue;
        const char* name = de->d_name;
        if (!isdigit((unsigned char)name[0])) continue;
        std::string exe;
        if (!matchInstance(name, self, &exe)) continue;
        pid_t p = (pid_t)atoi(name);
        if (p > 0 && p != me && !hasCmdlineArgs(p)) {
            kill(p, SIGKILL);
            dbg("[init] SIGKILL old pid %d (%s)", (int)p, exe.c_str());
        }
    }
    closedir(d);
}

/* ---------- Remote (IR) device discovery ----------
 * DEV (/dev/input/event0) used to be hard-wired.  On boxes where the IR
 * receiver shows up as event1 - or where an air mouse/USB keypad owns event0 -
 * the daemon then listened to the wrong device: real remote keys never
 * arrived, or foreign keys did.  Every /dev/input/event* is opened, asked what
 * it is (EVIOCGNAME) and whether it can produce the EV_KEY events we read, and
 * the best candidate wins; DEV is only the last resort. */
static bool devHasKeys(int fd) {
    unsigned long bits[2] = { 0, 0 };
    if (ioctl(fd, EVIOCGBIT(0, sizeof(bits)), bits) < 0) return false;
    return ((bits[0] >> EV_KEY) & 1UL) != 0;
}

/* Higher is better; 0 means "no keys at all" and the device is unusable. */
static int irScore(const char* name) {
    char n[128];
    snprintf(n, sizeof(n), "%s", (name && *name) ? name : "");
    for (int i = 0; n[i]; i++) n[i] = (char)tolower((unsigned char)n[i]);
    if (!n[0]) return 0;

    int s = 1;                                  /* has a name at all        */
    if (strstr(n, "ir-") || strstr(n, "-ir") || strstr(n, "_ir") ||
        strcmp(n, "ir") == 0 || strstr(n, "receiver") || strstr(n, "remote") ||
        strstr(n, "rc5")  || strstr(n, "rc6")  || strstr(n, "mce") ||
        strstr(n, "cec"))
        s += 100;                               /* this smells like the IR path */
    if (strstr(n, "hisilicon") || strstr(n, "hi-")) s += 20;
    if (strstr(n, "keypad") || strstr(n, "keyboard") ||
        strstr(n, "mouse")  || strstr(n, "touch"))
        s += 5;                                 /* takes keys, but is not the remote */
    return s;
}

static int openRemoteDevice() {
    int best = -1, bestScore = -1;
    char bestPath[64] = "";
    char bestName[128] = "";

    for (int i = 0; i < 32; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        char name[128];
        memset(name, 0, sizeof(name));
        if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) < 0) name[0] = '\0';
        bool keys = devHasKeys(fd);
        int score = keys ? irScore(name) : -1;
        dbg("[remote] %s name='%s' EV_KEY=%d score=%d", path, name, keys ? 1 : 0, score);
        if (score > bestScore) {
            if (best >= 0) close(best);
            best = fd;
            bestScore = score;
            snprintf(bestPath, sizeof(bestPath), "%s", path);
            snprintf(bestName, sizeof(bestName), "%s", name);
        } else {
            close(fd);
        }
    }

    if (best < 0) {
        best = open(DEV, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (best < 0) {
            dbg("[remote] no usable input device (last tried %s) errno=%d", DEV, errno);
            return -1;
        }
        snprintf(bestPath, sizeof(bestPath), "%s", DEV);
        snprintf(bestName, sizeof(bestName), "%s", "?");
    }
    snprintf(g_devPath, sizeof(g_devPath), "%s", bestPath);
    snprintf(g_devName, sizeof(g_devName), "%s", bestName[0] ? bestName : "?");
    dbg("[remote] chosen %s ('%s') score=%d", g_devPath, g_devName, bestScore);
    return best;
}

/* ==================================================================== *
 *  Update System (Phase 1: Local update, Ed25519 verify, guard, rollback)
 * ==================================================================== */

struct ManifestFile {
    std::string name;
    size_t size = 0;
    std::string sha512;
};

struct UpdateManifest {
    int version = 0;        /* manifest.txt MUST stay an integer (10000, 10100...) - read with atoi() */
    int min_version = 0;    /* same rule as version */
    std::string released;
    std::string notes;
    std::vector<ManifestFile> files;
};

static bool isPubKeyConfigured() {
    for (int i = 0; i < 32; ++i) {
        if (UPDATE_PUBKEY[i] != 0) return true;
    }
    return false;
}

static bool verifyEd25519Signature(const uint8_t* msg, size_t msg_len,
                                   const uint8_t sig[64], const uint8_t pk[32]) {
    unsigned long long smlen = 64 + msg_len;
    std::vector<unsigned char> sm(smlen);
    memcpy(sm.data(), sig, 64);
    if (msg_len > 0 && msg != nullptr) memcpy(sm.data() + 64, msg, msg_len);

    std::vector<unsigned char> m(smlen);
    unsigned long long mlen = 0;
    int r = crypto_sign_open(m.data(), &mlen, sm.data(), smlen, pk);
    return (r == 0 && mlen == (unsigned long long)msg_len);
}

static bool runSelftest() {
    // RFC 8032 Section 7.1 Test Vector 1 (0 bytes message):
    static const uint8_t RFC_PK1[32] = {
        0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
        0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
        0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
        0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
    };
    static const uint8_t RFC_SIG1[64] = {
        0xe5, 0x56, 0x43, 0x00, 0xc3, 0x60, 0xac, 0x72,
        0x90, 0x86, 0xe2, 0xcc, 0x80, 0x6e, 0x82, 0x8a,
        0x84, 0x87, 0x7f, 0x1e, 0xb8, 0xe5, 0xd9, 0x74,
        0xd8, 0x73, 0xe0, 0x65, 0x22, 0x49, 0x01, 0x55,
        0x5f, 0xb8, 0x82, 0x15, 0x90, 0xa3, 0x3b, 0xac,
        0xc6, 0x1e, 0x39, 0x70, 0x1c, 0xf9, 0xb4, 0x6b,
        0xd2, 0x5b, 0xf5, 0xf0, 0x59, 0x5b, 0xbe, 0x24,
        0x65, 0x51, 0x41, 0x43, 0x8e, 0x7a, 0x10, 0x0b
    };

    if (!verifyEd25519Signature(nullptr, 0, RFC_SIG1, RFC_PK1)) return false;

    // RFC 8032 Section 7.1 Test Vector 2 (1 byte message: 0x72):
    static const uint8_t RFC_PK2[32] = {
        0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a,
        0x92, 0xb7, 0x0a, 0xa7, 0x4d, 0x1b, 0x7e, 0xbc,
        0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4, 0x96, 0x8c,
        0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c
    };
    static const uint8_t RFC_MSG2[1] = { 0x72 };
    static const uint8_t RFC_SIG2[64] = {
        0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8,
        0x72, 0x0e, 0x82, 0x0b, 0x5f, 0x64, 0x25, 0x40,
        0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f,
        0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda,
        0x08, 0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99, 0x6e,
        0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c,
        0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a, 0xee,
        0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00
    };

    if (!verifyEd25519Signature(RFC_MSG2, sizeof(RFC_MSG2), RFC_SIG2, RFC_PK2)) return false;

    // RFC 8032 Section 7.1 Test Vector 3 (2 bytes message: 0xaf, 0x82):
    static const uint8_t RFC_PK3[32] = {
        0xfc, 0x51, 0xcd, 0x8e, 0x62, 0x18, 0xa1, 0xa3,
        0x8d, 0xa4, 0x7e, 0xd0, 0x02, 0x30, 0xf0, 0x58,
        0x08, 0x16, 0xed, 0x13, 0xba, 0x33, 0x03, 0xac,
        0x5d, 0xeb, 0x91, 0x15, 0x48, 0x90, 0x80, 0x25
    };
    static const uint8_t RFC_MSG3[2] = { 0xaf, 0x82 };
    static const uint8_t RFC_SIG3[64] = {
        0x62, 0x91, 0xd6, 0x57, 0xde, 0xec, 0x24, 0x02,
        0x48, 0x27, 0xe6, 0x9c, 0x3a, 0xbe, 0x01, 0xa3,
        0x0c, 0xe5, 0x48, 0xa2, 0x84, 0x74, 0x3a, 0x44,
        0x5e, 0x36, 0x80, 0xd7, 0xdb, 0x5a, 0xc3, 0xac,
        0x18, 0xff, 0x9b, 0x53, 0x8d, 0x16, 0xf2, 0x90,
        0xae, 0x67, 0xf7, 0x60, 0x98, 0x4d, 0xc6, 0x59,
        0x4a, 0x7c, 0x15, 0xe9, 0x71, 0x6e, 0xd2, 0x8d,
        0xc0, 0x27, 0xbe, 0xce, 0xea, 0x1e, 0xc4, 0x0a
    };

    if (!verifyEd25519Signature(RFC_MSG3, sizeof(RFC_MSG3), RFC_SIG3, RFC_PK3)) return false;

    // Negative check: corrupted signature must fail
    uint8_t bad_sig[64];
    memcpy(bad_sig, RFC_SIG1, 64);
    bad_sig[0] ^= 0x01;
    return !verifyEd25519Signature(nullptr, 0, bad_sig, RFC_PK1);
}

static bool sha512OfFile(const char* path, std::string* outHex) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < 0 || st.st_size > (off_t)MAX_EXE_SIZE) {
        close(fd);
        return false;
    }
    size_t sz = (size_t)st.st_size;
    std::vector<uint8_t> buf(sz);
    size_t rd = 0;
    while (rd < sz) {
        ssize_t n = read(fd, buf.data() + rd, sz - rd);
        if (n <= 0) break;
        rd += n;
    }
    close(fd);
    if (rd != sz) return false;

    uint8_t hash[64];
    crypto_hash(hash, buf.data(), (unsigned long long)sz);
    char hex[129];
    for (int i = 0; i < 64; ++i) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    hex[128] = '\0';
    *outHex = hex;
    return true;
}

static bool validateElf32Arm(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    uint8_t hdr[52];
    ssize_t n = read(fd, hdr, sizeof(hdr));
    close(fd);
    if (n != (ssize_t)sizeof(hdr)) return false;

    if (hdr[0] != 0x7f || hdr[1] != 'E' || hdr[2] != 'L' || hdr[3] != 'F') return false;
    if (hdr[4] != 1) return false;
    if (hdr[5] != 1) return false;
    uint16_t e_machine = (uint16_t)hdr[18] | ((uint16_t)hdr[19] << 8);
    if (e_machine != 40) return false;

    return true;
}

static bool validateJar(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    uint8_t hdr[4];
    ssize_t n = read(fd, hdr, sizeof(hdr));
    close(fd);
    if (n < 4) return false;
    return (hdr[0] == 0x50 && hdr[1] == 0x4b && hdr[2] == 0x03 && hdr[3] == 0x04);
}

static bool runCandidateSelftest(const char* exePath, int expectedVer) {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC) != 0) return false;

    sigset_t block, oldmask;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &oldmask);

    pid_t pid = fork();
    if (pid < 0) {
        sigprocmask(SIG_SETMASK, &oldmask, nullptr);
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        sigprocmask(SIG_SETMASK, &oldmask, nullptr);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        closeAllFdsAbove(3);
        char* const args[] = { (char*)exePath, (char*)"selftest", nullptr };
        execv(exePath, args);
        _exit(127);
    }

    close(pipefd[1]);

    std::string output;
    char buf[256];
    int64_t startMs = nowMs();
    int status = -1;
    bool done = false;

    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    while (nowMs() - startMs < 5000) {
        ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            output += buf;
        }
        pid_t wp = waitpid(pid, &status, WNOHANG);
        if (wp == pid) {
            done = true;
            while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';
                output += buf;
            }
            break;
        }
        usleep(50000);
    }
    close(pipefd[0]);
    sigprocmask(SIG_SETMASK, &oldmask, nullptr);

    if (!done) {
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        dbg("[up] candidate selftest timed out (>5s)");
        return false;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        dbg("[up] candidate selftest failed with code %d", WEXITSTATUS(status));
        return false;
    }

    char expected[64];
    snprintf(expected, sizeof(expected), "SELFTEST OK %d", expectedVer);
    if (output.find(expected) == std::string::npos) {
        dbg("[up] candidate selftest output mismatch: got '%s', want '%s'",
            output.c_str(), expected);
        return false;
    }

    return true;
}

static void fsyncDir(const char* filePath) {
    std::string dir = filePath;
    size_t lastSlash = dir.rfind('/');
    if (lastSlash == std::string::npos) dir = ".";
    else if (lastSlash == 0) dir = "/";
    else dir.resize(lastSlash);
    int dfd = open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dfd >= 0) {
        fsync(dfd);
        close(dfd);
    }
}

/* Atomic file installer:
 * Copies src to a temporary file (dst + ".new") in the SAME directory.
 * Fsyncs, chmods, verifies post-copy size and SHA-512 against expectations,
 * then atomically renames over dst, followed by fsync on parent directory.
 * Target file is NEVER opened for writing, completely avoiding Linux ETXTBSY
 * on running executables and preventing partial file corruption.
 */
static bool atomicInstallFile(const char* src, const char* dst, mode_t mode,
                              size_t expectedSize, const std::string& expectedSha512 = "") {
    if (expectedSize == 0) {
        dbg("[atomic] REFUSED: expectedSize must be > 0 for '%s'", dst);
        return false;
    }

    std::string tmpPath = std::string(dst) + ".new";
    unlink(tmpPath.c_str());

    int in = open(src, O_RDONLY | O_CLOEXEC);
    if (in < 0) {
        dbg("[atomic] open src '%s' failed errno=%d", src, errno);
        return false;
    }

    int out = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode);
    if (out < 0) {
        dbg("[atomic] open tmp '%s' failed errno=%d", tmpPath.c_str(), errno);
        close(in);
        return false;
    }

    bool ok = true;
    char buf[32768];
    while (ok) {
        ssize_t n = read(in, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        if (n == 0) break;
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if (w < 0) {
                if (errno == EINTR) continue;
                ok = false;
                break;
            }
            off += w;
        }
    }
    if (ok && fsync(out) != 0) {
        dbg("[atomic] fsync of %s failed errno=%d", tmpPath.c_str(), errno);
        ok = false;
    }
    close(out);
    close(in);

    if (!ok) {
        dbg("[atomic] copy %s -> %s failed during transfer", src, tmpPath.c_str());
        unlink(tmpPath.c_str());
        return false;
    }

    chmod(tmpPath.c_str(), mode);

    // Verify size (mandatory)
    long long actualSize = fileSizeOf(tmpPath.c_str());
    if (actualSize < 0 || (size_t)actualSize != expectedSize) {
        dbg("[atomic] post-copy size mismatch for %s: got %lld, want %zu",
            tmpPath.c_str(), actualSize, expectedSize);
        unlink(tmpPath.c_str());
        return false;
    }

    // Verify SHA-512 if specified
    if (!expectedSha512.empty()) {
        std::string actualSha;
        if (!sha512OfFile(tmpPath.c_str(), &actualSha) ||
            strcasecmp(actualSha.c_str(), expectedSha512.c_str()) != 0) {
            dbg("[atomic] post-copy SHA-512 mismatch for %s", tmpPath.c_str());
            unlink(tmpPath.c_str());
            return false;
        }
    }

    // Atomic rename over destination
    if (rename(tmpPath.c_str(), dst) != 0) {
        dbg("[atomic] rename %s -> %s failed errno=%d", tmpPath.c_str(), dst, errno);
        unlink(tmpPath.c_str());
        return false;
    }

    // Ensure directory entry metadata is committed to disk
    fsyncDir(dst);

    dbg("[atomic] installed %s -> %s (mode %o, size %zu) successfully",
        src, dst, (unsigned)mode, expectedSize);
    return true;
}

static bool parseManifest(const std::string& text, UpdateManifest* outManifest, std::string* errDesc) {
    outManifest->version = 0;
    outManifest->min_version = 0;
    outManifest->released.clear();
    outManifest->notes.clear();
    outManifest->files.clear();

    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find('\n', pos);
        std::string line = text.substr(pos, (end == std::string::npos) ? std::string::npos : end - pos);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        if (!line.empty()) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);

                if (k == "version") {
                    outManifest->version = atoi(v.c_str());
                } else if (k == "min_version") {
                    outManifest->min_version = atoi(v.c_str());
                } else if (k == "released") {
                    outManifest->released = v;
                } else if (k == "notes") {
                    outManifest->notes = v;
                } else if (k == "file") {
                    size_t p1 = v.find('|');
                    size_t p2 = (p1 != std::string::npos) ? v.find('|', p1 + 1) : std::string::npos;
                    if (p1 != std::string::npos && p2 != std::string::npos) {
                        ManifestFile mf;
                        mf.name = v.substr(0, p1);
                        mf.size = (size_t)strtoull(v.substr(p1 + 1, p2 - p1 - 1).c_str(), nullptr, 10);
                        mf.sha512 = v.substr(p2 + 1);

                        if (mf.name != CC_NAME_RAW && mf.name != HUD_NAME_RAW) {
                            *errDesc = "unsupported file in manifest: " + mf.name;
                            return false;
                        }
                        outManifest->files.push_back(mf);
                    }
                }
            }
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }

    if (outManifest->version <= 0) {
        *errDesc = "manifest missing valid version";
        return false;
    }
    return true;
}

static bool isTargetExeRunning() {
    DIR* d = opendir("/proc");
    if (!d) return false;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR) continue;
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        pid_t p = (pid_t)atoi(de->d_name);
        if (p <= 1 || p == getpid()) continue;
        std::string e = procExe(p);
        if (e == TARGET_BIN_PATH) {
            closedir(d);
            return true;
        }
    }
    closedir(d);
    return false;
}

static bool performRollback();

static void safeKillAndReap(pid_t pid, int timeoutMs = 500) {
    if (pid <= 0) return;
    kill(pid, SIGKILL);
    int64_t start = nowMs();
    while (nowMs() - start < timeoutMs) {
        int status = 0;
        pid_t wp = waitpid(pid, &status, WNOHANG);
        if (wp == pid || wp < 0) return;
        usleep(10000); // 10ms
    }
    dbg("[process] WARN: pid %d did not exit within %d ms (possible D-state)", pid, timeoutMs);
}

static void runGuardProcess(int oldVer, int newVer, int readyFd) {
    setsid();
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        dup2(devnull, STDIN_FILENO);
        if (devnull > 2) close(devnull);
    }

    dbg("[guard] started (old=%s, new=%s, readyFd=%d), waiting up to 45s for %s",
        versionToString(oldVer).c_str(), versionToString(newVer).c_str(),
        readyFd, UPDATE_OK_FILE);

    // Signal readiness to parent updater
    if (readyFd >= 0) {
        char r = 'R';
        ssize_t wr = write(readyFd, &r, 1);
        close(readyFd);
        (void)wr;
    }

    char want[32];
    snprintf(want, sizeof(want), "%d", newVer);

    bool ok = false;
    bool earlyRollback = false;
    for (int i = 0; i < 45; ++i) {
        sleep(1);
        std::string content = readFile(UPDATE_OK_FILE);
        if (content.find(want) != std::string::npos) {
            ok = true;
            break;
        }
        if (i >= 10 && !isTargetExeRunning()) {
            dbg("[guard] EARLY ROLLBACK: no process running %s after %d seconds (target died)",
                TARGET_BIN_PATH, i + 1);
            earlyRollback = true;
            break;
        }
    }

    if (ok) {
        dbg("[guard] new version %s confirmed healthy! Guard exiting.",
            versionToString(newVer).c_str());
        unlink(UPDATE_GUARD_BIN);
        return;
    }

    if (earlyRollback) {
        dbg("[guard] TARGET DEAD: new version %d is not running! Initiating immediate ROLLBACK.", newVer);
    } else {
        dbg("[guard] TIMEOUT: new version %d did not write update_ok within 45s! Initiating ROLLBACK.", newVer);
    }

    bool rolledBack = performRollback();
    dbg("[guard] performRollback returned %s", rolledBack ? "OK" : "FAILED");

    FILE* fp = fopen(DBG_FILE, "a");
    if (fp) {
        fprintf(fp, "[guard] ROLLBACK PERFORMED: restored version %d because version %d failed health check (early=%d)\n",
                oldVer, newVer, earlyRollback ? 1 : 0);
        fclose(fp);
    }

    unlink(UPDATE_GUARD_BIN);
}

static bool performRollback() {
    if (access(BACKUP_BIN_PATH, F_OK) != 0) {
        dbg("[rollback] REFUSED: no backup binary found at %s", BACKUP_BIN_PATH);
        printf("REFUSED: no backup binary found (%s)\n", BACKUP_BIN_PATH);
        return false;
    }

    ScopedUpdateLock lock;
    if (!lock.acquire()) {
        dbg("[rollback] REFUSED: update lock active");
        printf("REFUSED: update lock active\n");
        return false;
    }

    stopAllInstances();

    if (rename(BACKUP_BIN_PATH, TARGET_BIN_PATH) != 0) {
        dbg("[rollback] rename %s -> %s failed errno=%d", BACKUP_BIN_PATH, TARGET_BIN_PATH, errno);
        return false;
    }
    fsyncDir(TARGET_BIN_PATH);
    chmod(TARGET_BIN_PATH, 0755);

    if (access(BACKUP_JAR_PATH, F_OK) == 0) {
        if (rename(BACKUP_JAR_PATH, TARGET_JAR_PATH) == 0) {
            fsyncDir(TARGET_JAR_PATH);
            chmod(TARGET_JAR_PATH, 0644);
        } else {
            dbg("[rollback] rename %s -> %s failed errno=%d", BACKUP_JAR_PATH, TARGET_JAR_PATH, errno);
        }
    }

    unlink(UPDATE_PENDING_FILE);
    unlink(UPDATE_OK_FILE);

    dbg("[rollback] rollback applied successfully; restarting daemon");

    int daemonPipe[2];
    if (pipe(daemonPipe) != 0) {
        daemonPipe[0] = -1;
        daemonPipe[1] = -1;
    }

    pid_t p = fork();
    if (p < 0) {
        if (daemonPipe[0] >= 0) { close(daemonPipe[0]); close(daemonPipe[1]); }
        dbg("[rollback] fork daemon failed errno=%d", errno);
        lock.release();
        return false;
    }
    if (p == 0) {
        if (daemonPipe[0] >= 0) close(daemonPipe[0]);
        setsid();
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            dup2(devnull, STDIN_FILENO);
            if (devnull > 2) close(devnull);
        }
        if (daemonPipe[1] >= 0) {
            char envBuf[16];
            snprintf(envBuf, sizeof(envBuf), "%d", daemonPipe[1]);
            setenv("SF_PIPE", envBuf, 1);
        }
        char* const args[] = { (char*)TARGET_BIN_PATH, nullptr };
        execv(TARGET_BIN_PATH, args);
        _exit(127);
    }

    if (daemonPipe[1] >= 0) close(daemonPipe[1]);

    if (daemonPipe[0] >= 0) {
        struct pollfd dpfd;
        dpfd.fd = daemonPipe[0];
        dpfd.events = POLLIN;
        dpfd.revents = 0;
        int dpr = poll(&dpfd, 1, 1500); // Wait up to 1.5s for restored daemon
        char dByte = 0;
        if (dpr > 0 && (dpfd.revents & POLLIN)) {
            (void)read(daemonPipe[0], &dByte, 1);
        }
        close(daemonPipe[0]);
        dbg("[rollback] restored daemon signaled readiness (byte='%c', poll=%d)", dByte, dpr);
    }

    lock.release();
    return true;
}

static bool performLocalUpdate(const char* dir) {
    ScopedUpdateLock lock;
    if (!lock.acquire()) {
        dbg("[up] REFUSED: update lock active");
        printf("REFUSED: update lock active\n");
        return false;
    }

    if (!isPubKeyConfigured()) {
        dbg("[up] REFUSED: update disabled: no public key configured");
        printf("REFUSED: update disabled: no public key configured\n");
        return false;
    }

    // Clean any leftover marker files from previous failed runs before starting
    unlink(UPDATE_OK_FILE);
    unlink(UPDATE_PENDING_FILE);

    std::string manifestPath = std::string(dir) + MANIFEST_TXT;
    std::string sigPath = std::string(dir) + MANIFEST_SIG;

    if (access(manifestPath.c_str(), R_OK) != 0 || access(sigPath.c_str(), R_OK) != 0) {
        dbg("[up] REFUSED: manifest.txt or manifest.sig missing in %s", dir);
        printf("REFUSED: manifest.txt or manifest.sig missing in %s\n", dir);
        return false;
    }

    long long msize = fileSizeOf(manifestPath.c_str());
    if (msize <= 0 || msize > MAX_MANIFEST_SIZE) {
        dbg("[up] REFUSED: invalid manifest size (%lld bytes)", msize);
        printf("REFUSED: invalid manifest size (%lld bytes)\n", msize);
        return false;
    }

    std::string manifestBytes = readFileAll(manifestPath.c_str(), MAX_MANIFEST_SIZE);
    if (manifestBytes.size() != (size_t)msize) {
        dbg("[up] REFUSED: could not read exact manifest bytes");
        printf("REFUSED: could not read exact manifest bytes\n");
        return false;
    }

    std::string sigHex = readFile(sigPath.c_str(), 256);
    while (!sigHex.empty() && (sigHex.back() == '\r' || sigHex.back() == '\n' || sigHex.back() == ' '))
        sigHex.pop_back();

    if (sigHex.size() != 128) {
        dbg("[up] REFUSED: signature length %zu != 128", sigHex.size());
        printf("REFUSED: invalid signature length\n");
        return false;
    }

    uint8_t sig[64];
    for (int i = 0; i < 64; ++i) {
        char byteHex[3] = { sigHex[i * 2], sigHex[i * 2 + 1], '\0' };
        sig[i] = (uint8_t)strtoul(byteHex, nullptr, 16);
    }

    if (!verifyEd25519Signature((const uint8_t*)manifestBytes.data(), manifestBytes.size(), sig, UPDATE_PUBKEY)) {
        dbg("[up] REFUSED: Ed25519 signature verification FAILED!");
        printf("REFUSED: untrusted update (signature verification failed)\n");
        return false;
    }
    dbg("[up] Ed25519 signature verified OK");

    UpdateManifest manifest;
    std::string parseErr;
    if (!parseManifest(manifestBytes, &manifest, &parseErr)) {
        dbg("[up] REFUSED: manifest parse error: %s", parseErr.c_str());
        printf("REFUSED: manifest parse error: %s\n", parseErr.c_str());
        return false;
    }

    if (manifest.version <= CC_VERSION_NUM) {
        dbg("[up] REFUSED: manifest version %s <= current %s (no downgrades)",
            versionToString(manifest.version).c_str(), versionToString(CC_VERSION_NUM).c_str());
        printf("REFUSED: already on latest version or newer (%s <= %s)\n",
            versionToString(manifest.version).c_str(), versionToString(CC_VERSION_NUM).c_str());
        return false;
    }
    if (CC_VERSION_NUM < manifest.min_version) {
        dbg("[up] REFUSED: current version %s < min_version %s",
            versionToString(CC_VERSION_NUM).c_str(), versionToString(manifest.min_version).c_str());
        printf("REFUSED: current version %s below required min_version %s\n",
            versionToString(CC_VERSION_NUM).c_str(), versionToString(manifest.min_version).c_str());
        return false;
    }

    const ManifestFile* binManifest = nullptr;
    const ManifestFile* jarManifest = nullptr;

    for (const auto& f : manifest.files) {
        std::string fpath = std::string(dir) + "/" + f.name;
        long long fsize = fileSizeOf(fpath.c_str());
        if (fsize < 0 || (size_t)fsize != f.size) {
            dbg("[up] REFUSED: file %s size mismatch: got %lld, want %zu",
                f.name.c_str(), fsize, f.size);
            printf("REFUSED: file %s size mismatch\n", f.name.c_str());
            return false;
        }

        size_t maxLimit = (f.name == CC_NAME_RAW) ? MAX_EXE_SIZE : MAX_JAR_SIZE;
        if (f.size > maxLimit) {
            dbg("[up] REFUSED: file %s size %zu exceeds limit %zu",
                f.name.c_str(), f.size, maxLimit);
            printf("REFUSED: file %s exceeds max size\n", f.name.c_str());
            return false;
        }

        std::string calcSha;
        if (!sha512OfFile(fpath.c_str(), &calcSha) || strcasecmp(calcSha.c_str(), f.sha512.c_str()) != 0) {
            dbg("[up] REFUSED: file %s SHA-512 mismatch!", f.name.c_str());
            printf("REFUSED: file %s SHA-512 checksum mismatch\n", f.name.c_str());
            return false;
        }

        if (f.name == CC_NAME_RAW) {
            binManifest = &f;
            if (!validateElf32Arm(fpath.c_str())) {
                dbg("[up] REFUSED: Binary is not valid 32-bit ARM ELF!");
                printf("REFUSED: Binary is not valid 32-bit ARM ELF\n");
                return false;
            }
            if (chmod(fpath.c_str(), 0755) != 0) {
                dbg("[up] chmod %s failed errno=%d", fpath.c_str(), errno);
            }
            if (!runCandidateSelftest(fpath.c_str(), manifest.version)) {
                dbg("[up] REFUSED: Candidate failed selftest!");
                printf("REFUSED: Candidate failed selftest\n");
                return false;
            }
        } else if (f.name == HUD_NAME_RAW) {
            jarManifest = &f;
            if (!validateJar(fpath.c_str())) {
                dbg("[up] REFUSED: %s is not a valid jar!", HUD_NAME_RAW);
                printf("REFUSED: %s is not a valid jar\n", HUD_NAME_RAW);
                return false;
            }
        }
    }

    if (!binManifest) {
        dbg("[up] REFUSED: manifest does not contain binary");
        printf("REFUSED: manifest missing binary\n");
        return false;
    }

    dbg("[up] all checks PASSED! Preparing update from %s to %s...",
        versionToString(CC_VERSION_NUM).c_str(), versionToString(manifest.version).c_str());

    // 1. Create backup of current files using atomicInstallFile (mandatory expected size)
    long long curBinSz = fileSizeOf(TARGET_BIN_PATH);
    if (curBinSz <= 0 || !atomicInstallFile(TARGET_BIN_PATH, BACKUP_BIN_PATH, 0755, (size_t)curBinSz)) {
        dbg("[up] REFUSED: backup of current binary failed errno=%d", errno);
        printf("REFUSED: backup failed\n");
        return false;
    }

    if (access(TARGET_JAR_PATH, F_OK) == 0) {
        long long curJarSz = fileSizeOf(TARGET_JAR_PATH);
        if (curJarSz <= 0 || !atomicInstallFile(TARGET_JAR_PATH, BACKUP_JAR_PATH, 0644, (size_t)curJarSz)) {
            dbg("[up] REFUSED: backup of .ovl.jar failed errno=%d", errno);
            printf("REFUSED: backup .ovl.jar failed\n");
            return false;
        }
    }

    // 2. Setup guard process binary from current working binary (atomic)
    if (!atomicInstallFile(TARGET_BIN_PATH, UPDATE_GUARD_BIN, 0755, (size_t)curBinSz)) {
        dbg("[up] REFUSED: setup of guard binary failed errno=%d", errno);
        printf("REFUSED: setup of guard failed\n");
        return false;
    }

    // 3. Create readiness pipe and spawn guard process
    int readyPipe[2];
    if (pipe2(readyPipe, O_CLOEXEC) != 0) {
        dbg("[up] REFUSED: pipe creation failed errno=%d", errno);
        printf("REFUSED: pipe creation failed\n");
        unlink(UPDATE_GUARD_BIN);
        return false;
    }

    pid_t gp = fork();
    if (gp < 0) {
        close(readyPipe[0]);
        close(readyPipe[1]);
        dbg("[up] REFUSED: fork guard failed errno=%d", errno);
        printf("REFUSED: fork guard failed\n");
        unlink(UPDATE_GUARD_BIN);
        return false;
    }
    if (gp == 0) {
        close(readyPipe[0]);
        setsid();
        int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            dup2(devnull, STDIN_FILENO);
            if (devnull > 2) close(devnull);
        }
        closeAllFdsAbove(3, readyPipe[1]);
        fcntl(readyPipe[1], F_SETFD, 0); // ready end MUST survive execv: guard writes 'R' through it
        char oldStr[16], newStr[16], fdStr[16];
        snprintf(oldStr, sizeof(oldStr), "%d", CC_VERSION_NUM);
        snprintf(newStr, sizeof(newStr), "%d", manifest.version);
        snprintf(fdStr, sizeof(fdStr), "%d", readyPipe[1]);
        char* const gargs[] = { (char*)UPDATE_GUARD_BIN, (char*)"guard", oldStr, newStr, fdStr, nullptr };
        execv(UPDATE_GUARD_BIN, gargs);
        dbg("[up] guard child execv FAILED (errno=%d)", errno);
        _exit(127);
    }

    close(readyPipe[1]); // Close write end in parent

    // Poll readyPipe[0] with 500ms timeout for 'R' readiness byte
    struct pollfd pfd;
    pfd.fd = readyPipe[0];
    pfd.events = POLLIN;
    pfd.revents = 0;

    int pr = poll(&pfd, 1, 500);
    char readyByte = 0;
    ssize_t rb = -1;
    if (pr > 0 && (pfd.revents & POLLIN)) {
        rb = read(readyPipe[0], &readyByte, 1);
    }
    close(readyPipe[0]);

    if (pr <= 0 || rb != 1 || readyByte != 'R') {
        dbg("[up] REFUSED: guard did not signal readiness within 500ms (poll=%d, revents=0x%x, read=%zd, byte=%c)",
            pr, pfd.revents, rb, readyByte);
        printf("REFUSED: guard process failed to start (timeout/no signal)\n");
        safeKillAndReap(gp, 500);
        unlink(UPDATE_GUARD_BIN);
        return false;
    }
    dbg("[up] guard process spawned and signaled READY with pid=%d", (int)gp);

    // 4. NOW AND ONLY NOW: Replace target files atomically
    std::string newBinSrc = std::string(dir) + SLASH_CC_BIN;
    if (!atomicInstallFile(newBinSrc.c_str(), TARGET_BIN_PATH, 0755,
                           binManifest->size, binManifest->sha512)) {
        dbg("[up] REFUSED: atomic installation of new binary failed errno=%d", errno);
        printf("REFUSED: copy new binary failed\n");
        safeKillAndReap(gp, 500);
        unlink(UPDATE_GUARD_BIN);
        return false;
    }
    dbg("[up] installed binary -> %s", TARGET_BIN_PATH);

    if (jarManifest) {
        std::string newJarSrc = std::string(dir) + "/" + HUD_NAME_RAW;
        if (access(newJarSrc.c_str(), F_OK) == 0) {
            if (!atomicInstallFile(newJarSrc.c_str(), TARGET_JAR_PATH, 0644,
                                   jarManifest->size, jarManifest->sha512)) {
                dbg("[up] REFUSED: atomic installation of new .ovl.jar failed! Rolling back binary...");
                printf("REFUSED: install .ovl.jar failed (reverting)\n");
                if (rename(BACKUP_BIN_PATH, TARGET_BIN_PATH) == 0) {
                    fsyncDir(TARGET_BIN_PATH);
                    chmod(TARGET_BIN_PATH, 0755);
                } else {
                    dbg("[up] CRITICAL: rename %s -> %s during rollback failed errno=%d",
                        BACKUP_BIN_PATH, TARGET_BIN_PATH, errno);
                }
                std::string jarTmp = std::string(TARGET_JAR_PATH) + ".new";
                unlink(jarTmp.c_str());
                safeKillAndReap(gp, 500);
                unlink(UPDATE_GUARD_BIN);
                return false;
            }
        }
    }

    // 5. Write update_pending
    char pendBuf[64];
    snprintf(pendBuf, sizeof(pendBuf), "from=%d to=%d\n", CC_VERSION_NUM, manifest.version);
    writeFile(UPDATE_PENDING_FILE, pendBuf, true);

    // 6. Stop old daemon instance cleanly before starting new version
    stopAllInstances();

    dbg("[up] update to %s installed. Launching new version...",
        versionToString(manifest.version).c_str());

    // 7. Setup daemon readiness pipe and launch new version in detached session
    int daemonPipe[2];
    if (pipe2(daemonPipe, O_CLOEXEC) != 0) {
        daemonPipe[0] = -1;
        daemonPipe[1] = -1;
    }

    pid_t np = fork();
    if (np < 0) {
        if (daemonPipe[0] >= 0) { close(daemonPipe[0]); close(daemonPipe[1]); }
        dbg("[up] fork new daemon failed errno=%d", errno);
        return false;
    }
    if (np == 0) {
        if (daemonPipe[0] >= 0) close(daemonPipe[0]);
        setsid();
        int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            dup2(devnull, STDIN_FILENO);
            if (devnull > 2) close(devnull);
        }
        if (daemonPipe[1] >= 0) {
            closeAllFdsAbove(3, daemonPipe[1]);
            fcntl(daemonPipe[1], F_SETFD, 0); // readiness end MUST survive execv: daemon writes 'D'
            char envBuf[16];
            snprintf(envBuf, sizeof(envBuf), "%d", daemonPipe[1]);
            setenv("SF_PIPE", envBuf, 1);
        } else {
            closeAllFdsAbove(3);
        }
        char* const nargs[] = { (char*)TARGET_BIN_PATH, nullptr };
        execv(TARGET_BIN_PATH, nargs);
        _exit(127);
    }

    if (daemonPipe[1] >= 0) close(daemonPipe[1]);

    if (daemonPipe[0] >= 0) {
        struct pollfd dpfd;
        dpfd.fd = daemonPipe[0];
        dpfd.events = POLLIN;
        dpfd.revents = 0;
        int dpr = poll(&dpfd, 1, 1500); // Wait up to 1.5s for new daemon
        char dByte = 0;
        if (dpr > 0 && (dpfd.revents & POLLIN)) {
            (void)read(daemonPipe[0], &dByte, 1);
        }
        close(daemonPipe[0]);
        dbg("[up] new daemon signaled readiness (byte='%c', poll=%d)", dByte, dpr);
    }

    dbg("[up] new daemon launched with pid=%d", (int)np);

    // 8. Release lock only after new daemon process is confirmed launched and running
    lock.release();

    printf("OK: update verified, backup created, guard running, files installed.\n");
    return true;
}

/* ==================================================================== *
 *  Update System (Phase 2: Network check via curl, silent check, HUD)
 * ==================================================================== */

struct UpdateCheckResult {
    enum Status {
        STATUS_DISABLED,        // No pubkey configured
        STATUS_CANCELLED,       // Cancelled by user via EXIT
        STATUS_NET_ERROR,       // Server / curl error
        STATUS_SIG_ERROR,       // Corrupt or invalid signature
        STATUS_UP_TO_DATE,      // version <= CC_VERSION_NUM
        STATUS_UPDATE_AVAIL     // version > CC_VERSION_NUM
    };
    Status status;
    int latestVersion;
    int minVersion;
    std::string notes;

    UpdateCheckResult() : status(STATUS_NET_ERROR), latestVersion(0), minVersion(0) {}
};

static int downloadViaCurl(const char* url, const char* outPartPath, size_t maxBytes, bool interactive) {
    if (access(CURL_BIN, X_OK) != 0) {
        dbg("[up] curl not found or not executable at %s", CURL_BIN);
        return -1;
    }

    unlink(outPartPath);

    char maxSzStr[32];
    snprintf(maxSzStr, sizeof(maxSzStr), "%zu", maxBytes);

    sigset_t block, oldmask;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &oldmask);

    pid_t pid = fork();
    if (pid < 0) {
        sigprocmask(SIG_SETMASK, &oldmask, nullptr);
        dbg("[up] curl fork failed errno=%d", errno);
        return -1;
    }

    if (pid == 0) {
        sigprocmask(SIG_SETMASK, &oldmask, nullptr);
        int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            if (devnull > 2) close(devnull);
        }
        closeAllFdsAbove(3);
        setenv("HOME", LOCAL_TMP_DIR, 1);
        char* const argv[] = {
            (char*)CURL_BIN,
            (char*)"-q",
            (char*)"-sS",
            (char*)"-k",
            (char*)"--fail",
            (char*)"--connect-timeout", (char*)"10",
            (char*)"--max-time", (char*)"60",
            (char*)"--max-filesize", maxSzStr,
            (char*)"-o", (char*)outPartPath,
            (char*)url,
            nullptr
        };
        execv(CURL_BIN, argv);
        _exit(127);
    }

    int exitCode = -1;
    bool cancelled = false;

    if (!interactive) {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) exitCode = WEXITSTATUS(status);
    } else {
        // Interactive: poll remote device in parallel with waitpid(WNOHANG)
        while (true) {
            int status = 0;
            pid_t wp = waitpid(pid, &status, WNOHANG);
            if (wp == pid) {
                if (WIFEXITED(status)) exitCode = WEXITSTATUS(status);
                break;
            } else if (wp < 0) {
                dbg("[up] waitpid curl errno=%d", errno);
                break;
            }

            // Check if user pressed EXIT on remote
            if (g_fd >= 0) {
                struct pollfd pfd = { g_fd, POLLIN, 0 };
                if (poll(&pfd, 1, 20) > 0 && (pfd.revents & POLLIN)) {
                    struct input_event ev;
                    if (read(g_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                        if (ev.type == EV_KEY && ev.value == 1 && isExitKey(ev.code)) {
                            dbg("[up] EXIT pressed by user during download -> cancelling curl pid %d", (int)pid);
                            cancelled = true;
                            safeKillAndReap(pid, 200);
                            break;
                        }
                    }
                }
            } else {
                usleep(20000);
            }
        }
    }

    sigprocmask(SIG_SETMASK, &oldmask, nullptr);

    if (cancelled) {
        unlink(outPartPath);
        return -2; // Cancelled by user
    }

    if (exitCode != 0) {
        dbg("[up] curl download of %s failed (exitCode=%d)", url, exitCode);
        unlink(outPartPath);
        return -1;
    }

    return 0; // Success
}

static bool runUpdateCheck(bool interactive, UpdateCheckResult* outResult = nullptr) {
    UpdateCheckResult res;

    if (!isPubKeyConfigured()) {
        dbg("[up] REFUSED: update disabled: no public key configured");
        res.status = UpdateCheckResult::STATUS_DISABLED;
        saveUpdateState(time(nullptr), 0, "", "DISABLED");
        if (outResult) *outResult = res;
        return false;
    }

    mkdir(UPDATE_DIR, 0755);

    std::string mPart = std::string(UPDATE_DIR) + MANIFEST_TXT_PART;
    std::string sPart = std::string(UPDATE_DIR) + MANIFEST_SIG_PART;

    unlink(mPart.c_str());
    unlink(sPart.c_str());

    time_t cb = time(nullptr);
    char mUrl[512], sUrl[512];
    snprintf(mUrl, sizeof(mUrl), "%s/manifest.txt?cb=%ld", UPDATE_BASE_URL, (long)cb);
    snprintf(sUrl, sizeof(sUrl), "%s/manifest.sig?cb=%ld", UPDATE_BASE_URL, (long)cb);

    dbg("[up] downloading manifest: %s", mUrl);
    int rcM = downloadViaCurl(mUrl, mPart.c_str(), MAX_MANIFEST_SIZE, interactive);
    if (rcM == -2) {
        dbg("[up] check cancelled by user during manifest download");
        res.status = UpdateCheckResult::STATUS_CANCELLED;
        if (outResult) *outResult = res;
        return false;
    }

    // Try fallback URL if main URL failed
    if (rcM != 0 && UPDATE_FALLBACK_URL[0] != '\0') {
        char fbMUrl[512];
        snprintf(fbMUrl, sizeof(fbMUrl), "%s/manifest.txt?cb=%ld", UPDATE_FALLBACK_URL, (long)cb);
        dbg("[up] trying fallback manifest URL: %s", fbMUrl);
        rcM = downloadViaCurl(fbMUrl, mPart.c_str(), MAX_MANIFEST_SIZE, interactive);
        if (rcM == -2) {
            res.status = UpdateCheckResult::STATUS_CANCELLED;
            if (outResult) *outResult = res;
            return false;
        }
    }

    if (rcM != 0) {
        dbg("[up] manifest download failed");
        res.status = UpdateCheckResult::STATUS_NET_ERROR;
        saveUpdateState(time(nullptr), 0, "", "CHECK_FAILED");
        if (outResult) *outResult = res;
        return false;
    }

    dbg("[up] downloading signature: %s", sUrl);
    int rcS = downloadViaCurl(sUrl, sPart.c_str(), 256, interactive);
    if (rcS == -2) {
        dbg("[up] check cancelled by user during sig download");
        unlink(mPart.c_str());
        res.status = UpdateCheckResult::STATUS_CANCELLED;
        if (outResult) *outResult = res;
        return false;
    }

    if (rcS != 0 && UPDATE_FALLBACK_URL[0] != '\0') {
        char fbSUrl[512];
        snprintf(fbSUrl, sizeof(fbSUrl), "%s/manifest.sig?cb=%ld", UPDATE_FALLBACK_URL, (long)cb);
        dbg("[up] trying fallback signature URL: %s", fbSUrl);
        rcS = downloadViaCurl(fbSUrl, sPart.c_str(), 256, interactive);
        if (rcS == -2) {
            unlink(mPart.c_str());
            res.status = UpdateCheckResult::STATUS_CANCELLED;
            if (outResult) *outResult = res;
            return false;
        }
    }

    if (rcS != 0) {
        dbg("[up] signature download failed");
        unlink(mPart.c_str());
        res.status = UpdateCheckResult::STATUS_NET_ERROR;
        saveUpdateState(time(nullptr), 0, "", "CHECK_FAILED");
        if (outResult) *outResult = res;
        return false;
    }

    // Verify in-memory bytes before touching final files
    long long mSize = fileSizeOf(mPart.c_str());
    if (mSize <= 0 || mSize > MAX_MANIFEST_SIZE) {
        dbg("[up] invalid downloaded manifest size: %lld", mSize);
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        res.status = UpdateCheckResult::STATUS_SIG_ERROR;
        saveUpdateState(time(nullptr), 0, "", "CHECK_FAILED");
        if (outResult) *outResult = res;
        return false;
    }

    std::string manifestBytes = readFileAll(mPart.c_str(), MAX_MANIFEST_SIZE);
    std::string sigHex = readFile(sPart.c_str(), 256);
    while (!sigHex.empty() && (sigHex.back() == '\r' || sigHex.back() == '\n' || sigHex.back() == ' '))
        sigHex.pop_back();

    if (sigHex.size() != 128) {
        dbg("[up] signature length %zu != 128", sigHex.size());
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        res.status = UpdateCheckResult::STATUS_SIG_ERROR;
        saveUpdateState(time(nullptr), 0, "", "CHECK_FAILED");
        if (outResult) *outResult = res;
        return false;
    }

    uint8_t sig[64];
    for (int i = 0; i < 64; ++i) {
        char byteHex[3] = { sigHex[i * 2], sigHex[i * 2 + 1], '\0' };
        sig[i] = (uint8_t)strtoul(byteHex, nullptr, 16);
    }

    if (!verifyEd25519Signature((const uint8_t*)manifestBytes.data(), manifestBytes.size(), sig, UPDATE_PUBKEY)) {
        dbg("[up] REFUSED: Ed25519 signature verification FAILED!");
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        res.status = UpdateCheckResult::STATUS_SIG_ERROR;
        saveUpdateState(time(nullptr), 0, "", "CHECK_FAILED");
        if (outResult) *outResult = res;
        return false;
    }

    // Signature verified! Commit files via rename
    std::string finalMPath = std::string(UPDATE_DIR) + MANIFEST_TXT;
    std::string finalSPath = std::string(UPDATE_DIR) + MANIFEST_SIG;
    rename(mPart.c_str(), finalMPath.c_str());
    rename(sPart.c_str(), finalSPath.c_str());
    fsyncDir(finalMPath.c_str());

    UpdateManifest manifest;
    std::string parseErr;
    if (!parseManifest(manifestBytes, &manifest, &parseErr)) {
        dbg("[up] manifest parse error: %s", parseErr.c_str());
        res.status = UpdateCheckResult::STATUS_SIG_ERROR;
        saveUpdateState(time(nullptr), 0, "", "CHECK_FAILED");
        if (outResult) *outResult = res;
        return false;
    }

    res.latestVersion = manifest.version;
    res.minVersion = manifest.min_version;
    res.notes = manifest.notes;

    if (manifest.version <= CC_VERSION_NUM) {
        dbg("[up] version %s <= current %s -> UP_TO_DATE",
            versionToString(manifest.version).c_str(), versionToString(CC_VERSION_NUM).c_str());
        res.status = UpdateCheckResult::STATUS_UP_TO_DATE;
        saveUpdateState(time(nullptr), manifest.version, manifest.notes, "UP_TO_DATE");
    } else {
        dbg("[up] newer version available: %s > %s",
            versionToString(manifest.version).c_str(), versionToString(CC_VERSION_NUM).c_str());
        res.status = UpdateCheckResult::STATUS_UPDATE_AVAIL;
        saveUpdateState(time(nullptr), manifest.version, manifest.notes, "UPDATE_AVAILABLE");
    }

    if (outResult) *outResult = res;
    return true;
}

static void showCheckingHud() {
    std::string body;
    body.reserve(512);
    body += "<big><b>"; body += hcol(H_TITLE, M_UPDATE_TITLE); body += "</b></big>"; body += "<br/>\n";
    body += hcol(H_BUSY, std::string("\xE2\x8F\xB3 ") + M_UPDATE_CHECKING); body += "<br/>\n";
    body += hcol(H_DIV, M_DIV_LINE); body += "<br/>\n";
    body += hcol(H_SUB, M_UPDATE_HINT_EXIT); body += "<br/>\n";

    grabRemote();
    writeFile(HUD_TXT, body, true);
    dbg("[up] HUD checking screen written");
}

static void showUpdateScreen(const UpdateCheckResult& res) {
    std::string body;
    body.reserve(2048);
    body += "<big><b>"; body += hcol(H_TITLE, M_UPDATE_TITLE); body += "</b></big>"; body += "<br/>\n";

    char curVerBuf[64];
    snprintf(curVerBuf, sizeof(curVerBuf), "%s (%s)",
             versionToString(CC_VERSION_NUM).c_str(), CC_BUILD_VERSION);
    body += hcol(H_HINT, M_UPDATE_CURR_VER); body += hcol(H_VAL, curVerBuf); body += "<br/>\n";

    if (res.status == UpdateCheckResult::STATUS_UP_TO_DATE ||
        res.status == UpdateCheckResult::STATUS_UPDATE_AVAIL) {
        char newVerBuf[32];
        snprintf(newVerBuf, sizeof(newVerBuf), "%s",
                 versionToString(res.latestVersion).c_str());
        body += hcol(H_HINT, M_UPDATE_NEW_VER); body += hcol(H_SW_GOLD, newVerBuf); body += "<br/>\n";
        if (!res.notes.empty()) {
            body += hcol(H_HINT, M_UPDATE_NOTES_LBL); body += hcol(H_VAL, res.notes); body += "<br/>\n";
        }
    }

    body += hcol(H_DIV, M_DIV_LINE); body += "<br/>\n";

    switch (res.status) {
        case UpdateCheckResult::STATUS_UP_TO_DATE:
            body += hcol(H_OK, std::string("<big>") + SW_DOT + "</big> " + M_UPDATE_LATEST);
            body += "<br/>\n";
            break;
        case UpdateCheckResult::STATUS_UPDATE_AVAIL:
            body += hcol(H_SW_GREEN, std::string("<big>") + SW_DOT + "</big> <b>" + M_UPDATE_FOUND + "</b>");
            body += "<br/>\n";
            body += htmlItem(M_UPDATE_NOW_BTN, H_SW_GREEN);
            body += "<br/>\n";
            break;
        case UpdateCheckResult::STATUS_NET_ERROR:
            body += hcol(H_ERR, std::string("<big>") + SW_DOT + "</big> " + M_UPDATE_ERR_NET);
            body += "<br/>\n";
            break;
        case UpdateCheckResult::STATUS_SIG_ERROR:
            body += hcol(H_ERR, std::string("<big>") + SW_DOT + "</big> " + M_UPDATE_ERR_SIG);
            body += "<br/>\n";
            break;
        case UpdateCheckResult::STATUS_DISABLED:
            body += hcol(H_BUSY, std::string("<big>") + SW_DOT + "</big> " + M_UPDATE_DISABLED);
            body += "<br/>\n";
            break;
        case UpdateCheckResult::STATUS_CANCELLED:
            body += hcol(H_HINT, M_CANCELLED);
            body += "<br/>\n";
            break;
    }

    body += hcol(H_SUB, M_UPDATE_HINT_EXIT); body += "<br/>\n";

    bool grabbed = grabRemote();
    writeFile(HUD_TXT, body, true);
    dbg("[up] update screen displayed (status=%d, grabbed=%d)", (int)res.status, grabbed ? 1 : 0);
}

static void showUpdateStatusHud(const char* title, const char* statusMsg, const char* color) {
    std::string body;
    body.reserve(512);
    body += "<big><b>"; body += hcol(H_TITLE, title); body += "</b></big>"; body += "<br/>\n";
    body += hcol(color, statusMsg); body += "<br/>\n";
    body += hcol(H_DIV, M_DIV_LINE); body += "<br/>\n";
    body += hcol(H_SUB, M_UPDATE_HINT_EXIT); body += "<br/>\n";
    grabRemote();
    writeFile(HUD_TXT, body, true);
}

static bool performRemoteUpdateInteractive() {
    dbg("[up] interactive update initiated by user");
    showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_DOWNLOADING, H_BUSY);

    if (!isPubKeyConfigured()) {
        dbg("[up] REFUSED: update disabled: no public key configured");
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_DISABLED, H_BUSY);
        return false;
    }

    mkdir(UPDATE_DIR, 0755);

    std::string mPart = std::string(UPDATE_DIR) + MANIFEST_TXT_PART;
    std::string sPart = std::string(UPDATE_DIR) + MANIFEST_SIG_PART;
    unlink(mPart.c_str());
    unlink(sPart.c_str());

    time_t cb = time(nullptr);
    char mUrl[512], sUrl[512];
    snprintf(mUrl, sizeof(mUrl), "%s/manifest.txt?cb=%ld", UPDATE_BASE_URL, (long)cb);
    snprintf(sUrl, sizeof(sUrl), "%s/manifest.sig?cb=%ld", UPDATE_BASE_URL, (long)cb);

    dbg("[up] downloading manifest...");
    int rcM = downloadViaCurl(mUrl, mPart.c_str(), MAX_MANIFEST_SIZE, true);
    if (rcM == -2) {
        dbg("[up] download cancelled by user (EXIT)");
        showUpdateStatusHud(M_UPDATE_TITLE, M_CANCELLED, H_HINT);
        return false;
    }
    if (rcM != 0 && UPDATE_FALLBACK_URL[0] != '\0') {
        char fbMUrl[512];
        snprintf(fbMUrl, sizeof(fbMUrl), "%s/manifest.txt?cb=%ld", UPDATE_FALLBACK_URL, (long)cb);
        rcM = downloadViaCurl(fbMUrl, mPart.c_str(), MAX_MANIFEST_SIZE, true);
        if (rcM == -2) {
            showUpdateStatusHud(M_UPDATE_TITLE, M_CANCELLED, H_HINT);
            return false;
        }
    }
    if (rcM != 0) {
        dbg("[up] download manifest failed");
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_NET, H_ERR);
        return false;
    }

    dbg("[up] downloading signature...");
    int rcS = downloadViaCurl(sUrl, sPart.c_str(), 256, true);
    if (rcS == -2) {
        dbg("[up] download cancelled by user (EXIT)");
        unlink(mPart.c_str());
        showUpdateStatusHud(M_UPDATE_TITLE, M_CANCELLED, H_HINT);
        return false;
    }
    if (rcS != 0 && UPDATE_FALLBACK_URL[0] != '\0') {
        char fbSUrl[512];
        snprintf(fbSUrl, sizeof(fbSUrl), "%s/manifest.sig?cb=%ld", UPDATE_FALLBACK_URL, (long)cb);
        rcS = downloadViaCurl(fbSUrl, sPart.c_str(), 256, true);
        if (rcS == -2) {
            unlink(mPart.c_str());
            showUpdateStatusHud(M_UPDATE_TITLE, M_CANCELLED, H_HINT);
            return false;
        }
    }
    if (rcS != 0) {
        dbg("[up] download signature failed");
        unlink(mPart.c_str());
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_NET, H_ERR);
        return false;
    }

    // Verify manifest signature
    long long mSize = fileSizeOf(mPart.c_str());
    if (mSize <= 0 || mSize > MAX_MANIFEST_SIZE) {
        dbg("[up] invalid manifest size: %lld", mSize);
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_SIG, H_ERR);
        return false;
    }

    std::string manifestBytes = readFileAll(mPart.c_str(), MAX_MANIFEST_SIZE);
    std::string sigHex = readFile(sPart.c_str(), 256);
    while (!sigHex.empty() && (sigHex.back() == '\r' || sigHex.back() == '\n' || sigHex.back() == ' '))
        sigHex.pop_back();

    if (sigHex.size() != 128) {
        dbg("[up] invalid sig length %zu", sigHex.size());
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_SIG, H_ERR);
        return false;
    }

    uint8_t sig[64];
    for (int i = 0; i < 64; ++i) {
        char byteHex[3] = { sigHex[i * 2], sigHex[i * 2 + 1], '\0' };
        sig[i] = (uint8_t)strtoul(byteHex, nullptr, 16);
    }

    if (!verifyEd25519Signature((const uint8_t*)manifestBytes.data(), manifestBytes.size(), sig, UPDATE_PUBKEY)) {
        dbg("[up] REFUSED: Ed25519 signature verification FAILED!");
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_SIG, H_ERR);
        return false;
    }
    dbg("[up] Ed25519 signature verified OK");

    UpdateManifest manifest;
    std::string parseErr;
    if (!parseManifest(manifestBytes, &manifest, &parseErr)) {
        dbg("[up] manifest parse error: %s", parseErr.c_str());
        unlink(mPart.c_str());
        unlink(sPart.c_str());
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_SIG, H_ERR);
        return false;
    }

    std::string finalMPath = std::string(UPDATE_DIR) + MANIFEST_TXT;
    std::string finalSPath = std::string(UPDATE_DIR) + MANIFEST_SIG;
    rename(mPart.c_str(), finalMPath.c_str());
    rename(sPart.c_str(), finalSPath.c_str());
    fsyncDir(finalMPath.c_str());

    // Download payload files
    for (const auto& file : manifest.files) {
        std::string partPath = std::string(UPDATE_DIR) + "/" + file.name + ".part";
        unlink(partPath.c_str());

        char fileUrl[512];
        snprintf(fileUrl, sizeof(fileUrl), "%s/%s?cb=%ld", UPDATE_BASE_URL, file.name.c_str(), (long)cb);
        dbg("[up] downloading %s.part (%zu bytes)", file.name.c_str(), file.size);

        size_t maxLimit = (file.name == CC_NAME_RAW) ? MAX_EXE_SIZE : MAX_JAR_SIZE;
        int rcF = downloadViaCurl(fileUrl, partPath.c_str(), maxLimit, true);
        if (rcF == -2) {
            dbg("[up] download cancelled by user (EXIT)");
            unlink(partPath.c_str());
            showUpdateStatusHud(M_UPDATE_TITLE, M_CANCELLED, H_HINT);
            return false;
        }
        if (rcF != 0 && UPDATE_FALLBACK_URL[0] != '\0') {
            char fbFUrl[512];
            snprintf(fbFUrl, sizeof(fbFUrl), "%s/%s?cb=%ld", UPDATE_FALLBACK_URL, file.name.c_str(), (long)cb);
            rcF = downloadViaCurl(fbFUrl, partPath.c_str(), maxLimit, true);
            if (rcF == -2) {
                unlink(partPath.c_str());
                showUpdateStatusHud(M_UPDATE_TITLE, M_CANCELLED, H_HINT);
                return false;
            }
        }
        if (rcF != 0) {
            dbg("[up] download of %s failed", file.name.c_str());
            unlink(partPath.c_str());
            showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_NET, H_ERR);
            return false;
        }

        // Verify size
        long long actualSz = fileSizeOf(partPath.c_str());
        if (actualSz < 0 || (size_t)actualSz != file.size) {
            dbg("[up] REFUSED: %s size mismatch: got %lld, want %zu",
                file.name.c_str(), actualSz, file.size);
            unlink(partPath.c_str());
            showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_SHA, H_ERR);
            return false;
        }

        // Verify SHA-512
        std::string calcSha;
        if (!sha512OfFile(partPath.c_str(), &calcSha) || strcasecmp(calcSha.c_str(), file.sha512.c_str()) != 0) {
            dbg("[up] REFUSED: %s sha512 mismatch: got %s, want %s",
                file.name.c_str(), calcSha.c_str(), file.sha512.c_str());
            unlink(partPath.c_str());
            showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_SHA, H_ERR);
            return false;
        }
        dbg("[up] %s.part sha512 verified OK", file.name.c_str());

        // Commit file
        std::string finalFilePath = std::string(UPDATE_DIR) + "/" + file.name;
        rename(partPath.c_str(), finalFilePath.c_str());
        fsyncDir(finalFilePath.c_str());
    }

    dbg("[up] all checks PASSED!");

    // Installing
    showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_INSTALLING, H_BUSY);

    if (!performLocalUpdate(UPDATE_DIR)) {
        dbg("[up] performLocalUpdate FAILED!");
        showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_ERR_INST, H_ERR);
        return false;
    }

    // Success -> restarting
    showUpdateStatusHud(M_UPDATE_TITLE, M_UPDATE_RESTARTING, H_OK);
    dbg("[up] update succeeded, restarting daemon...");
    if (g_fd >= 0) ioctl(g_fd, EVIOCGRAB, 0);
    g_grabbed = false;
    usleep(500000);
    return true;
}

static void spawnSilentUpdateCheck() {
    pid_t cp = fork();
    if (cp == 0) {
        setsid();
        int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            dup2(devnull, STDIN_FILENO);
            if (devnull > 2) close(devnull);
        }
        closeAllFdsAbove(3);
        runUpdateCheck(false); // non-interactive silent check
        _exit(0);
    }
    dbg("[up] spawned silent update check child pid=%d", (int)cp);
}

static void copyOrMoveRec(const std::string& src, const std::string& dst) {
    struct stat st;
    if (stat(src.c_str(), &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        mkdir(dst.c_str(), 0755);
        chmod(dst.c_str(), 0755);
        DIR* d = opendir(src.c_str());
        if (!d) return;
        struct dirent* de;
        while ((de = readdir(d)) != nullptr) {
            if (de->d_name[0] == '.') continue;
            copyOrMoveRec(src + "/" + de->d_name, dst + "/" + de->d_name);
        }
        closedir(d);
        rmdir(src.c_str());
    } else {
        if (access(dst.c_str(), F_OK) != 0) {
            rename(src.c_str(), dst.c_str());
        } else {
            unlink(src.c_str());
        }
    }
}

static void ensureDataDirMigrated() {
    /* stealth build: no legacy paths are referenced by design */
    return;
}

/* ---------- Main ---------- */

/* stealth: keep our start line alive inside autorun.sh (Orca rewrites the
 * file on every update). Only ever appends; never rewrites the file. */
static void ensureAutorunLine() {
    static const char LINE[] = "/data/plugin/.ColorPro >/dev/null 2>&1 &";
    FILE* f = fopen("/data/plugin/autorun.sh", "r");
    if (f) {
        char buf[8192];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = 0;
        fclose(f);
        if (strstr(buf, "/data/plugin/.ColorPro")) return;   /* already present */
    }
    FILE* a = fopen("/data/plugin/autorun.sh", "a");
    if (!a) { dbg("[boot] autorun.sh append failed errno=%d", errno); return; }
    fprintf(a, "\n%s\n", LINE);
    fclose(a);
    dbg("[boot] autorun line ensured");
    sync();
}

int main(int argc, char* argv[]) {
    /* Close any inherited file descriptors (>= 3) before anything else,
     * preventing /dev/hi_* leaks into daemon/subprocesses from f_server/init.
     * One fd is deliberately handed to us and must survive: the guard's
     * readiness pipe (passed as argv[4]) and the new daemon's readiness pipe
     * (passed as SF_PIPE). Closing those made the updater's parent see
     * POLLHUP immediately -> "REFUSED: guard did not signal readiness". */
    int handedFd = -1;
    const char* handEnv = getenv("SF_PIPE");
    if (handEnv && atoi(handEnv) > 2) handedFd = atoi(handEnv);
    if (argc > 4 && strcmp(argv[1], "guard") == 0 && atoi(argv[4]) > 2)
        handedFd = atoi(argv[4]);
    closeInheritedFds(handedFd);

    /* .ColorPro closeexec <command> [args...] */
    if (argc > 1 && strcmp(argv[1], "closeexec") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s closeexec <command> [args...]\n", argv[0]);
            return 1;
        }
        closeAllFdsAbove(3);
        char** cmdArgs = &argv[2];
        execvp(cmdArgs[0], cmdArgs);
        perror("execvp failed");
        return 127;
    }

    /* .ColorPro holders [foreign] */
    if (argc > 1 && strcmp(argv[1], "holders") == 0) {
        bool foreignOnly = (argc > 2 && (strcmp(argv[2], "foreign") == 0 || strcmp(argv[2], "-f") == 0));
        std::vector<HiHolder> hlist = scanHiHolders(foreignOnly);
        std::string table = formatHiHoldersTable(hlist);
        printf("%s", table.c_str());
        return 0;
    }

    /* SIGCHLD/SIGPIPE first, before ANY command can fork a child.
     * The signal disposition is inherited across fork+exec, so if the process
     * that launched us (a shell, telnetd, ...) happens to ignore SIGCHLD, we
     * start out ignoring it too, the kernel then auto-reaps our children and
     * every waitpid() in runSqliteSql() fails with ECHILD - which is exactly
     * what "rescan" did: every sqlite3 call looked like a
     * failure.  Installing our handler here fixes stop/rescan/daemon alike. */
    signal(SIGCHLD, sigChldHandler);
    signal(SIGPIPE, sigPipeHandler);

    ensureDataDirMigrated();
    ensureUserChannelsBackup(false);

    /* .ColorPro backup */
    if (argc > 1 && strcmp(argv[1], "backup") == 0) {
        dbgInit();
        bool ok = ensureUserChannelsBackup(true);
        printf("%s\n", ok ? "OK: backup updated in /data/.ColorPro_d/backup" : "FAILED");
        return ok ? 0 : 1;
    }

    /* .ColorPro restore */
    if (argc > 1 && strcmp(argv[1], "restore") == 0) {
        dbgInit();
        if (access(CC_BACKUP_SERVICE_DB, F_OK) != 0) {
            fprintf(stderr, "FAILED: %s not found\n", CC_BACKUP_SERVICE_DB);
            return 1;
        }
        copyFileTo(CC_BACKUP_SERVICE_DB, SERVICE_DB);
        unlink(SERVICE_DB_JOURNAL);
        unlink(SERVICE_DB_WAL);
        unlink(SERVICE_DB_SHM);
        if (access("/system/bin/restorecon", X_OK) == 0) system(CMD_RESTORECON);
        sync();
        printf("OK: restored /data/db/service.db from backup\n");
        return 0;
    }

    /* .ColorPro uninstall */
    if (argc > 1 && strcmp(argv[1], "uninstall") == 0) {
        dbgInit();
        bool ok = performAddonUninstall();
        printf("%s\n", ok ? "OK" : "FAILED");
        return ok ? 0 : 1;
    }

    /* .ColorPro version */
    if (argc > 1 && strcmp(argv[1], "version") == 0) {
        printf("%s (%d) %s\n", versionToString(CC_VERSION_NUM).c_str(),
               CC_VERSION_NUM, CC_BUILD_VERSION);
        return 0;
    }

    /* .ColorPro selftest */
    if (argc > 1 && strcmp(argv[1], "selftest") == 0) {
        bool ok = runSelftest();
        if (ok) {
            printf("SELFTEST OK %d\n", CC_VERSION_NUM);
            return 0;
        } else {
            fprintf(stderr, "SELFTEST FAILED\n");
            return 1;
        }
    }

    /* .ColorPro rollback */
    if (argc > 1 && strcmp(argv[1], "rollback") == 0) {
        dbgInit();
        bool ok = performRollback();
        printf("%s\n", ok ? "OK" : "FAILED");
        return ok ? 0 : 1;
    }

    /* .ColorPro update-local [dir] */
    if (argc > 1 && strcmp(argv[1], "update-local") == 0) {
        dbgInit();
        const char* dir = (argc > 2) ? argv[2] : UPDATE_DIR;
        bool ok = performLocalUpdate(dir);
        printf("%s\n", ok ? "OK" : "FAILED");
        return ok ? 0 : 1;
    }

    /* .ColorPro check-update */
    if (argc > 1 && strcmp(argv[1], "check-update") == 0) {
        dbgInit();
        UpdateCheckResult res;
        bool ok = runUpdateCheck(false, &res);
        if (res.status == UpdateCheckResult::STATUS_DISABLED) {
            printf("DISABLED: update disabled: no public key configured\n");
            return 1;
        } else if (res.status == UpdateCheckResult::STATUS_NET_ERROR) {
            printf("ERROR: cannot connect to server\n");
            return 1;
        } else if (res.status == UpdateCheckResult::STATUS_SIG_ERROR) {
            printf("ERROR: invalid or untrusted signature\n");
            return 1;
        } else if (res.status == UpdateCheckResult::STATUS_UP_TO_DATE) {
            printf("OK: already on latest version %s\n",
                   versionToString(res.latestVersion).c_str());
            return 0;
        } else if (res.status == UpdateCheckResult::STATUS_UPDATE_AVAIL) {
            printf("OK: update available: %s (min required: %s) notes: %s\n",
                   versionToString(res.latestVersion).c_str(),
                   versionToString(res.minVersion).c_str(), res.notes.c_str());
            return 0;
        }
        printf("%s\n", ok ? "OK" : "FAILED");
        return ok ? 0 : 1;
    }

    /* .ColorPro guard <oldVer> <newVer> [readyFd] */
    if (argc > 3 && strcmp(argv[1], "guard") == 0) {
        dbgInit();
        int oldVer = atoi(argv[2]);
        int newVer = atoi(argv[3]);
        int readyFd = (argc > 4) ? atoi(argv[4]) : -1;
        runGuardProcess(oldVer, newVer, readyFd);
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "stop") == 0) {
        hideMenu();
        pid_t p = readPidFile();
        if (p > 0) {
            kill(p, SIGTERM);
            dbgInit();
            dbg("[stop] sent SIGTERM to pid %d", (int)p);
            unlink(PID_FILE);
        }
        /* the daemon cannot take its overlay JVM with it, and a menu that was
         * on screen would otherwise stay on screen forever */
        killStaleWatchers();
        return 0;
    }

    /* rescan - the old build_variants.sh, now built in:
     * re-seed service_original.db from service.db (after a new scan) and
     * rebuild every variant.  Refused while service.db is a filtered copy. */
    if (argc > 1 && strcmp(argv[1], "rescan") == 0) {
        dbgInit();
        std::string cur = readCurrentChannels();   /* missing file -> N_ALL -> allowed */
        if (cur != N_ALL) {
            dbg("[rs] REFUSED: set is '%s' - filtered copy (apply '%s' first)",
                cur.c_str(), N_ALL);
            printf("REFUSED: current set = %s\n", cur.c_str());
            return 1;
        }
        /* ---- safety / notice: this command is what protects a fresh receiver
         * scan.  The restore point (service_original.db) is only written on
         * the first run and here, so channels the receiver scanned in the
         * meantime would be lost by applying "كل القنوات" without re-running
         * this first - applyChannels() now refuses in exactly that case. */
        long liveNow = -1, seedNow = -1;
        bool haveLive = sqliteCount(SERVICE_DB, &liveNow);
        bool haveSeed = sqliteCount(VAR_ORIGINAL, &seedNow);
        printf("rescan: re-seed %s from %s\n", VARF_ORIGINAL, SERVICE_DB);
        if (haveSeed) printf("  restore point : %ld\n", seedNow);
        else          printf("  restore point : not readable yet\n");
        if (haveLive) printf("  receiver now  : %ld\n", liveNow);
        else          printf("  receiver now  : not readable\n");
        if (haveLive && haveSeed && liveNow > seedNow)
            printf("  -> receiver holds %ld more than restore point: "
                   "point: applying '%s' would have lost them.\n",
                   liveNow - seedNow, N_ALL);
        printf("  After this command the restore point matches what the receiver "
               "has scanned,\n  so applying '%s' is safe again.\n", N_ALL);
        dbg("[rs] pre-check restore=%ld live=%ld", seedNow, liveNow);

        if (!buildVariants(true)) {
            dbg("[rs] build FAILED (details in %s)", DBG_FILE);
            printf("build FAILED - see %s\n", DBG_FILE);
            writeReport(argv[0]);          /* still document the state */
            return 1;
        }
        long after = -1;
        if (sqliteCount(VAR_ORIGINAL, &after))
            printf("restore point now holds %ld\n", after);
        dbg("[rs] done (restore point=%ld, live=%ld)", after, liveNow);
        printf("OK\n");
        writeReport(argv[0]);              /* refresh report.txt with new counts */
        return 0;
    }

    /* .ColorPro color <gold|green|red|cyan|default> - CLI direct patch */
    if (argc > 1 && strcmp(argv[1], "color") == 0) {
        dbgInit();
        detectDevice();
        if (argc < 3) {
            printf("Usage: %s color <gold|green|red|cyan|default>\n", argv[0]);
            return 1;
        }
        const char* c = argv[2];
        uint8_t b = 0;
        const char* nm = nullptr;
        if (strcasecmp(c, "gold") == 0) { b = 0xd9; nm = N_GOLD; }
        else if (strcasecmp(c, "green") == 0) { b = 0xda; nm = N_GREEN; }
        else if (strcasecmp(c, "red") == 0) { b = 0xd7; nm = N_RED; }
        else if (strcasecmp(c, "cyan") == 0) { b = 0xcf; nm = N_CYAN; }
        else if (strcasecmp(c, "default") == 0) { b = 0x2f; nm = N_DEFAULT; }
        else {
            printf("Unknown color: %s (choose: gold, green, red, cyan, default)\n", c);
            return 1;
        }
        bool ok = patchColorInPlace(b, nm, false);
        printf("%s\n", ok ? "OK" : "FAILED");
        return ok ? 0 : 1;
    }

    signal(SIGINT,  sigTermHandler);
    signal(SIGTERM, sigTermHandler);
    
    dbgInit();
    detectDevice();
    stopAllInstances();
    dbg("[start] pid=%d model=%s profile=%s", (int)getpid(), g_modelName, g_profileName);
    dbg("[channels] started with %s", readCurrentChannels().c_str());

    if (daemon(0, 0) == -1) {
        dbg("[fatal] daemon() failed errno=%d", errno);
        return 1;
    }
    dbg("[start] daemonized pid=%d", (int)getpid());
    ensureAutorunLine();
    writePidFile();

    const char* dPipeEnv = getenv("SF_PIPE");
    if (dPipeEnv) {
        int pfd = atoi(dPipeEnv);
        if (pfd > 0) {
            char d = 'D';
            ssize_t wr = write(pfd, &d, 1);
            close(pfd);
            (void)wr;
            dbg("[start] signaled daemon ready to pipe fd=%d", pfd);
        }
        unsetenv("SF_PIPE");
    }

    int fd = openRemoteDevice();          /* dynamic IR lookup, DEV = fallback */
    if (fd < 0) {
        dbg("[fatal] cannot open any /dev/input/event* (last tried %s) errno=%d", DEV, errno);
        return 1;
    }
    g_fd = fd;
    dbg("[start] opened %s ('%s') fd=%d", g_devPath, g_devName, fd);

    chmod(DATA_DIR, 0755);
    chmod(SQLITE3_BIN, 0755);
    chmod(HUD_JAR, 0644);

    startOverlayWatcher();

    /* Build the channel variants (and variant_counts.txt) if anything is
     * missing; shows "جاري تجهيز القنوات..." while it runs. */
    ensureChannelVariants();

    /* Auto-healing: ensure dalvik-cache reflects the last chosen color */
    healColorFromChoice();

    /* Every start produces report.txt (counts, integrity, service.db state,
     * process pictures) so it can be fetched over FTP without a shell.
     * It is written by a child: re-deriving the counts takes a few seconds
     * and must not delay the first key press.  The parent waits for it only
     * when the report has to be fresh before the daemon serves keys (never). */
    pid_t rp = fork();
    if (rp == 0) {
        closeAllFdsAbove(3);
        writeReport(argv[0]);
        _exit(0);
    }
    if (rp < 0) {
        dbg("[report] fork errno=%d - writing inline", errno);
        writeReport(argv[0]);
    } else {
        dbg("[report] writing %s in child pid=%d", REPORT_PATH, (int)rp);
    }

    /* Silent background update check at daemon startup */
    spawnSilentUpdateCheck();
    time_t lastSilentCheckTime = time(nullptr);

    int menuState = 0;      /* 0 = closed, 1 = main menu, 2 = channel types, 3 = update check */
    time_t openTime = 0;
    int64_t redPressStartMs = 0;
    int64_t mainLoopStartMs = nowMs();
    bool updateOkWritten = false;

    struct pollfd pfd = { fd, POLLIN, 0 };
    struct input_event ev;

    while (g_running) {
        /* Auto-confirm update if new version runs cleanly for 15 seconds */
        if (!updateOkWritten && access(UPDATE_PENDING_FILE, F_OK) == 0) {
            if (nowMs() - mainLoopStartMs >= 15000) {
                char okBuf[32];
                snprintf(okBuf, sizeof(okBuf), "%d\n", CC_VERSION_NUM);
                writeFile(UPDATE_OK_FILE, okBuf, true);
                unlink(UPDATE_PENDING_FILE);
                dbg("[up] new version %s confirmed healthy (15s uptime), wrote update_ok",
                    versionToString(CC_VERSION_NUM).c_str());
                updateOkWritten = true;
            }
        }

        /* Periodic silent background update check (every 24h, or 6h if last failed) */
        time_t nowTime = time(nullptr);
        int silentInterval = 24 * 3600;
        UpdateStateInfo lastState;
        if (readUpdateState(&lastState) && lastState.status == "CHECK_FAILED") {
            silentInterval = 6 * 3600; // retry after 6h on failure
        }
        if (nowTime - lastSilentCheckTime >= silentInterval) {
            lastSilentCheckTime = nowTime;
            dbg("[up] spawning recurring silent update check (interval=%d s)", silentInterval);
            spawnSilentUpdateCheck();
        }

        /* Periodic check: if plugin binary was deleted while daemon is running */
        static time_t s_lastBinaryCheck = 0;
        time_t nowSec = time(nullptr);
        if (nowSec - s_lastBinaryCheck >= 2) {
            s_lastBinaryCheck = nowSec;
            if (access(TARGET_BIN_PATH, F_OK) != 0 && access(UPDATE_LOCK_FILE, F_OK) != 0) {
                dbg("[daemon] DETECTED: %s deleted from plugin list! Silently restoring channels...", TARGET_BIN_PATH);
                performSilentRestoreAndExit();
            }
        }

#ifdef CC_TEST_CRASH
        /* Test-only flag: deliberately crash after ~3 seconds to test rollback guard */
        if (nowMs() - mainLoopStartMs >= 3000) {
            dbg("[crash-test] Triggering deliberate abort() after 3s of main loop (CC_TEST_CRASH)");
            abort();
        }
#endif

        int pollTimeout = (redPressStartMs > 0) ? 50 : ((menuState == 8) ? 40 : POLL_TIMEOUT_MS);
        while (waitpid(-1, nullptr, WNOHANG) > 0);
        int r = poll(&pfd, 1, pollTimeout);
        if (r < 0) {
            if (errno == EINTR) {
                if (!g_running) break;
                continue;
            }
            dbg("[poll] error errno=%d", errno);
            continue;
        }

        /* Check for long-press trigger on red key (>= 700ms) */
        if (menuState == 0 && redPressStartMs > 0) {
            if (nowMs() - redPressStartMs >= 700) {
                dbg("[ac] RED key long-press triggered (%lld ms) -> showMenu",
                    (long long)(nowMs() - redPressStartMs));
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
                redPressStartMs = 0;
            }
        }

        int timeoutSec = MENU_TIMEOUT_SEC;
        if (menuState == 8) timeoutSec = 300; // 5 minutes for dish alignment
        else if (menuState == 2) timeoutSec = CHANNELS_TIMEOUT_SEC;
        else if (menuState == 3 || menuState == 4 || menuState == 5) timeoutSec = 30; // 30s timeout for sub-screens

        if (r == 0) {
            if (menuState == 8) {
                static long long lastSnrHudMs = 0;
                long long nowBp = nowMs();
                if (nowBp - lastSnrHudMs >= 350) {
                    showSnrMonitor();
                    lastSnrHudMs = nowBp;
                }
                /* ---- beeper ---- */
                if (!g_beepMute) {
                    int interval = beepIntervalMs(g_beepQuality);
                    if (nowBp - g_beepLastMs >= interval) {
                        playBeepAsync(beepFreqHz(g_beepQuality), beepDurationMs(g_beepQuality));
                        g_beepLastMs = nowBp;
                    }
                }
                if (time(nullptr) - openTime > 300) {
                    dbg("[ac] SNR monitor timeout -> hide");
                    hideMenu();
                    menuState = 0;
                }
            } else if (menuState != 0 && time(nullptr) - openTime > timeoutSec) {
                dbg("[ac] menu timeout (state %d) -> hide", menuState);
                hideMenu();
                menuState = 0;
            }
            continue;
        }
        if (!(pfd.revents & POLLIN)) continue;

        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n != (ssize_t)sizeof(ev)) continue;
        if (ev.type != EV_KEY) continue;

        bool isDown = (ev.value == 1);
        unsigned code = ev.code;
        dbg("[ev] menuState=%d code=%u value=%d", menuState, code, ev.value);

        if (menuState == 0) {
            if (ev.value == 1) { /* Key Press Down */
                if (isRedKey(code)) {
                    redPressStartMs = nowMs();
                    dbg("[ac] RED key pressed down, start timer at %lld ms", (long long)redPressStartMs);
                } else {
                    redPressStartMs = 0;
                }
            } else if (ev.value == 0) { /* Key Release Up */
                if (isRedKey(code)) {
                    dbg("[ac] RED key released before long-press threshold (%lld ms) -> ignored",
                        (long long)(redPressStartMs > 0 ? nowMs() - redPressStartMs : 0));
                    redPressStartMs = 0;
                }
            } else if (ev.value == 2) { /* Repeat */
                if (isRedKey(code) && redPressStartMs > 0 && (nowMs() - redPressStartMs >= 700)) {
                    dbg("[ac] RED key long-press repeat triggered (%lld ms) -> showMenu",
                        (long long)(nowMs() - redPressStartMs));
                    showMenu();
                    menuState = 1;
                    openTime = time(nullptr);
                    redPressStartMs = 0;
                }
            }
            continue;
        }

        if (time(nullptr) - openTime > timeoutSec) {
            hideMenu();
            menuState = 0;
            redPressStartMs = 0;
            continue;
        }

        if (!isDown) continue;

        /* ---- channel-types sub-screen: 1..4 apply, EXIT goes back ---- */
        /* ---- SNR monitor sub-screen: EXIT/OK/8/RED hides, BACK goes to main menu ---- */
        if (menuState == 8) {
            if (isExitKey(code) || isOkKey(code) || code == KEY_8 || isRedKey(code)) {
                dbg("[ac] hide SNR monitor (code %u)", code);
                g_beepMute = false; /* reset mute on close */
                hideMenu();
                menuState = 0;
            } else if (code == KEY_BACK) {
                dbg("[ac] SNR monitor -> back to main menu");
                g_beepMute = false;
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
            } else if (code == 11 /* KEY_0 */ || code == 113 /* KEY_MUTE */) {
                g_beepMute = !g_beepMute;
                g_beepLastMs = 0; /* fire beep immediately on unmute */
                openTime = time(nullptr); /* keep SNR monitor alive */
                dbg("[ac] beep mute=%d", (int)g_beepMute);
                showSnrMonitor(); /* refresh HUD immediately */
            }
            continue;
        }

        if (menuState == 2) {
            int idx = -1;
            if      (code == KEY_1) idx = 0;
            else if (code == KEY_2) idx = 1;
            else if (code == KEY_3) idx = 2;
            else if (code == KEY_4) idx = 3;
            if (idx >= 0 && idx < CHANNEL_MENU_COUNT && idx < VARIANTS_N) {
                dbg("[ac] sel [%d] %s", idx + 1, VARIANTS[idx].file.c_str());
                applyChannels(VARIANTS[idx].file.c_str(), VARIANTS[idx].label.c_str());
                menuState = 0;
            } else if (isExitKey(code)) {
                dbg("[ac] back to main menu (code %u)", code);
                showMenu();                    /* EXIT returns, it does not close */
                menuState = 1;
                openTime = time(nullptr);
            }
            continue;
        }

        /* ---- update check sub-screen: EXIT goes back ---- */
        if (menuState == 3) {
            if (code == KEY_1) {
                dbg("[ac] update now requested (KEY_1)");
                bool ok = performRemoteUpdateInteractive();
                if (ok) {
                    _exit(0);
                } else {
                    openTime = time(nullptr);
                }
            } else if (isExitKey(code)) {
                dbg("[ac] update screen -> back to main menu (code %u)", code);
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
            }
            continue;
        }

        /* ---- remove addon confirmation sub-screen (menuState == 4) ---- */
        if (menuState == 4) {
            if (isOkKey(code)) {
                dbg("[ac] user confirmed addon removal");
                performAddonUninstall();
                _exit(0);
            } else if (isExitKey(code)) {
                dbg("[ac] remove addon cancelled, returning to main menu");
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
            }
            continue;
        }

        /* ---- update backup confirmation sub-screen (menuState == 5) ---- */
        if (menuState == 5) {
            if (isOkKey(code)) {
                dbg("[ac] user confirmed backup update");
                bool ok = ensureUserChannelsBackup(true);
                if (ok) {
                    writeFile(HUD_TXT, hcol(H_OK, M_BACKUP_UPDATED_OK), true);
                } else {
                    writeFile(HUD_TXT, hcol(H_ERR, "\xD9\x81\xD8\xB4\xD9\x84\x20\xD8\xAA\xD8\xAD\xD8\xAF\xD9\x8A\xD8\xAB\x20\xD8\xA7\xD9\x84\xD9\x86\xD8\xB3\xD8\xAE\xD8\xA9"), true); /* فشل تحديث النسخة */
                }
                sleep(SUCCESS_DELAY_SEC);
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
            } else if (isExitKey(code)) {
                dbg("[ac] backup update cancelled, returning to main menu");
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
            }
            continue;
        }

        /* ---- main colour screen ---- */
        if (code == KEY_1) {
            patchColorInPlace(0xd9, N_GOLD);
            menuState = 0;
        } else if (code == KEY_2) {
            patchColorInPlace(0xda, N_GREEN);
            menuState = 0;
        } else if (code == KEY_3) {
            patchColorInPlace(0xd7, N_RED);
            menuState = 0;
        } else if (code == KEY_4) {
            patchColorInPlace(0xcf, N_CYAN);
            menuState = 0;
        } else if (code == KEY_5 || code == KEY_0) {
            patchColorInPlace(0x2f, N_DEFAULT);
            menuState = 0;
        } else if (code == KEY_6) {
            dbg("[ac] open sub-screen (code %u)", code);
            showChannels();
            menuState = 2;
            openTime = time(nullptr);
        } else if (code == KEY_7) {
            dbg("[ac] open update check (code %u)", code);
            showCheckingHud();
            menuState = 3;
            openTime = time(nullptr);
            UpdateCheckResult res;
            runUpdateCheck(true, &res);
            if (res.status == UpdateCheckResult::STATUS_CANCELLED) {
                dbg("[ac] update check cancelled by user, returning to main menu");
                showMenu();
                menuState = 1;
                openTime = time(nullptr);
            } else {
                showUpdateScreen(res);
                openTime = time(nullptr);
            }
        } else if (code == KEY_8) {
            dbg("[ac] open SNR monitor (KEY_8)");
            showSnrMonitor();
            menuState = 8;
            openTime = time(nullptr);
            g_beepLastMs = 0; /* fire beep immediately on open */
        } else if (isExitKey(code)) {
            dbg("[ac] hideMenu via exit key (code %u)", code);
            hideMenu();
            menuState = 0;
        }
    }

    close(fd);
    g_fd = -1;
    hideMenu();
    if (g_overlayPid > 0) {
        kill(g_overlayPid, SIGTERM);
        dbg("[overlay] watcher pid=%d stopped with the daemon", (int)g_overlayPid);
        g_overlayPid = -1;
    }
    unlink(PID_FILE);
    if (g_dbgFile) { fclose(g_dbgFile); g_dbgFile = nullptr; }
    return 0;
}