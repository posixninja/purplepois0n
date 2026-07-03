/*
 * purplepois0n minimal KPF — AMFI + dyld + AMFI kext patches.
 * Pattern-based patchfinder derived from checkra1n/PongoOS checkra1n/kpf (MIT).
 */

#include <pongo.h>
#include <xnu/xnu.h>
#include <string.h>
#include <stdlib.h>

#define NOP 0xd503201f
#define RET 0xd65f03c0

/** Default purple KPF: patch cstrings / __DATA_CONST only; opt-in text with flag 0x2. */
#define KPF_FLAG_DATA_ONLY  (1u << 0)
#define KPF_FLAG_ALLOW_TEXT (1u << 1)

#if DEV_BUILD
#define DEVLOG(x, ...) do { \
    printf(x "\n", ##__VA_ARGS__); \
} while (0)
#define panic_at(addr, str, ...) do { \
    panic(str " (0x%llx)", ##__VA_ARGS__, xnu_ptr_to_va(addr)); \
} while (0)
#else
#define DEVLOG(x, ...) do {} while (0)
#define panic_at(addr, str, ...) do { \
    (void)(addr); \
    panic(str, ##__VA_ARGS__); \
} while (0)
#endif

static int gkpf_didrun = 0;
static int gkpf_flags = 0;

static bool g_found_amfi = false;
static bool g_found_dyld = false;
static bool g_found_amfi_sha1 = false;
static bool g_found_amfi_execve = false;
static bool g_found_mac_mount = false;
static bool g_found_dounmount = false;
static bool g_found_snapshot = false;
static bool g_found_conversion = false;
static bool g_found_convert_port = false;
static bool g_found_vm_map_protect = false;
static bool g_found_vm_fault_enter = false;
static bool g_found_sbops = false;
static int g_data_patch_count = 0;

static bool kpf_wants_data_only(void)
{
    if (gkpf_flags & KPF_FLAG_ALLOW_TEXT) {
        return false;
    }
    return true;
}

int32_t sxt32(int32_t value, uint8_t bits)
{
    value = ((uint32_t)value) << (32 - bits);
    value >>= (32 - bits);
    return value;
}

uint32_t* dyld_hook_addr;

uint32_t* find_next_insn(uint32_t* from, uint32_t num, uint32_t insn, uint32_t mask)
{
    while (num) {
        if ((*from & mask) == (insn & mask)) {
            return from;
        }
        from++;
        num--;
    }
    return NULL;
}

uint32_t* find_prev_insn(uint32_t* from, uint32_t num, uint32_t insn, uint32_t mask)
{
    while (num) {
        if ((*from & mask) == (insn & mask)) {
            return from;
        }
        from--;
        num--;
    }
    return NULL;
}

static bool kpf_amfi_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    (void)patch;
    char has_frame = 0;
    for (int x = 0; x < 128; x++) {
        uint32_t opcde = opcode_stream[-x];
        if (opcde == RET || opcde == 0xd65f0fff ||
            (opcde & 0xFF000000) == 0x94000000 ||
            (opcde & 0xFF000000) == 0x14000000) {
            break;
        }
        if ((opcde & 0xffff0000) == 0xa9430000) {
            has_frame = 1;
            break;
        } else if ((opcde & 0xff0003e0) == 0xa90003e0) {
            has_frame = 1;
            break;
        }
    }
    if (!has_frame) {
        puts("KPF: Found AMFI (Leaf)");
        opcode_stream[0] = 0xd2800020;
        opcode_stream[1] = RET;
    } else {
        bool found_something = false;
        uint32_t* retpoint = find_next_insn(&opcode_stream[0], 0x180, RET, 0xffffffff);
        if (retpoint == NULL) {
            DEVLOG("kpf_amfi_callback: failed to find retpoint");
            return false;
        }
        uint32_t* patchpoint = find_prev_insn(retpoint, 0x40, 0xAA0003E0, 0xffe0ffff);
        if (patchpoint != NULL) {
            patchpoint[0] = 0xd2800020;
            found_something = true;
        }
        patchpoint = find_prev_insn(retpoint, 0x40, 0x52800000, 0xffffffff);
        if (patchpoint != NULL) {
            patchpoint[0] = 0xd2800020;
            found_something = true;
        }
        if (!found_something) {
            DEVLOG("kpf_amfi_callback: failed to find anything");
            return false;
        }
        puts("KPF: Found AMFI (Routine)");
    }
    return true;
}

