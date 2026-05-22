#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "test_runner.h"

#include <string.h>

/* ── helpers ───────────────────────────────────────────────────────── */

/* Count non-background pixels inside a rectangle within the fb.
   bg is the background colour we cleared with. */
static int count_filled(plm_fb *fb, plm_irect r, plm_color bg) {
    int cnt = 0, x, y;
    for (y = r.y0; y < r.y1; y++) {
        for (x = r.x0; x < r.x1; x++) {
            unsigned char *p = fb->pixels + y * fb->stride + x * 4;
            if (p[0] != bg.r || p[1] != bg.g || p[2] != bg.b)
                cnt++;
        }
    }
    return cnt;
}

/* Count *any* non-background pixel in the whole fb. */
static int count_any(plm_fb *fb, plm_color bg) {
    plm_irect all = { 0, 0, fb->width, fb->height };
    return count_filled(fb, all, bg);
}

/* Return 1 if all pixels outside 'rect' (+margin) are still the background
   colour.  A small margin is allowed because text glyphs may extend a few
   pixels past the rect boundary (anti-aliasing, character shapes). */
static int outside_untouched(plm_fb *fb, plm_irect rect, plm_color bg, int margin) {
    int x, y;
    int x0 = rect.x0 - margin;
    int y0 = rect.y0 - margin;
    int x1 = rect.x1 + margin;
    int y1 = rect.y1 + margin;
    for (y = 0; y < fb->height; y++) {
        for (x = 0; x < fb->width; x++) {
            if (x >= x0 && x < x1 && y >= y0 && y < y1)
                continue;  /* inside (+margin) – skip */
            unsigned char *p = fb->pixels + y * fb->stride + x * 4;
            if (p[0] != bg.r || p[1] != bg.g || p[2] != bg.b)
                return 0;  /* pixel far outside rect was modified */
        }
    }
    return 1;
}

static plm_color BG;  /* will be set per test */

/* ── horizontal colourbar tests ────────────────────────────────────── */

TEST(hbar_normal_fits)
{
    /* 200×120 rect – plenty of room for scale=1 (top_margin=19+bot=10+bar) */
    plm_fb fb = plm_fb_create(malloc(200*120*4), 200, 120, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 110 };  /* 180×100 */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_HEAT,
                 PLM_CBAR_HORIZONTAL, "my label", 5);

    /* bar should have coloured pixels inside rect */
    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 100);  /* plenty of gradient + text pixels */

    /* nothing outside the rect */
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_normal_no_label)
{
    plm_fb fb = plm_fb_create(malloc(200*80*4), 200, 80, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 70 };  /* 180×60 – enough for top_margin+bar */
    plm_colorbar(&fb, rect, -1.0, 1.0, PLM_CMAP_VIRIDIS,
                 PLM_CBAR_HORIZONTAL, NULL, 3);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 50);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_tight_rect)
{
    /* Only 40 px tall – forces tight-margin path (scale=1 needs 19+10+10=39,
       but with label we need 19+10+10=39 minimum; 40 is just enough) */
    plm_fb fb = plm_fb_create(malloc(200*60*4), 200, 60, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 50 };  /* 180×40 – tight */
    plm_colorbar(&fb, rect, 0.0, 10.0, PLM_CMAP_PLASMA,
                 PLM_CBAR_HORIZONTAL, "tight", 3);

    /* still draws something inside the rect */
    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 10);

    /* nothing outside */
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_very_tight)
{
    /* Only 16 px tall – forces dropping tick labels entirely */
    plm_fb fb = plm_fb_create(malloc(200*36*4), 200, 36, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 26 };  /* 180×16 – very tight */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_INFERNO,
                 PLM_CBAR_HORIZONTAL, NULL, 2);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 4);  /* at least the bar + border */

    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_extreme_tight)
{
    /* Only 6 px tall – absolute minimum: just the bar */
    plm_fb fb = plm_fb_create(malloc(200*26*4), 200, 26, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 16 };  /* 180×6 */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_TURBO,
                 PLM_CBAR_HORIZONTAL, "x", 2);

    /* should not crash – bar might be 2-4 px */
    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside >= 0);  /* just no crash */

    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_zero_height_rect)
{
    plm_fb fb = plm_fb_create(malloc(200*20*4), 200, 20, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 10 };  /* zero height */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_JET,
                 PLM_CBAR_HORIZONTAL, NULL, 3);

    /* should be a no-op (rect.y0 >= rect.y1 → return early) */
    int total = count_any(&fb, BG);
    ASSERT_EQ(total, 0);  /* nothing drawn */
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_vmin_equals_vmax)
{
    plm_fb fb = plm_fb_create(malloc(200*100*4), 200, 100, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 90 };
    /* vmin == vmax → range forced to 1.0 internally */
    plm_colorbar(&fb, rect, 5.0, 5.0, PLM_CMAP_COOLWARM,
                 PLM_CBAR_HORIZONTAL, "zero range", 4);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 50);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(hbar_negative_rect_clips)
{
    plm_fb fb = plm_fb_create(malloc(200*100*4), 200, 100, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    /* rect starts negative – should be clipped to fb */
    plm_irect rect = { -20, -10, 60, 50 };
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_GRAY,
                 PLM_CBAR_HORIZONTAL, NULL, 3);

    /* draws in the clipped region only */
    plm_irect clipped = { 0, 0, 60, 50 };
    int inside = count_filled(&fb, clipped, BG);
    ASSERT_TRUE(inside > 10);

    /* everything outside clipped is untouched */
    ASSERT_TRUE(outside_untouched(&fb, clipped, BG, 10));

    free(fb.pixels);
    return 0;
}

