#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "test_runner.h"

#include <math.h>

/* ---- isnan ---- */

TEST(isnan_normal)
{
    ASSERT_FALSE(plm__isnan(3.14f));
    ASSERT_FALSE(plm__isnan(0.0f));
    ASSERT_FALSE(plm__isnan(-1.0f));
    return 0;
}

TEST(isnan_nan)
{
    float nan_val = NAN;
    ASSERT_TRUE(plm__isnan(nan_val));
    ASSERT_TRUE(plm__isnan(NAN));
    return 0;
}

TEST(isnan_inf)
{
    ASSERT_FALSE(plm__isnan(INFINITY));
    ASSERT_FALSE(plm__isnan(-INFINITY));
    return 0;
}

/* ---- clamp ---- */

TEST(clamp_in_range)
{
    ASSERT_EQ(plm__clamp(5, 0, 10), 5);
    ASSERT_EQ(plm__clamp(0, 0, 10), 0);
    ASSERT_EQ(plm__clamp(10, 0, 10), 10);
    return 0;
}

TEST(clamp_below)
{
    ASSERT_EQ(plm__clamp(-5, 0, 10), 0);
    return 0;
}

TEST(clamp_above)
{
    ASSERT_EQ(plm__clamp(15, 0, 10), 10);
    return 0;
}

/* ---- min / max ---- */

TEST(min_max)
{
    ASSERT_EQ(plm__min(3, 7), 3);
    ASSERT_EQ(plm__max(3, 7), 7);
    ASSERT_EQ(plm__min(-2, -5), -5);
    ASSERT_EQ(plm__max(-2, -5), -2);
    return 0;
}

/* ---- nice_step ---- */

TEST(nice_step_10)
{
    double s = plm__nice_step(10.0, 5);
    /* range 10 / 5 = 2 → nice round: 2 */
    ASSERT_FEQ(s, 2.0, 0.01);
    return 0;
}

TEST(nice_step_1)
{
    double s = plm__nice_step(1.0, 5);
    /* range 1 / 5 = 0.2 → nice: 0.2 */
    ASSERT_FEQ(s, 0.2, 0.01);
    return 0;
}

TEST(nice_step_100)
{
    double s = plm__nice_step(100.0, 10);
    /* range 100 / 10 = 10 → nice: 10 */
    ASSERT_FEQ(s, 10.0, 0.01);
    return 0;
}

TEST(nice_step_zero_range)
{
    double s = plm__nice_step(0.0, 5);
    ASSERT_FEQ(s, 1.0, 0.01);
    return 0;
}

TEST(nice_step_negative)
{
    /* range should be non-negative; if passed negative, it's garbage */
    double s = plm__nice_step(-10.0, 5);
    /* just checks it doesn't crash; returns 1.0 for non-positive */
    ASSERT_TRUE(s == 1.0);
    return 0;
}

/* ---- coordinate mapping ---- */

TEST(map_x)
{
    plm_plot p;
    plm_plot_init(&p);
    p.x_axis.min = 0.0;
    p.x_axis.max = 10.0;
    p.plot_area.x0 = 100;
    p.plot_area.x1 = 200;

    ASSERT_FEQ(plm__map_x(&p, 0.0),  100.0, 0.01);
    ASSERT_FEQ(plm__map_x(&p, 10.0), 200.0, 0.01);
    ASSERT_FEQ(plm__map_x(&p, 5.0),  150.0, 0.01);

    plm_plot_reset(&p);
    return 0;
}

TEST(map_y)
{
    plm_plot p;
    plm_plot_init(&p);
    p.y_axis.min = 0.0;
    p.y_axis.max = 10.0;
    p.plot_area.y0 = 50;
    p.plot_area.y1 = 150;

    /* Y is flipped: data 0 -> pixel bottom (y1-1), data 10 -> pixel top (y0-1) */
    ASSERT_FEQ(plm__map_y(&p, 0.0),  149.0, 0.01);
    ASSERT_FEQ(plm__map_y(&p, 10.0),  49.0, 0.01);
    ASSERT_FEQ(plm__map_y(&p, 5.0),   99.0, 0.01);

    plm_plot_reset(&p);
    return 0;
}

/* ---- line clipping ---- */

TEST(clip_outcode_inside)
{
    /* clip rect: (0,0) - (100,100) */
    int code = plm__clip_outcode(50, 50, 0, 0, 100, 100);
    ASSERT_EQ(code, 0);  /* CLIP_INSIDE */
    return 0;
}

TEST(clip_outcode_left)
{
    int code = plm__clip_outcode(-5, 50, 0, 0, 100, 100);
    ASSERT_TRUE(code != 0);
    return 0;
}

TEST(clip_line_inside)
{
    /* both endpoints inside → accepted, coords unchanged */
    int x0 = 10, y0 = 10, x1 = 90, y1 = 90;
    int ok = plm__clip_line(&x0, &y0, &x1, &y1, 0, 0, 100, 100);
    ASSERT_TRUE(ok);
    ASSERT_EQ(x0, 10);
    ASSERT_EQ(y0, 10);
    ASSERT_EQ(x1, 90);
    ASSERT_EQ(y1, 90);
    return 0;
}

TEST(clip_line_completely_outside)
{
    /* both points far to the left */
    int x0 = -50, y0 = 10, x1 = -10, y1 = 10;
    int ok = plm__clip_line(&x0, &y0, &x1, &y1, 0, 0, 100, 100);
    ASSERT_FALSE(ok);
    return 0;
}

/* ---- autorange ---- */