static bool kpf_amfi_callback_tracked(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    if (kpf_amfi_callback(patch, opcode_stream)) {
        g_found_amfi = true;
        return true;
    }
    return false;
}

static void kpf_amfi_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0x91000000,
        0x52800200,
        0xd3000000,
        0x9b000000
    };
    uint64_t masks[] = {
        0xFF000000,
        0xFFFFFF00,
        0xFF000000,
        0xFF000000
    };
    xnu_pf_maskmatch(patchset, "amfi_patch", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false,
                     (void*)kpf_amfi_callback_tracked);
}

static bool kpf_dyld_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    (void)patch;
    if (dyld_hook_addr) {
        puts("dyld_hook_addr already found; skipping");
        return false;
    }
    uint8_t rn = (opcode_stream[6] >> 5) & 0x1f;
    if ((opcode_stream[10] & 0xFF00001F) != (0x35000000 | rn)) {
        DEVLOG("Invalid match for dyld patch at 0x%llx (missing CBNZ w%d)",
               xnu_rebase_va(xnu_ptr_to_va(opcode_stream)), rn);
        return false;
    }
    rn = (opcode_stream[3] >> 16) & 0x1f;
    dyld_hook_addr = &opcode_stream[1];
    opcode_stream[1] = 0;
    opcode_stream[2] = 0xAA0003E0 | rn;
    opcode_stream[3] = 0x14000008;
    puts("KPF: Patched dyld check");
    g_found_dyld = true;
    return true;
}

static void kpf_dyld_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0x54000002,
        0x90000000,
        0x91000000,
        0xAA0003E0,
        0x39400000,
        0x39400000,
        0x6B00001F
    };
    uint64_t masks[] = {
        0xFF00000F,
        0x9F000000,
        0xFF000000,
        0xFFE0FFE0,
        0xFFFFC000,
        0xFFFFC000,
        0xFFE0FC1F
    };
    xnu_pf_maskmatch(patchset, "dyld_patch", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)kpf_dyld_callback);
}

static bool kpf_mac_mount_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    puts("KPF: Found mac_mount");
    uint32_t* mac_mount = &opcode_stream[0];
    uint32_t* mac_mount_1 = find_prev_insn(mac_mount, 0x40, 0x37280000, 0xfffe0000);
    if (!mac_mount_1) {
        mac_mount_1 = find_next_insn(mac_mount, 0x40, 0x37280000, 0xfffe0000);
    }
    if (!mac_mount_1) {
        DEVLOG("kpf_mac_mount_callback: failed to find NOP point");
        return false;
    }
    mac_mount_1[0] = NOP;
    mac_mount_1 = find_prev_insn(mac_mount, 0x40, 0x3941c508, 0xFFFFFFFF);
    if (!mac_mount_1) {
        mac_mount_1 = find_next_insn(mac_mount, 0x40, 0x3941c508, 0xFFFFFFFF);
    }
    if (!mac_mount_1) {
        DEVLOG("kpf_mac_mount_callback: failed to find xzr point");
        return false;
    }
    mac_mount_1[0] = 0xaa1f03e8;
    g_found_mac_mount = true;
    xnu_pf_disable_patch(patch);
    puts("KPF: Found mac_mount");
    return true;
}

static void kpf_mac_mount_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = { 0x321f2fe9 };
    uint64_t masks[] = { 0xFFFFFFFF };
    xnu_pf_maskmatch(patchset, "mac_mount_patch1", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)kpf_mac_mount_callback);
    matches[0] = 0x5283ffc9;
    xnu_pf_maskmatch(patchset, "mac_mount_patch2", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)kpf_mac_mount_callback);
}

