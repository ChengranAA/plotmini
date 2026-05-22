#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "test_runner.h"

/* ---- figure init ---- */

TEST(figure_init_2x2)
{
    plm_figure fig;
    plm_figure_init(&fig, 2, 2);

    ASSERT_EQ(fig.nrows, 2);
    ASSERT_EQ(fig.ncols, 2);
    ASSERT_EQ(fig.hgap, 20);
    ASSERT_EQ(fig.vgap, 20);
    ASSERT_TRUE(fig.plots != NULL);

    /* all 4 plots should be initialised */
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(fig.plots[i].line_count, 0);
        ASSERT_EQ(fig.plots[i].hist_count, 0);
    }

    plm_figure_reset(&fig);
    return 0;
}

TEST(figure_init_1x1)
{
    plm_figure fig;
    plm_figure_init(&fig, 1, 1);

    ASSERT_EQ(fig.nrows, 1);
    ASSERT_EQ(fig.ncols, 1);
    ASSERT_TRUE(fig.plots != NULL);

    plm_figure_reset(&fig);
    return 0;
}

TEST(figure_init_bogus_dims)
{
    plm_figure fig;
    plm_figure_init(&fig, 0, -1);  /* clamped to 1 */

    ASSERT_EQ(fig.nrows, 1);
    ASSERT_EQ(fig.ncols, 1);

    plm_figure_reset(&fig);
    return 0;
}

/* ---- figure_plot ---- */

TEST(figure_plot_valid)
{
    plm_figure fig;
    plm_figure_init(&fig, 2, 3);

    plm_plot *p = plm_figure_plot(&fig, 1, 2);
    ASSERT_TRUE(p != NULL);
    /* should be the last plot in the array */
    ASSERT_TRUE(p == &fig.plots[1 * 3 + 2]);

    plm_figure_reset(&fig);
    return 0;
}

TEST(figure_plot_oob)
{
    plm_figure fig;
    plm_figure_init(&fig, 2, 2);

    ASSERT_TRUE(plm_figure_plot(&fig, -1, 0) == NULL);
    ASSERT_TRUE(plm_figure_plot(&fig, 0, -1) == NULL);
    ASSERT_TRUE(plm_figure_plot(&fig, 2, 0) == NULL);
    ASSERT_TRUE(plm_figure_plot(&fig, 0, 2) == NULL);

    plm_figure_reset(&fig);
    return 0;
}

TEST(figure_plot_null_fig)
{
    ASSERT_TRUE(plm_figure_plot(NULL, 0, 0) == NULL);
    return 0;
}

/* ---- figure plot independence ---- */

TEST(figure_plots_independent)
{
    plm_figure fig;
    plm_figure_init(&fig, 1, 2);

    /* add data to cell (0,0) */
    plm_plot *a = plm_figure_plot(&fig, 0, 0);
    a->title = "Plot A";
    float ax[] = { 1.f, 2.f, 3.f };
    float ay[] = { 4.f, 5.f, 6.f };
    plm_plot_add_line(a, ax, ay, 3, (plm_line_style){PLM_BLUE, 1.f, NULL, 0, 0});

    /* cell (0,1) should be empty */
    plm_plot *b = plm_figure_plot(&fig, 0, 1);
    b->title = "Plot B";

    ASSERT_EQ(a->line_count, 1);
    ASSERT_EQ(b->line_count, 0);
    ASSERT_TRUE(a != b);

    plm_figure_reset(&fig);
    return 0;
}

/* ---- figure render (smoke test) ---- */

TEST(figure_render_smoke)
{
    plm_figure fig;
    plm_figure_init(&fig, 2, 2);

    /* add a line to each cell */
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 2; c++) {
            plm_plot *p = plm_figure_plot(&fig, r, c);
            float x[] = { 0.f, 1.f, 2.f, 3.f, 4.f };
            float y[] = { 0.f, 1.f, 4.f, 9.f, 16.f };
            plm_plot_add_line(p, x, y, 5, (plm_line_style){PLM_BLUE, 1.f, NULL, 0, 0});
        }
    }

    /* render into a framebuffer */
    int w = 400, h = 300;
    plm_fb fb = plm_fb_create(NULL, w, h, PLM_RGBA8);
    plm_figure_render(&fig, &fb);

    /* just verify no crash; spot-check some pixels */
    ASSERT_TRUE(fb.pixels != NULL);

    /* check the framebuffer isn't all zero (some drawing happened) */
    int non_zero = 0;
    for (int i = 0; i < w * h * 4 && non_zero < 10; i++) {
        if (fb.pixels[i] != 0) non_zero++;
    }
    ASSERT_TRUE(non_zero > 0);

    PLOTMINI_FREE(fb.pixels);
    plm_figure_reset(&fig);
    return 0;
}

/* ---- figure reset ---- */

TEST(figure_reset_clears)
{
    plm_figure fig;
    plm_figure_init(&fig, 1, 1);

    plm_plot *p = plm_figure_plot(&fig, 0, 0);
    float x[] = { 1.f };
    float y[] = { 2.f };
    plm_plot_add_line(p, x, y, 1, (plm_line_style){PLM_BLUE, 1.f, NULL, 0, 0});

    plm_figure_reset(&fig);

    ASSERT_EQ(fig.nrows, 0);
    ASSERT_EQ(fig.ncols, 0);
    ASSERT_TRUE(fig.plots == NULL);

    return 0;
}

TEST_MAIN()
{
    RUN_TEST(figure_init_2x2);
    RUN_TEST(figure_init_1x1);
    RUN_TEST(figure_init_bogus_dims);
    RUN_TEST(figure_plot_valid);
    RUN_TEST(figure_plot_oob);
    RUN_TEST(figure_plot_null_fig);
    RUN_TEST(figure_plots_independent);
    RUN_TEST(figure_render_smoke);
    RUN_TEST(figure_reset_clears);
}
TEST_MAIN_END()