TEST(autorange_lines_only)
{
    plm_plot p;
    plm_plot_init(&p);

    float x[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float y[] = { 10.0f, 20.0f, 15.0f, 25.0f, 30.0f };
    plm_plot_add_line(&p, x, y, 5, (plm_line_style){PLM_BLUE, 1.0f, NULL});

    /* trigger autorange (min==max on init) */
    plm__axis_autorange(&p.x_axis, p.lines, p.line_count,
                         p.hists, p.hist_count,
                         p.scatters, p.scatter_count, 0);
    plm__axis_autorange(&p.y_axis, p.lines, p.line_count,
                         p.hists, p.hist_count,
                         p.scatters, p.scatter_count, 1);

    /* X: 1..5 + 5% padding = 1 - 0.2 .. 5 + 0.2 */
    ASSERT_TRUE(p.x_axis.min < 1.0);
    ASSERT_TRUE(p.x_axis.max > 5.0);

    /* Y: 10..30 + 5% padding */
    ASSERT_TRUE(p.y_axis.min <= 10.0);
    ASSERT_TRUE(p.y_axis.max >= 30.0);

    plm_plot_reset(&p);
    return 0;
}

TEST(autorange_single_value)
{
    plm_plot p;
    plm_plot_init(&p);

    float x[] = { 5.0f };
    float y[] = { 7.0f };
    plm_plot_add_line(&p, x, y, 1, (plm_line_style){PLM_BLUE, 1.0f, NULL});

    plm__axis_autorange(&p.x_axis, p.lines, p.line_count,
                         p.hists, p.hist_count,
                         p.scatters, p.scatter_count, 0);
    plm__axis_autorange(&p.y_axis, p.lines, p.line_count,
                         p.hists, p.hist_count,
                         p.scatters, p.scatter_count, 1);

    /* single value → hi=lo+1, lo=lo-1 → 4..6 */
    ASSERT_TRUE(p.x_axis.min < 5.0);
    ASSERT_TRUE(p.x_axis.max > 5.0);
    ASSERT_TRUE(p.y_axis.min < 7.0);
    ASSERT_TRUE(p.y_axis.max > 7.0);

    plm_plot_reset(&p);
    return 0;
}

TEST(autorange_no_data)
{
    plm_plot p;
    plm_plot_init(&p);
    p.x_axis.min = p.x_axis.max = 0.0;  /* trigger: min==max */

    plm__axis_autorange(&p.x_axis, p.lines, p.line_count,
                         p.hists, p.hist_count,
                         p.scatters, p.scatter_count, 0);

    /* no data → defaults to 0..1 */
    ASSERT_EQ(p.x_axis.min, 0.0);
    ASSERT_EQ(p.x_axis.max, 1.0);

    plm_plot_reset(&p);
    return 0;
}

/* ---- text ---- */

TEST(text_width)
{
    int w = plm_text_width("Hello");
    /* 5 chars × 6 px advance = 30 */
    ASSERT_EQ(w, 30);
    return 0;
}

TEST(text_width_empty)
{
    int w = plm_text_width("");
    ASSERT_EQ(w, 0);
    return 0;
}

TEST(text_height)
{
    int h = plm_text_height();
    ASSERT_EQ(h, 7);
    return 0;
}

/* ---- fit_into ---- */

TEST(fit_into_same_aspect)
{
    plm_irect bounds = { 0, 0, 200, 100 };
    plm_irect r = plm_fit_into(bounds, 400, 200);  /* same 2:1 aspect */
    ASSERT_EQ(r.x0, 0);
    ASSERT_EQ(r.y0, 0);
    ASSERT_EQ(r.x1, 200);
    ASSERT_EQ(r.y1, 100);
    return 0;
}

TEST(fit_into_tall_image)
{
    plm_irect bounds = { 0, 0, 200, 200 };
    plm_irect r = plm_fit_into(bounds, 100, 400);  /* tall: 1:4 */
    /* should be centred, width 50, height 200 */
    ASSERT_TRUE(r.x1 - r.x0 > 0);
    ASSERT_TRUE(r.y1 - r.y0 > 0);
    /* aspect preserved: (x1-x0) / (y1-y0) ≈ 1/4 */
    double aspect = (double)(r.x1 - r.x0) / (double)(r.y1 - r.y0);
    ASSERT_FEQ(aspect, 0.25, 0.02);
    /* centred horizontally */
    int mid_x = (r.x0 + r.x1) / 2;
    ASSERT_TRUE(mid_x >= 95 && mid_x <= 105);
    return 0;
}

TEST_MAIN()
{
    RUN_TEST(isnan_normal);
    RUN_TEST(isnan_nan);
    RUN_TEST(isnan_inf);
    RUN_TEST(clamp_in_range);
    RUN_TEST(clamp_below);
    RUN_TEST(clamp_above);
    RUN_TEST(min_max);
    RUN_TEST(nice_step_10);
    RUN_TEST(nice_step_1);
    RUN_TEST(nice_step_100);
    RUN_TEST(nice_step_zero_range);
    RUN_TEST(nice_step_negative);
    RUN_TEST(map_x);
    RUN_TEST(map_y);
    RUN_TEST(clip_outcode_inside);
    RUN_TEST(clip_outcode_left);
    RUN_TEST(clip_line_inside);
    RUN_TEST(clip_line_completely_outside);
    RUN_TEST(autorange_lines_only);
    RUN_TEST(autorange_single_value);
    RUN_TEST(autorange_no_data);
    RUN_TEST(text_width);
    RUN_TEST(text_width_empty);
    RUN_TEST(text_height);
    RUN_TEST(fit_into_same_aspect);
    RUN_TEST(fit_into_tall_image);
}
TEST_MAIN_END()