static bool kpf_mac_dounmount_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    (void)patch;
    uint8_t rn;
    if ((opcode_stream[-1] & 0xFFE0FFFF) == 0xAA0003E0 &&
        (opcode_stream[3] & 0xFC000000) == 0x94000000) {
        rn = (opcode_stream[-1] >> 16) & 0x1f;
        opcode_stream += 3;
    } else if ((opcode_stream[3] & 0xFFE0FFFF) == 0xAA0003E0 &&
               (opcode_stream[4] & 0xFC000000) == 0x94000000) {
        rn = (opcode_stream[3] >> 16) & 0x1f;
        opcode_stream += 4;
    } else {
        return false;
    }

    if (opcode_stream[1] != (0xAA0003E0 | (rn << 16)) ||
        (opcode_stream[2] & 0xFC000000) != 0x94000000) {
        return false;
    }

    uint32_t* parent_lock = find_next_insn(opcode_stream, 0x100, 0x52800041, 0xFFFFFFFF);
    if (!parent_lock) {
        parent_lock = find_next_insn(opcode_stream, 0x100, 0x321F03E1, 0xFFFFFFFF);
    }
    if (!parent_lock) {
        DEVLOG("Dounmount no parent lock code");
        return false;
    }

    uint8_t parent_rn = 0;
    uint32_t* call;
    if ((parent_lock[-1] & 0xFFE0FFFF) == 0xAA0003E0 &&
        (parent_lock[1] & 0xFC000000) == 0x94000000) {
        call = parent_lock + 1;
        parent_rn = (parent_lock[-1] >> 16) & 0x1f;
    } else if ((parent_lock[1] & 0xFFE0FFFF) == 0xAA0003E0 &&
               (parent_lock[2] & 0xFC000000) == 0x94000000) {
        parent_rn = (parent_lock[1] >> 16) & 0x1f;
        call = parent_lock + 2;
    } else {
        DEVLOG("Dounmount failed to find first call for parent_vp");
        return false;
    }

    if (call[1] != (0xAA0003E0 | (parent_rn << 16)) ||
        (call[2] & 0xFC000000) != 0x94000000) {
        DEVLOG("Dounmount failed to find second call for parent_vp");
        return false;
    }

    if (g_found_dounmount) {
        panic("dounmount found twice!");
    }

    puts("KPF: Found dounmount");
    opcode_stream[0] = NOP;
    g_found_dounmount = true;
    return true;
}

static void kpf_mac_dounmount_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0x52800001,
        0x52800002,
        0x52800003,
    };
    uint64_t masks[] = {
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
    };
    xnu_pf_maskmatch(patchset, "dounmount_patch", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)kpf_mac_dounmount_callback);
}


typedef struct {
    const char* needle;
    const char* label;
    bool* found_flag;
} kpf_cstring_neutralize_t;

static bool neutralize_cstring_in_buffer(void* base, size_t size, const char* needle,
                                           const char* label)
{
    if (!base || !size || !needle || !needle[0]) {
        return false;
    }
    const size_t needle_len = strlen(needle);
    char* hit = memmem(base, size, needle, needle_len);
    if (!hit) {
        return false;
    }
    hit[0] = 'x';
    puts("KPF: data patch (cstring)");
    printf("KPF:   %s\n", label);
    g_data_patch_count++;
    return true;
}

static void apply_cstring_neutralizations(struct mach_header_64* hdr)
{
    static kpf_cstring_neutralize_t table[] = {
        {"com.apple.os.update-", "snapshot_os_update", &g_found_snapshot},
        {"launch-failure-unsupported-ancillary", "launch_failure_ancillary", NULL},
        {"Signed System Volume", "ssv_banner", NULL},
        {"root filesystem is not authenticated", "rootfs_auth", NULL},
    };

    xnu_pf_range_t* text_cstring = xnu_pf_section(hdr, "__TEXT", "__cstring");
    xnu_pf_range_t* plk_text = xnu_pf_section(hdr, "__PRELINK_TEXT", "__text");

    for (size_t t = 0; t < sizeof(table) / sizeof(table[0]); ++t) {
        bool matched = false;
        if (text_cstring) {
            matched = neutralize_cstring_in_buffer(text_cstring->cacheable_base, text_cstring->size,
                                                   table[t].needle, table[t].label);
        }
        if (!matched && plk_text) {
            matched = neutralize_cstring_in_buffer(plk_text->cacheable_base, plk_text->size,
                                                   table[t].needle, table[t].label);
        }
        if (matched && table[t].found_flag) {
            *table[t].found_flag = true;
        }
    }
}

static bool sb_ops_data_callback(struct xnu_pf_patch* patch, void* cacheable_stream)
{
    (void)patch;
    (void)cacheable_stream;
    puts("KPF: Found sbops (data pointer)");
    g_found_sbops = true;
    return true;
}

