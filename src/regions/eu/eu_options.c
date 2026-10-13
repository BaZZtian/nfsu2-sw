/* Port the existing US v0.5 Video options to the independently verified EU
 * XBE addresses. Only private copies of the Speedometer row vtable are used.
 * Original source: antoxa2584x/nfsu2-sw 48b1b2798d60f5deb80a189781d78643ac8f7f8a.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

#ifdef __SWITCH__
#define NFSU2_OPTIONS_FILE "sdmc:/switch/nfsu2x/nfsu2x_options.txt"
#else
#define NFSU2_OPTIONS_FILE "nfsu2x_options.txt"
#endif
#define FE_PAD_LEFT 0x9120409Eu
#define FE_PAD_RIGHT 0xB5971BF1u
#define ROW_VTABLE 0x0034F5F8u
#define ROW_VTABLE_SIZE 0x48u
#define ENVMAP_TABLE 0x003F3210u
#ifdef NFSU2_VULKAN
extern volatile int nv2a_vk_cube_maps;
extern volatile int nv2a_vk_scale_pct;
#endif
extern uint32_t xbox_ContiguousAlloc(uint32_t size, uint32_t alignment);
extern void sub_000CE7B0_gen(void);
extern void sub_000B5110_gen(void);
extern void sub_000B5170_gen(void);
extern void sub_0009B680_gen(void);

static const int s_scale_pct[] = {100, 150, 200, 250};
static int s_refl = 1, s_scale_idx = 1, s_scale_saved;
typedef struct {
    uint32_t label;
    const uint32_t *texts;
    int n;
    int *value;
    void (*changed)(void);
    uint32_t vtable;
} OptRow;
static void refl_changed(void)
{
#ifdef NFSU2_VULKAN
    nv2a_vk_cube_maps = s_refl;
#endif
}
#ifdef NFSU2_VULKAN
static void scale_changed(void)
{
    nv2a_vk_scale_pct = s_scale_pct[s_scale_idx];
    s_scale_saved = 1;
}
#endif
static const uint32_t s_refl_texts[] = {0x0000CCFAu, 0x0000063Cu};
static const uint32_t s_scale_texts[] = {0x2A2A4AB3u, 0x822E4E9Bu, 0xB55E7665u, 0xD3588630u};
static OptRow s_rows[] = {
    {0x8FE9288Eu, s_refl_texts, 2, &s_refl, refl_changed, 0},
#ifdef NFSU2_VULKAN
    {0x4AC50BCFu, s_scale_texts, 4, &s_scale_idx, scale_changed, 0},
#endif
};
#define N_ROWS ((int)(sizeof s_rows / sizeof s_rows[0]))

/* Treat a damaged/edited options file as a failed setting, never an index. */
static int parsed_value(const char *s, long *value)
{
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s)
        return 0;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
        ++end;
    if (*end)
        return 0;
    *value = v;
    return 1;
}
void nfsu2_options_load(void)
{
    char line[128];
    FILE *f = fopen(NFSU2_OPTIONS_FILE, "r");
    int i;
    if (f) {
        while (fgets(line, sizeof line, f)) {
            long value;
            if (!strncmp(line, "reflections=", 12) && parsed_value(line + 12, &value)) {
                if (value == 0 || value == 1)
                    s_refl = (int)value;
            } else if (!strncmp(line, "scale=", 6) && parsed_value(line + 6, &value)) {
                for (i = 0; i < 4; ++i)
                    if (s_scale_pct[i] == value) {
                        s_scale_idx = i;
                        s_scale_saved = 1;
                    }
            }
        }
        fclose(f);
    }
    refl_changed();
#ifdef NFSU2_VULKAN
    {
        const char *e = getenv("RECOMP_GL_SCALE");
        if (!s_scale_saved && e && *e) {
            double k = strtod(e, NULL), best = 1e9;
            for (i = 0; i < 4; ++i) {
                double d = fabs(k * 100.0 - s_scale_pct[i]);
                if (d < best) { best = d; s_scale_idx = i; }
            }
        } else {
            nv2a_vk_scale_pct = s_scale_pct[s_scale_idx];
        }
    }
    fprintf(stderr, "[options] car reflections %s, resolution scale %d%%%s\n",
            s_refl ? "on" : "off", s_scale_pct[s_scale_idx],
            nv2a_vk_scale_pct ? "" : " (RECOMP_GL_SCALE)");
#else
    fprintf(stderr, "[options] car reflections %s\n", s_refl ? "on" : "off");
#endif
}
static void options_save(void)
{
    FILE *f = fopen(NFSU2_OPTIONS_FILE, "w");
    if (!f) { fprintf(stderr, "[options] cannot write " NFSU2_OPTIONS_FILE "\n"); return; }
    fprintf(f, "reflections=%d\n", s_refl);
    if (s_scale_saved)
        fprintf(f, "scale=%d\n", s_scale_pct[s_scale_idx]);
    if (fclose(f))
        fprintf(stderr, "[options] settings write failed\n");
}
static OptRow *opt_row(uint32_t obj)
{
    uint32_t vt;
    if (!obj || !(vt = MEM32(obj)))
        return NULL;
    for (int i = 0; i < N_ROWS; ++i)
        if (s_rows[i].vtable && s_rows[i].vtable == vt)
            return &s_rows[i];
    return NULL;
}
static void refl_views(void)
{
    if (s_refl)
        return;
    for (unsigned n = 0; n < 6; ++n) {
        uint32_t v = MEM32(ENVMAP_TABLE + n * 4);
        if (v)
            MEM8(v + 8) = 0;
    }
}

