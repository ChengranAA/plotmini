#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "test_runner.h"

#include <string.h>

/* ── helpers ───────────────────────────────────────────────────────── */

static int count_filled_rect(plm_fb *fb, int x0, int y0, int x1, int y1,
                              plm_color bg) {
    int cnt = 0, x, y;
    for (y = y0; y < y1; y++) {
        for (x = x0; x < x1; x++) {
            unsigned char *p = fb->pixels + y * fb->stride + x * 4;
            if (p[0] != bg.r || p[1] != bg.g || p[2] != bg.b)
                cnt++;
        }
    }
    return cnt;
}

static int outside_untouched(plm_fb *fb, int x0, int y0, int x1, int y1,
                              plm_color bg, int margin) {
    int x, y;
    int cx0 = x0 - margin, cy0 = y0 - margin;
    int cx1 = x1 + margin, cy1 = y1 + margin;
    for (y = 0; y < fb->height; y++) {
        for (x = 0; x < fb->width; x++) {
            if (x >= cx0 && x < cx1 && y >= cy0 && y < cy1) continue;
            unsigned char *p = fb->pixels + y * fb->stride + x * 4;
            if (p[0] != bg.r || p[1] != bg.g || p[2] != bg.b) return 0;
        }
    }
    return 1;
}

/* ── title margin auto-expansion ─────────────────────────────────── */

