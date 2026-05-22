#define PLOTMINI_TEXT_SCALE 2
#define PLOTMINI_DARK_THEME
#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    const int win_w = 960;
    const int win_h = 720;

    struct mfb_window *win = mfb_open(
        "plotmini - new features demo (step, stem, asym, log, legend, dual Y)", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels =
        (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    plm_figure fig;
    plm_figure_init(&fig, 2, 2);

    /* ================================================================ */
    /* Top-left: Step plots (pre, post, mid) with legend                */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 0);
        p->title   = "Step plot modes (pre / post / mid)";
        p->x_label = "x";
        p->y_label = "y";
        p->legend_position = PLM_LEGEND_TOP_LEFT;

        const int n = 12;
        float x[n], y[n];
        int i;
        for (i = 0; i < n; i++) {
            x[i] = (float)i;
            y[i] = 1.0f + (float)(i % 3) * 1.5f + 0.3f * (float)i;
        }

        plm_line_style pre_s  = { PLM_BLUE,  2.0f, "step-pre",  PLM_STEP_PRE };
        plm_line_style post_s = { PLM_RED,   2.0f, "step-post", PLM_STEP_POST };
        plm_line_style mid_s  = { PLM_GREEN, 2.0f, "step-mid",  PLM_STEP_MID };

        /* offset the post and mid data slightly for visual separation */
        float y_post[n], y_mid[n];
        for (i = 0; i < n; i++) {
            y_post[i] = y[i] + 1.5f;
            y_mid[i]  = y[i] + 3.0f;
        }

        plm_plot_add_line(p, x, y,      n, pre_s);
        plm_plot_add_line(p, x, y_post, n, post_s);
        plm_plot_add_line(p, x, y_mid,  n, mid_s);
    }

    /* ================================================================ */
    /* Top-right: Stem plot on log-scale Y with custom tick formatter   */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 1);
        p->title   = "Stem plot (log Y, custom ticks)";
        p->x_label = "x";
        p->y_label = "magnitude";
        p->y_axis.scale = PLM_LOG;
        p->y_axis.tick_formatter = plm_tick_fmt_log10;
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        const int n = 15;
        float x[n], y[n];
        int i;
        for (i = 0; i < n; i++) {
            x[i] = (float)i * 0.7f;
            y[i] = powf(10.0f, 0.3f + 0.25f * (float)i +
                        0.4f * sinf((float)i * 0.9f));
        }

        plm_stem_style ss = {
            PLM_CYAN,       /* line_color */
            2.0f,           /* line_width */
            PLM_YELLOW,     /* marker_color */
            4.0f,           /* marker_radius */
            "measurements"  /* legend (not shown, no legend enabled) */
        };
        plm_plot_add_stem(p, x, y, n, 1.0 /* baseline */, ss);
    }

    /* ================================================================ */
    /* Bottom-left: Asymmetric error bars with legend                   */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 0);
        p->title   = "Asymmetric error bars";
        p->x_label = "x";
        p->y_label = "y";
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        const int n = 8;
        float x[n], y[n], y_lo[n], y_hi[n];
        int i;
        for (i = 0; i < n; i++) {
            x[i]    = 1.0f + (float)i * 1.2f;
            y[i]    = 2.0f + 0.8f * (float)i + 1.2f * sinf((float)i * 0.6f);
            /* asymmetric: larger error upward, smaller downward */
            y_lo[i] = 0.25f + 0.1f * (float)i;
            y_hi[i] = 0.60f + 0.2f * (float)i;
        }

        plm_errorbar_style es = {
            PLM_BLUE,       /* line_color */
            1.5f,           /* line_width */
            4.0f,           /* cap_size */
            PLM_RED,        /* marker_color */
            3.5f,           /* marker_radius */
            "asym ±err"
        };
        plm_plot_add_errorbar_asym(p, x, y,
            NULL, NULL,         /* no x error */
            y_lo, y_hi,         /* asymmetric y error */
            n, es);

        /* also add a symmetric reference for comparison */
        float y_sym_err[n];
        for (i = 0; i < n; i++) {
            y_sym_err[i] = 0.4f;  /* constant symmetric error */
            y[i] += 3.0f;         /* offset upward */
        }
        plm_errorbar_style es2 = {
            PLM_GREY(30),   /* line_color */
            1.2f,           /* line_width */
            3.0f,           /* cap_size */
            PLM_GREEN,      /* marker_color */
            3.0f,           /* marker_radius */
            "sym err"
        };
        plm_plot_add_errorbar(p, x, y, NULL, y_sym_err, n, es2);
    }

    /* ================================================================ */
    /* Bottom-right: Secondary (right) Y-axis                           */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 1);
        p->title   = "Dual Y-axis (left & right)";
        p->x_label = "time (s)";
        p->y_label = "Temperature (°C)";
        p->y_label_right = "Pressure (kPa)";
        p->legend_position = PLM_LEGEND_BOTTOM_RIGHT;

        /* configure right y-axis with a different range */
        p->y_axis_right.min  = 90.0;
        p->y_axis_right.max  = 115.0;
        p->y_axis_right.tick_hint = 5;
        p->y_axis_right.label = NULL;  /* label is set via y_label_right */

        const int n = 50;
        float tx[n], temp[n], pres[n];
        int i;
        for (i = 0; i < n; i++) {
            tx[i]   = (float)i * 0.2f;
            /* temperature: rising then stable (left axis) */
            temp[i] = 20.0f + 60.0f * (1.0f - expf(-tx[i] * 0.5f)) +
                      3.0f * sinf(tx[i] * 0.7f);
            /* pressure: correlated but different scale (right axis) */
            pres[i] = 100.0f + 10.0f * sinf(tx[i] * 0.5f) +
                      2.0f * cosf(tx[i] * 0.3f);
        }

        plm_line_style temp_s = { PLM_RED,    2.0f, "Temp (°C)", 0 };
        plm_line_style pres_s = { PLM_BLUE,   1.5f, "Pres (kPa)", 0 };

        plm_plot_add_line(p, tx, temp, n, temp_s);
        plm_plot_add_line(p, tx, pres, n, pres_s);
    }

    /* Render and display */
    plm_figure_render(&fig, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {
        /* idle */
    }

    plm_figure_reset(&fig);
    free(pixels);
    return 0;
}