/* Do not leak the native row's extra calls into the generated caller's GPR
 * state. In particular, generated callees publish g_ebp/g_seh_ebp. */
typedef struct { uint32_t a,c,d,s,b,si,di,bp,seh; } GuestFrame;
static GuestFrame frame_save(void)
{
    GuestFrame f = {g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp};
    return f;
}
static void frame_restore(GuestFrame f, unsigned pop)
{
    g_eax=f.a;g_ecx=f.c;g_edx=f.d;g_esp=f.s+pop;g_ebx=f.b;
    g_esi=f.si;g_edi=f.di;g_ebp=f.bp;g_seh_ebp=f.seh;
}
#define EU_OPTION_CALL(va, fn, frame) do { \
    g_ebp = (frame).bp; g_seh_ebp = (frame).seh; \
    RECOMP_ABI_CALL((va), fn); \
} while (0)

void sub_000CE7B0(void)
{
    uint32_t screen = g_ecx;
    sub_000CE7B0_gen();
    GuestFrame f = frame_save();
    for (int i = 0; i < N_ROWS; ++i) {
        OptRow *o = &s_rows[i];
        if (!o->vtable) {
            /* Verify the retail EU row's dispatch before cloning it. */
            if (MEM32(ROW_VTABLE + 4) != 0x000B5110u ||
                MEM32(ROW_VTABLE + 12) != 0x000B5170u ||
                MEM32(ROW_VTABLE + 64) != 0x0012B910u) {
                fprintf(stderr, "[options] EU row vtable differs; extra rows unavailable\n");
                break;
            }
            o->vtable = xbox_ContiguousAlloc(ROW_VTABLE_SIZE, 16);
            if (!o->vtable) { fprintf(stderr, "[options] row vtable allocation failed\n"); break; }
            memcpy((void *)XBOX_PTR(o->vtable), (const void *)XBOX_PTR(ROW_VTABLE), ROW_VTABLE_SIZE);
        }
        PUSH32(g_esp, 0x54u);
        PUSH32(g_esp, 0x000CE905u);
        EU_OPTION_CALL(0x0017F940u, sub_0017F940, f);
        g_esp += 4;
        uint32_t row = g_eax;
        if (!row) { fprintf(stderr, "[options] row allocation failed\n"); break; }
        PUSH32(g_esp, 1);
        PUSH32(g_esp, screen + 0x120u);
        PUSH32(g_esp, screen + 0x118u);
        PUSH32(g_esp, screen + 0x110u);
        PUSH32(g_esp, screen + 0x100u);
        g_ecx = row;
        PUSH32(g_esp, 0x000CE93Du);
        EU_OPTION_CALL(0x0012B480u, sub_0012B480, f);
        MEM32(row) = o->vtable;
        PUSH32(g_esp, 1);
        PUSH32(g_esp, row);
        g_ecx = screen;
        PUSH32(g_esp, 0x000CE957u);
        EU_OPTION_CALL(0x0015B8C0u, sub_0015B8C0, f);
    }
    frame_restore(f, 0);
}
static void opt_row_text(uint32_t row, const OptRow *o, GuestFrame f)
{
    PUSH32(g_esp, o->label);
    PUSH32(g_esp, MEM32(row + 0x2Cu));
    PUSH32(g_esp, 0x000B5181u);
    EU_OPTION_CALL(0x00119020u, sub_00119020, f);
    g_esp += 8;
    PUSH32(g_esp, o->texts[*o->value]);
    PUSH32(g_esp, MEM32(row + 0x30u));
    PUSH32(g_esp, 0x000B519Eu);
    EU_OPTION_CALL(0x00119020u, sub_00119020, f);
    g_esp += 8;
}
void sub_000B5170(void)
{
    OptRow *o = opt_row(g_ecx);
    if (!o) { sub_000B5170_gen(); return; }
    RECOMP_EU_ENTER(0x000B5170u);RECOMP_PREEMPT();
    GuestFrame f = frame_save();
    opt_row_text(g_ecx, o, f);
    frame_restore(f, 4);
}
void sub_000B5110(void)
{
    uint32_t row = g_ecx, msg = MEM32(g_esp + 8);
    OptRow *o = opt_row(row);
    if (!o) { sub_000B5110_gen(); return; }
    RECOMP_EU_ENTER(0x000B5110u);RECOMP_PREEMPT();
    GuestFrame f = frame_save();
    if (msg == FE_PAD_LEFT || msg == FE_PAD_RIGHT) {
        *o->value = (*o->value + (msg == FE_PAD_RIGHT ? 1 : o->n - 1)) % o->n;
        o->changed();refl_views();options_save();
        fprintf(stderr, "[options] car reflections %s, resolution scale %d%%\n",
                s_refl ? "on" : "off", s_scale_pct[s_scale_idx]);
    }
    PUSH32(g_esp, msg);
    g_ecx = row;
    PUSH32(g_esp, 0x000B5141u);
    EU_OPTION_CALL(0x0012B910u, sub_0012B910, f);
    opt_row_text(row, o, f);
    frame_restore(f, 12);
}
void sub_0009B680(void)
{
    sub_0009B680_gen();
    refl_views();
}