static void apply_data_const_patches(struct mach_header_64* hdr)
{
    xnu_pf_range_t* text_cstring_range = xnu_pf_section(hdr, "__TEXT", "__cstring");
    xnu_pf_range_t* data_const_range = xnu_pf_section(hdr, "__DATA_CONST", "__const");
    xnu_pf_range_t* plk_text_range = xnu_pf_section(hdr, "__PRELINK_TEXT", "__text");
    xnu_pf_range_t* plk_data_const_range = xnu_pf_section(hdr, "__PLK_DATA_CONST", "__data");

    if (!text_cstring_range || !data_const_range) {
        DEVLOG("apply_data_const_patches: missing __TEXT/__cstring or __DATA_CONST");
        return;
    }

    xnu_pf_patchset_t* patchset = xnu_pf_patchset_create(XNU_PF_ACCESS_64BIT);
    xnu_pf_ptr_to_data(patchset, xnu_slide_value(hdr), text_cstring_range,
                       "Seatbelt sandbox policy", strlen("Seatbelt sandbox policy") + 1, false,
                       (void*)sb_ops_data_callback);
    xnu_pf_emit(patchset);
    xnu_pf_apply(data_const_range, patchset);
    xnu_pf_patchset_destroy(patchset);

    if (!g_found_sbops && plk_text_range && plk_data_const_range) {
        patchset = xnu_pf_patchset_create(XNU_PF_ACCESS_64BIT);
        xnu_pf_ptr_to_data(patchset, xnu_slide_value(hdr), plk_text_range,
                           "Seatbelt sandbox policy", strlen("Seatbelt sandbox policy") + 1, false,
                           (void*)sb_ops_data_callback);
        xnu_pf_emit(patchset);
        xnu_pf_apply(plk_data_const_range, patchset);
        xnu_pf_patchset_destroy(patchset);
    }
}

static void apply_data_only_patches(struct mach_header_64* hdr)
{
    puts("KPF: data-only mode — cstrings + __DATA_CONST (no __TEXT_EXEC writes)");
    apply_data_const_patches(hdr);
    apply_cstring_neutralizations(hdr);
}

static bool kpf_amfi_execve_tail(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    uint32_t* ret = find_next_insn(opcode_stream, 0x80, RET, 0xFFFFFFFF);
    if (!ret) {
        DEVLOG("kpf_amfi_execve_tail: failed to find amfi_ret");
        return false;
    }
    puts("KPF: Found AMFI execve hook");
    xnu_pf_disable_patch(patch);
    g_found_amfi_execve = true;
    return true;
}

static bool kpf_amfi_sha1(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    (void)patch;
    uint32_t* cmp = find_next_insn(opcode_stream, 0x10, 0x7100081f, 0xFFFFFFFF);
    if (!cmp) {
        DEVLOG("kpf_amfi_sha1: failed to find cmp");
        return false;
    }
    puts("KPF: Found AMFI hashtype check");
    *cmp = 0x6b00001f;
    g_found_amfi_sha1 = true;
    return true;
}

static void kpf_amfi_kext_patches(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0x34000000,
        0x52844009,
        0x2a090108
    };
    uint64_t masks[] = {
        0xff000000,
        0xffffffff,
        0xffffffff,
    };
    xnu_pf_maskmatch(patchset, "amfi_execve_tail", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)kpf_amfi_execve_tail);

    uint64_t sha1_matches[] = { 0x36d00002 };
    uint64_t sha1_masks[] = { 0xfff8001f };
    xnu_pf_maskmatch(patchset, "amfi_sha1", sha1_matches, sha1_masks,
                     sizeof(sha1_matches) / sizeof(uint64_t), false, (void*)kpf_amfi_sha1);
}