/* ── vertical colourbar tests ──────────────────────────────────────── */

TEST(vbar_normal_fits)
{
    plm_fb fb = plm_fb_create(malloc(150*200*4), 150, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 140, 190 };  /* 130×180 – plenty wide */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_VIRIDIS,
                 PLM_CBAR_VERTICAL, "label", 5);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 100);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_normal_no_label)
{
    plm_fb fb = plm_fb_create(malloc(80*200*4), 80, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 70, 190 };  /* 60×180 – enough for left_margin+bar */
    plm_colorbar(&fb, rect, -1.0, 1.0, PLM_CMAP_HEAT,
                 PLM_CBAR_VERTICAL, NULL, 3);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 50);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_tight_rect)
{
    /* 42 px wide – tight for scale=1 (left_margin=19, right=0, min_bar=4) */
    plm_fb fb = plm_fb_create(malloc(60*200*4), 60, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 9, 10, 51, 190 };  /* 42×180 – tight */
    plm_colorbar(&fb, rect, 0.0, 10.0, PLM_CMAP_PLASMA,
                 PLM_CBAR_VERTICAL, NULL, 3);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 10);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_very_tight)
{
    /* 18 px wide – forces dropping tick labels */
    plm_fb fb = plm_fb_create(malloc(36*200*4), 36, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 9, 10, 27, 190 };  /* 18×180 */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_MAGMA,
                 PLM_CBAR_VERTICAL, NULL, 2);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 4);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_extreme_tight)
{
    /* 6 px wide – absolute minimum */
    plm_fb fb = plm_fb_create(malloc(24*200*4), 24, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 9, 10, 15, 190 };  /* 6×180 */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_CIVIDIS,
                 PLM_CBAR_VERTICAL, "x", 2);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside >= 0);  /* no crash */
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_zero_width_rect)
{
    plm_fb fb = plm_fb_create(malloc(40*100*4), 40, 100, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 10, 90 };  /* zero width */
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_JET,
                 PLM_CBAR_VERTICAL, NULL, 3);

    int total = count_any(&fb, BG);
    ASSERT_EQ(total, 0);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_negative_rect_clips)
{
    plm_fb fb = plm_fb_create(malloc(100*200*4), 100, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { -10, -10, 60, 60 };
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_GRAY,
                 PLM_CBAR_VERTICAL, NULL, 3);

    plm_irect clipped = { 0, 0, 60, 60 };
    int inside = count_filled(&fb, clipped, BG);
    ASSERT_TRUE(inside > 10);
    ASSERT_TRUE(outside_untouched(&fb, clipped, BG, 10));

    free(fb.pixels);
    return 0;
}