TEST(title_expands_top_margin)
{
    /* Plot with a title, small initial margin_top.  After render,
       margin_top should have been expanded to fit the title. */
    plm_plot p;
    plm_plot_init(&p);
    p.margin_top = 10;  /* too small for title */
    p.title = "My Plot Title";

    float x[] = {0, 1, 2};
    float y[] = {0, 1, 0};
    plm_plot_add_line(&p, x, y, 3,
        (plm_line_style){PLM_BLUE, 1.0f, NULL, 0, 0});

    plm_fb fb = plm_fb_create(malloc(400*300*4), 400, 300, PLM_RGBA8);
    plm_color bg = PLM_BG_COLOR;
    plm_fb_clear(&fb, bg);

    plm_render(&p, &fb);

    /* margin_top should have grown to at least fit the title text */
    int th = plm_text_height();
    ASSERT_TRUE(p.margin_top >= th + 6);  /* th + gap + gap */

    /* title should be rendered (some non-bg pixels in top region) */
    int top_pixels = count_filled_rect(&fb, 0, 0, 400, p.margin_top, bg);
    ASSERT_TRUE(top_pixels > 5);

    /* plot area should start below margin_top */
    ASSERT_TRUE(p.plot_area.y0 >= p.margin_top);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

TEST(title_no_title_leaves_margin_alone)
{
    /* Without a title, margin_top should not be auto-expanded */
    plm_plot p;
    plm_plot_init(&p);
    int orig_top = p.margin_top;

    float x[] = {0, 1};
    float y[] = {0, 1};
    plm_plot_add_line(&p, x, y, 2,
        (plm_line_style){PLM_BLUE, 1.0f, NULL, 0, 0});

    plm_fb fb = plm_fb_create(malloc(200*200*4), 200, 200, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_BG_COLOR);

    plm_render(&p, &fb);

    /* margin_top should be unchanged (no title to expand for) */
    ASSERT_EQ(p.margin_top, orig_top);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

TEST(title_long_title_fits)
{
    /* Long title should still get enough top margin */
    plm_plot p;
    plm_plot_init(&p);
    p.margin_top = 5;
    p.title = "This is a very long plot title that should still fit";

    float x[] = {0, 1};
    float y[] = {0, 1};
    plm_plot_add_line(&p, x, y, 2,
        (plm_line_style){PLM_RED, 1.0f, NULL, 0, 0});

    plm_fb fb = plm_fb_create(malloc(600*200*4), 600, 200, PLM_RGBA8);
    plm_color bg = PLM_BG_COLOR;
    plm_fb_clear(&fb, bg);

    plm_render(&p, &fb);

    /* margin_top should expand (title is 1 line tall regardless of width) */
    int th = plm_text_height();
    ASSERT_TRUE(p.margin_top >= th + 6);

    /* title should not overflow past cell top */
    ASSERT_TRUE(p.plot_area.y0 >= p.margin_top);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

/* ── legend OUTSIDE_RIGHT margin auto-expansion ──────────────────── */

TEST(legend_outside_right_expands_margin)
{
    plm_plot p;
    plm_plot_init(&p);
    p.margin_right = 10;  /* too small for legend */
    p.legend_position = PLM_LEGEND_OUTSIDE_RIGHT;

    float x[] = {0, 1, 2};
    float y[] = {0, 1, 0};
    plm_plot_add_line(&p, x, y, 3,
        (plm_line_style){PLM_BLUE, 1.0f, "series alpha", 0, 0});

    plm_fb fb = plm_fb_create(malloc(400*200*4), 400, 200, PLM_RGBA8);
    plm_color bg = PLM_BG_COLOR;
    plm_fb_clear(&fb, bg);

    plm_render(&p, &fb);

    /* margin_right should have grown to fit the legend box */
    ASSERT_TRUE(p.margin_right > 10);

    /* legend should be drawn to the right of plot_area */
    ASSERT_TRUE(p.plot_area.x1 + p.margin_right <= fb.width);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

TEST(legend_inside_does_not_expand_margin)
{
    /* TOP_RIGHT legend stays inside plot area – doesn't expand margin */
    plm_plot p;
    plm_plot_init(&p);
    int orig_right = p.margin_right;
    p.legend_position = PLM_LEGEND_TOP_RIGHT;

    float x[] = {0, 1};
    float y[] = {0, 1};
    plm_plot_add_line(&p, x, y, 2,
        (plm_line_style){PLM_RED, 1.0f, "series", 0, 0});

    plm_fb fb = plm_fb_create(malloc(200*200*4), 200, 200, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_BG_COLOR);

    plm_render(&p, &fb);

    /* margin_right should be unchanged (legend is inside) */
    ASSERT_EQ(p.margin_right, orig_right);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

TEST(legend_no_entries_leaves_margin_alone)
{
    plm_plot p;
    plm_plot_init(&p);
    int orig_right = p.margin_right;
    p.legend_position = PLM_LEGEND_OUTSIDE_RIGHT;

    /* line has no legend label */
    float x[] = {0, 1};
    float y[] = {0, 1};
    plm_plot_add_line(&p, x, y, 2,
        (plm_line_style){PLM_BLUE, 1.0f, NULL, 0, 0});

    plm_fb fb = plm_fb_create(malloc(200*200*4), 200, 200, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_BG_COLOR);

    plm_render(&p, &fb);

    /* no legend entries → margin_right stays */
    ASSERT_EQ(p.margin_right, orig_right);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

/* ── tick label horizontal overflow safety ──────────────────────── */

TEST(xtick_labels_stay_within_cell)
{
    /* Even with small margins, x-tick labels shouldn't crash or
       draw widely outside the cell. */
    plm_plot p;
    plm_plot_init(&p);
    p.margin_left  = 20;
    p.margin_right = 20;
    p.margin_bottom = 40;

    float x[] = {0, 10, 20};
    float y[] = {0, 5, 10};
    plm_plot_add_line(&p, x, y, 3,
        (plm_line_style){PLM_BLUE, 1.0f, NULL, 0, 0});

    plm_fb fb = plm_fb_create(malloc(200*150*4), 200, 150, PLM_RGBA8);
    plm_color bg = PLM_BG_COLOR;
    plm_fb_clear(&fb, bg);

    plm_render(&p, &fb);

    /* The plot area should be well within the cell */
    ASSERT_TRUE(p.plot_area.x0 >= 20);
    ASSERT_TRUE(p.plot_area.x1 <= 180);
    ASSERT_TRUE(p.plot_area.y1 <= 110);

    /* No pixels should be completely outside the cell */
    ASSERT_TRUE(outside_untouched(&fb, 0, 0, fb.width, fb.height, bg, 0));

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

TEST(ytick_labels_stay_within_cell)
{
    plm_plot p;
    plm_plot_init(&p);
    p.margin_left = 30;

    float x[] = {0, 1, 2};
    float y[] = {100, 200, 150};
    plm_plot_add_line(&p, x, y, 3,
        (plm_line_style){PLM_BLUE, 1.0f, NULL, 0, 0});

    plm_fb fb = plm_fb_create(malloc(200*150*4), 200, 150, PLM_RGBA8);
    plm_color bg = PLM_BG_COLOR;
    plm_fb_clear(&fb, bg);

    plm_render(&p, &fb);

    /* margin_left should expand to fit y-tick labels */
    ASSERT_TRUE(p.margin_left >= 30);

    /* plot area starts after margin */
    ASSERT_TRUE(p.plot_area.x0 >= p.margin_left);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

/* ── smoke: render with everything ───────────────────────────────── */

TEST(smoke_full_plot_with_all_decorations)
{
    plm_plot p;
    plm_plot_init(&p);
    p.title = "Full Plot Test";
    p.x_label = "X Axis";
    p.y_label = "Y Axis";
    p.margin_top = 10;
    p.margin_right = 10;
    p.legend_position = PLM_LEGEND_OUTSIDE_RIGHT;

    float x[] = {0, 1, 2, 3, 4};
    float y1[] = {0, 2, 1, 3, 2};
    float y2[] = {1, 3, 2, 1, 0};
    plm_plot_add_line(&p, x, y1, 5,
        (plm_line_style){PLM_BLUE, 1.5f, "alpha", 0, 0});
    plm_plot_add_line(&p, x, y2, 5,
        (plm_line_style){PLM_RED, 2.0f, "beta", 0, 0});

    plm_fb fb = plm_fb_create(malloc(500*350*4), 500, 350, PLM_RGBA8);
    plm_color bg = PLM_BG_COLOR;
    plm_fb_clear(&fb, bg);

    plm_render(&p, &fb);

    /* All margins should have been expanded */
    ASSERT_TRUE(p.margin_top >= 13);     /* title: th+gap+gap = 7+3+3 */
    ASSERT_TRUE(p.margin_right >= 40);   /* legend */
    ASSERT_TRUE(p.margin_bottom >= 20);  /* x ticks */
    ASSERT_TRUE(p.margin_left >= 25);    /* y ticks + label */

    /* plot area should be positive size */
    ASSERT_TRUE(p.plot_area.x1 > p.plot_area.x0);
    ASSERT_TRUE(p.plot_area.y1 > p.plot_area.y0);

    /* Plot area should fit inside cell */
    ASSERT_TRUE(p.plot_area.x0 >= 0);
    ASSERT_TRUE(p.plot_area.y0 >= 0);
    ASSERT_TRUE(p.plot_area.x1 <= fb.width);
    ASSERT_TRUE(p.plot_area.y1 <= fb.height);

    free(fb.pixels);
    plm_plot_reset(&p);
    return 0;
}

TEST_MAIN()
{
    RUN_TEST(title_expands_top_margin);
    RUN_TEST(title_no_title_leaves_margin_alone);
    RUN_TEST(title_long_title_fits);
    RUN_TEST(legend_outside_right_expands_margin);
    RUN_TEST(legend_inside_does_not_expand_margin);
    RUN_TEST(legend_no_entries_leaves_margin_alone);
    RUN_TEST(xtick_labels_stay_within_cell);
    RUN_TEST(ytick_labels_stay_within_cell);
    RUN_TEST(smoke_full_plot_with_all_decorations);
}
TEST_MAIN_END()