static bool kpf_conversion_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    (void)patch;
    uint32_t lr1 = opcode_stream[0];
    uint32_t lr2 = opcode_stream[2];
    if ((lr1 & 0x1f) != (opcode_stream[1] & 0x1f) ||
        (lr2 & 0x1f) != (opcode_stream[3] & 0x1f) ||
        (lr1 & 0x3ffc00) != (lr2 & 0x3ffc00)) {
        DEVLOG("kpf_conversion_callback: opcode check failed at 0x%llx",
               xnu_ptr_to_va(opcode_stream));
        return false;
    }
    puts("KPF: Found task_conversion_eval");
    uint32_t regs = (1 << ((lr1 >> 5) & 0x1f)) | (1 << ((lr2 >> 5) & 0x1f));
    for (size_t i = 0; i < 128; ++i) {
        uint32_t op = *--opcode_stream;
        if ((op & 0xffe0fc1f) == 0xeb00001f &&
            (regs & (1 << ((op >> 5) & 0x1f))) != 0 &&
            (regs & (1 << ((op >> 16) & 0x1f))) != 0) {
            uint32_t next = opcode_stream[1];
            if ((next & 0xffe0fc10) == 0xfa401000 ||
                (next & 0xff00001e) == 0x54000000) {
                *opcode_stream = 0xeb1f03ff;
                g_found_conversion = true;
                return true;
            }
        } else if ((op & 0xffe0ffe0) == 0xaa0003e0) {
            uint32_t src = (op >> 16) & 0x1f;
            uint32_t dst = op & 0x1f;
            regs |= ((regs >> dst) & 1) << src;
        }
    }
    DEVLOG("kpf_conversion_callback: failed to find cmp at 0x%llx", xnu_ptr_to_va(opcode_stream));
    return false;
}

static void kpf_conversion_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0xb9400000,
        0x36500000,
        0xb9400000,
        0x36500000,
    };
    uint64_t masks[] = {
        0xffc00000,
        0xfff80000,
        0xffc00000,
        0xfef80000,
    };
    xnu_pf_maskmatch(patchset, "conversion_patch", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)kpf_conversion_callback);
}

static bool kpf_convert_port_to_map_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    (void)patch;
    if ((opcode_stream[0] & 0x1f) != ((opcode_stream[3] >> 5) & 0x1f)) {
        return false;
    }
    uint32_t* patchpoint = find_next_insn(opcode_stream + 5, 0x18, 0x54000000, 0xff00001f);
    if (!patchpoint) {
        return false;
    }
    if (g_found_convert_port) {
        DEVLOG("convert_port_to_map found twice; skipping");
        return false;
    }
    puts("KPF: Found convert_port_to_map_with_flavor");
    g_found_convert_port = true;
    *patchpoint = NOP;
    return true;
}

static void kpf_convert_port_to_map_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0xaa0103f0,
        0x7100083f,
        0x54000000,
        0x7100061f,
        0x54000000,
    };
    uint64_t masks[] = {
        0xfffffff0,
        0xffffffff,
        0xff00001f,
        0xfffffe1f,
        0xff00001f,
    };
    xnu_pf_maskmatch(patchset, "convert_port_to_map", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false,
                     (void*)kpf_convert_port_to_map_callback);
}

static bool kpf_mac_vm_map_protect_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    puts("KPF: Found vm_map_protect");
    uint32_t* first_ldr = find_next_insn(&opcode_stream[0], 0x400, 0x37480000, 0xFFFF0000);
    if (!first_ldr) {
        DEVLOG("kpf_mac_vm_map_protect_callback: failed to find ldr");
        return false;
    }
    first_ldr++;
    uint32_t delta = (uint32_t)(first_ldr - (&opcode_stream[2]));
    delta &= 0x03ffffff;
    delta |= 0x14000000;
    opcode_stream[2] = delta;
    g_found_vm_map_protect = true;
    xnu_pf_disable_patch(patch);
    return true;
}

static void kpf_mac_vm_map_protect_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0x121f0600,
        0x71001900
    };
    uint64_t masks[] = {
        0xffffff00,
        0xffffff00
    };
    xnu_pf_maskmatch(patchset, "vm_map_protect", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false,
                     (void*)kpf_mac_vm_map_protect_callback);
    uint64_t i_matches[] = {
        0x2A2003E0,
        0x721F041F,
        0x54000000
    };
    uint64_t i_masks[] = {
        0xFFE0FFE0,
        0xFFFFFC1F,
        0xff000000
    };
    xnu_pf_maskmatch(patchset, "vm_map_protect2", i_matches, i_masks,
                     sizeof(i_matches) / sizeof(uint64_t), false,
                     (void*)kpf_mac_vm_map_protect_callback);
}

static bool vm_fault_enter_callback(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    if (g_found_vm_fault_enter) {
        return false;
    }
    if (!find_next_insn(opcode_stream, 0x18, 0x36100000, 0xFEF80000)) {
        return false;
    }
    uint32_t* b_loc = find_prev_insn(opcode_stream, 0x80, 0x14000000, 0xFF000000);
    if (!b_loc) {
        return false;
    }
    uint32_t* wanted_addr = b_loc + 1;
    for (int i = 2; i < 20; i++) {
        uint32_t* try_loc = wanted_addr - i;
        if (((*try_loc | (i << 5)) & 0xFD07FFE0) == (0x34000000 | i << 5)) {
            *try_loc = NOP;
            puts("KPF: Found vm_fault_enter");
            g_found_vm_fault_enter = true;
            xnu_pf_disable_patch(patch);
            return true;
        }
    }
    return false;
}