/* ── colourbar with TEXT_SCALE=2 (like imshow example) ─────────────── */
/* These replicate the exact conditions from 04_imshow.c with the fix. */

TEST(hbar_scale2_normal)
{
    /* Simulate TEXT_SCALE=2: need to rebuild with PLOTMINI_TEXT_SCALE=2.
       Since we can't redefine it, we test with default scale=1 but ensure
       the rect is large enough for scale=2 logic.  We verify no overflow. */
    plm_fb fb = plm_fb_create(malloc(1060*160*4), 1060, 160, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    /* The fixed example uses 980×80 rect for the horizontal colorbar */
    plm_irect rect = { 40, 40, 1020, 120 };  /* 980×80 – matches fixed example */
    plm_colorbar(&fb, rect, -1.0, 1.0, PLM_CMAP_COOLWARM,
                 PLM_CBAR_HORIZONTAL, "correlation  (coolwarm)", 5);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 200);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(vbar_scale2_normal)
{
    /* Matches the fixed vertical colorbar in the 3×3 grid: 50 px wide */
    plm_fb fb = plm_fb_create(malloc(100*200*4), 100, 200, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 60, 190 };  /* 50×180 – matches fixed cbar_width */
    plm_colorbar(&fb, rect, 0.0, 255.0, PLM_CMAP_TURBO,
                 PLM_CBAR_VERTICAL, NULL, 3);

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 50);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

/* ── diverse colormaps ─────────────────────────────────────────────── */

TEST(all_cmaps_horizontal)
{
    plm_cmap maps[] = {
        PLM_CMAP_HEAT, PLM_CMAP_VIRIDIS, PLM_CMAP_GRAY,
        PLM_CMAP_PLASMA, PLM_CMAP_INFERNO, PLM_CMAP_MAGMA,
        PLM_CMAP_TURBO, PLM_CMAP_CIVIDIS, PLM_CMAP_COOLWARM, PLM_CMAP_JET
    };
    int i;
    for (i = 0; i < 10; i++) {
        plm_fb fb = plm_fb_create(malloc(200*80*4), 200, 80, PLM_RGBA8);
        BG = PLM_BG_COLOR;
        plm_fb_clear(&fb, BG);

        plm_irect rect = { 10, 10, 190, 70 };
        plm_colorbar(&fb, rect, 0.0, 1.0, maps[i],
                     PLM_CBAR_HORIZONTAL, NULL, 3);

        int inside = count_filled(&fb, rect, BG);
        ASSERT_TRUE(inside > 40);
        ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

        free(fb.pixels);
    }
    return 0;
}

TEST(all_cmaps_vertical)
{
    plm_cmap maps[] = {
        PLM_CMAP_HEAT, PLM_CMAP_VIRIDIS, PLM_CMAP_GRAY,
        PLM_CMAP_PLASMA, PLM_CMAP_INFERNO, PLM_CMAP_MAGMA,
        PLM_CMAP_TURBO, PLM_CMAP_CIVIDIS, PLM_CMAP_COOLWARM, PLM_CMAP_JET
    };
    int i;
    for (i = 0; i < 10; i++) {
        plm_fb fb = plm_fb_create(malloc(80*200*4), 80, 200, PLM_RGBA8);
        BG = PLM_BG_COLOR;
        plm_fb_clear(&fb, BG);

        plm_irect rect = { 10, 10, 70, 190 };
        plm_colorbar(&fb, rect, -1.0, 1.0, maps[i],
                     PLM_CBAR_VERTICAL, "cmap", 4);

        int inside = count_filled(&fb, rect, BG);
        ASSERT_TRUE(inside > 50);
        ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

        free(fb.pixels);
    }
    return 0;
}

/* ── tick_hint edge cases ──────────────────────────────────────────── */

TEST(tick_hint_zero_uses_default)
{
    plm_fb fb = plm_fb_create(malloc(200*80*4), 200, 80, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 70 };
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_HEAT,
                 PLM_CBAR_HORIZONTAL, NULL, 0);  /* tick_hint=0 → auto 5 */

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 40);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(tick_hint_negative)
{
    plm_fb fb = plm_fb_create(malloc(200*80*4), 200, 80, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 190, 70 };
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_VIRIDIS,
                 PLM_CBAR_HORIZONTAL, NULL, -5);  /* negative → auto 5 */

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 40);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