static bool vm_fault_enter_callback14(struct xnu_pf_patch* patch, uint32_t* opcode_stream)
{
    if (g_found_vm_fault_enter) {
        return false;
    }
    uint32_t* mov = find_prev_insn(opcode_stream, 0x18, 0x52800640, 0xffffffff);
    if (!mov || (mov[1] & 0xff000000) != 0x14000000) {
        return false;
    }
    opcode_stream[0] = NOP;
    puts("KPF: Found vm_fault_enter");
    g_found_vm_fault_enter = true;
    xnu_pf_disable_patch(patch);
    return true;
}

static void kpf_mac_vm_fault_enter_patch(xnu_pf_patchset_t* patchset)
{
    uint64_t matches[] = {
        0x37980000,
        0x37900000
    };
    uint64_t masks[] = {
        0xFFF80000,
        0xFFF80000
    };
    xnu_pf_maskmatch(patchset, "vm_fault_enter", matches, masks,
                     sizeof(matches) / sizeof(uint64_t), false, (void*)vm_fault_enter_callback);
    uint64_t matches_i[] = {
        0x37980000,
        0x34000000,
        0x37900000
    };
    uint64_t masks_i[] = {
        0xFFF80000,
        0xFF000000,
        0xFFF80000
    };
    xnu_pf_maskmatch(patchset, "vm_fault_enter_i", matches_i, masks_i,
                     sizeof(matches_i) / sizeof(uint64_t), false, (void*)vm_fault_enter_callback);
    uint64_t matches14[] = {
        0x36180000,
        0x52800000,
    };
    uint64_t masks14[] = {
        0xFFF80000,
        0xFFFFFFE0,
    };
    xnu_pf_maskmatch(patchset, "vm_fault_enter14", matches14, masks14,
                     sizeof(matches14) / sizeof(uint64_t), false, (void*)vm_fault_enter_callback14);
    uint64_t matches14_alt[] = {
        0x36180000,
        0xAA170210,
        0x52800000,
    };
    uint64_t masks14_alt[] = {
        0xFFF80000,
        0xFFFFFE10,
        0xFFFFFFE0,
    };
    xnu_pf_maskmatch(patchset, "vm_fault_enter14_alt", matches14_alt, masks14_alt,
                     sizeof(matches14_alt) / sizeof(uint64_t), false, (void*)vm_fault_enter_callback14);
}

static void apply_text_exec_patches(struct mach_header_64* hdr)
{
    xnu_pf_patchset_t* patchset = xnu_pf_patchset_create(XNU_PF_ACCESS_32BIT);
    xnu_pf_range_t* text_exec_range = xnu_pf_section(hdr, "__TEXT_EXEC", "__text");
    if (!text_exec_range) {
        panic("no __TEXT_EXEC __text");
    }

    kpf_dyld_patch(patchset);
    kpf_amfi_patch(patchset);
    kpf_mac_mount_patch(patchset);
    kpf_mac_dounmount_patch(patchset);
    kpf_conversion_patch(patchset);
    kpf_convert_port_to_map_patch(patchset);
    kpf_mac_vm_map_protect_patch(patchset);
    kpf_mac_vm_fault_enter_patch(patchset);
    xnu_pf_emit(patchset);
    xnu_pf_apply(text_exec_range, patchset);
    xnu_pf_patchset_destroy(patchset);
}

static void apply_amfi_kext_patches(struct mach_header_64* hdr)
{
    struct mach_header_64* amfi_header =
        xnu_pf_get_kext_header(hdr, "com.apple.driver.AppleMobileFileIntegrity");
    if (!amfi_header) {
        puts("KPF: AMFI kext header not found (prelink only?)");
        return;
    }

    xnu_pf_range_t* amfi_text_exec = xnu_pf_section(amfi_header, "__TEXT_EXEC", "__text");
    if (!amfi_text_exec) {
        puts("KPF: AMFI kext __TEXT_EXEC missing");
        return;
    }

    xnu_pf_patchset_t* patchset = xnu_pf_patchset_create(XNU_PF_ACCESS_32BIT);
    kpf_amfi_kext_patches(patchset);
    xnu_pf_emit(patchset);
    xnu_pf_apply(amfi_text_exec, patchset);
    xnu_pf_patchset_destroy(patchset);
}

void command_kpf(void)
{
    if (gkpf_didrun) {
        puts("purplepois0n KPF did run already! Behavior here is undefined.\n");
    }
    gkpf_didrun++;

    g_found_amfi = false;
    g_found_dyld = false;
    g_found_amfi_sha1 = false;
    g_found_amfi_execve = false;
    g_found_mac_mount = false;
    g_found_dounmount = false;
    g_found_snapshot = false;
    g_found_conversion = false;
    g_found_convert_port = false;
    g_found_vm_map_protect = false;
    g_found_vm_fault_enter = false;
    g_found_sbops = false;
    g_data_patch_count = 0;
    dyld_hook_addr = NULL;

    struct mach_header_64* hdr = xnu_header();
    if (!hdr) {
        panic("no xnu header");
    }

    if (kpf_wants_data_only()) {
        apply_data_only_patches(hdr);
    } else {
        apply_amfi_kext_patches(hdr);
        apply_text_exec_patches(hdr);
        apply_cstring_neutralizations(hdr);
    }

    if (!kpf_wants_data_only() &&
        !g_found_amfi && !g_found_amfi_sha1 && !g_found_amfi_execve) {
        puts("KPF: warning — no AMFI patches matched on this kernel");
    }

    printf("KPF: patch summary — mode=%s data_patches=%d sbops=%d snapshot=%d",
           kpf_wants_data_only() ? "data-only" : "text+data",
           g_data_patch_count, g_found_sbops, g_found_snapshot);
    if (!kpf_wants_data_only()) {
        printf(" amfi=%d dyld=%d amfi_sha1=%d amfi_execve=%d "
               "mac_mount=%d dounmount=%d conversion=%d convert_port=%d "
               "vm_map_protect=%d vm_fault_enter=%d",
               g_found_amfi, g_found_dyld, g_found_amfi_sha1, g_found_amfi_execve,
               g_found_mac_mount, g_found_dounmount,
               g_found_conversion, g_found_convert_port,
               g_found_vm_map_protect, g_found_vm_fault_enter);
    }
    puts("");
    puts("KPF: purplepois0n patchset applied");
}

static void kpf_flags_cmd(const char* cmd, char* args)
{
    (void)cmd;
    uint32_t nflags = 0;
    if (args[0] != 0) {
        nflags = strtoul(args, NULL, 16);
        printf("setting kpf_flags to %x\n", nflags);
        gkpf_flags = (int)nflags;
    } else {
        printf("kpf_flags: %x\n", gkpf_flags);
    }
}

static void kpf_do_autoboot(const char* cmd, char* args)
{
    (void)cmd;
    (void)args;
    queue_rx_string("bootx\n");
}

static void command_kpf_cmd(const char* cmd, char* args)
{
    (void)cmd;
    (void)args;
    command_kpf();
}

static void kpf_autoboot(const char* cmd, char* args)
{
    (void)cmd;
    (void)args;
    queue_rx_string("xargs serial=3\n");
    queue_rx_string("\nsep auto\n");
    command_register("shell", "kickstarts auto-boot", kpf_do_autoboot);
}

void module_entry(void)
{
    puts("");
    puts("# purplepois0n KPF v0.5");
    puts("# default: data-only (cstring + __DATA_CONST); kpf_flags 2 = allow __TEXT_EXEC");
    puts("");

    gkpf_flags = (int)KPF_FLAG_DATA_ONLY;
    const char* allow_text = getenv("KPF_ALLOW_TEXT");
    if (allow_text != NULL && allow_text[0] == '1') {
        gkpf_flags = (int)KPF_FLAG_ALLOW_TEXT;
    }

    preboot_hook = command_kpf;
    command_register("kpf_flags", "set flags for kernel patchfinder", kpf_flags_cmd);
    command_register("autoboot", "purplepois0n-kpf autoboot hook", kpf_autoboot);
    command_register("kpf", "run patchfinder without booting", command_kpf_cmd);
}

char* module_name = "purplepois0n-kpf-0.5";

struct pongo_exports exported_symbols[] = {
    { .name = NULL, .value = 0 }
};