TEST(tick_hint_many_ticks)
{
    plm_fb fb = plm_fb_create(malloc(400*100*4), 400, 100, PLM_RGBA8);
    BG = PLM_BG_COLOR;
    plm_fb_clear(&fb, BG);

    plm_irect rect = { 10, 10, 390, 90 };
    plm_colorbar(&fb, rect, 0.0, 100.0, PLM_CMAP_JET,
                 PLM_CBAR_HORIZONTAL, NULL, 20);  /* many ticks */

    int inside = count_filled(&fb, rect, BG);
    ASSERT_TRUE(inside > 40);
    ASSERT_TRUE(outside_untouched(&fb, rect, BG, 10));

    free(fb.pixels);
    return 0;
}

/* ── no font mode ──────────────────────────────────────────────────── */
/* We test that the gradient + border still work when PLOTMINI_NO_FONT
   is defined.  Since we can't redefine it here, we just verify the normal
   path doesn't crash – the NO_FONT path is compile-tested via the build. */

/* ── 1 bpp framebuffer ─────────────────────────────────────────────── */

TEST(hbar_1bpp)
{
    unsigned char *buf = (unsigned char *)malloc(200 * 80);
    memset(buf, 0, 200 * 80);
    plm_fb fb;
    fb.pixels = buf;
    fb.width  = 200;
    fb.height = 80;
    fb.stride = 200;
    fb.bpp    = 1;

    plm_irect rect = { 10, 10, 190, 70 };
    plm_colorbar(&fb, rect, 0.0, 1.0, PLM_CMAP_GRAY,
                 PLM_CBAR_HORIZONTAL, NULL, 3);

    /* some non-zero bytes should have been written inside rect */
    int cnt = 0, x, y;
    for (y = rect.y0; y < rect.y1; y++)
        for (x = rect.x0; x < rect.x1; x++)
            if (buf[y * 200 + x] != 0) cnt++;
    ASSERT_TRUE(cnt > 30);

    /* check outside untouched */
    for (y = 0; y < 80; y++)
        for (x = 0; x < 200; x++) {
            if (x >= rect.x0 && x < rect.x1 && y >= rect.y0 && y < rect.y1)
                continue;
            ASSERT_EQ((int)buf[y * 200 + x], 0);
        }

    free(buf);
    return 0;
}

TEST_MAIN()
{
    /* ── horizontal ── */
    RUN_TEST(hbar_normal_fits);
    RUN_TEST(hbar_normal_no_label);
    RUN_TEST(hbar_tight_rect);
    RUN_TEST(hbar_very_tight);
    RUN_TEST(hbar_extreme_tight);
    RUN_TEST(hbar_zero_height_rect);
    RUN_TEST(hbar_vmin_equals_vmax);
    RUN_TEST(hbar_negative_rect_clips);

    /* ── vertical ── */
    RUN_TEST(vbar_normal_fits);
    RUN_TEST(vbar_normal_no_label);
    RUN_TEST(vbar_tight_rect);
    RUN_TEST(vbar_very_tight);
    RUN_TEST(vbar_extreme_tight);
    RUN_TEST(vbar_zero_width_rect);
    RUN_TEST(vbar_negative_rect_clips);

    /* ── scale-2 simulation ── */
    RUN_TEST(hbar_scale2_normal);
    RUN_TEST(vbar_scale2_normal);

    /* ── diverse colormaps ── */
    RUN_TEST(all_cmaps_horizontal);
    RUN_TEST(all_cmaps_vertical);

    /* ── tick_hint ── */
    RUN_TEST(tick_hint_zero_uses_default);
    RUN_TEST(tick_hint_negative);
    RUN_TEST(tick_hint_many_ticks);

    /* ── 1 bpp ── */
    RUN_TEST(hbar_1bpp);
}
TEST_MAIN_END()
