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
        "plotmini - error bar & band subplot demo", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels =
        (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- build a 2x2 subplot figure ---- */
    plm_figure fig;
    plm_figure_init(&fig, 2, 2);

    /* ================================================================ */
    /* Top-left: Error bars with BOTH x and y errors (measured data)    */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 0);
        p->title   = "Measured data (x & y errors)";
        p->x_label = "x";
        p->y_label = "y";

        const int n = 10;
        float *x     = (float *)malloc((size_t)n * sizeof(float));
        float *y     = (float *)malloc((size_t)n * sizeof(float));
        float *x_err = (float *)malloc((size_t)n * sizeof(float));
        float *y_err = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i]     = 0.5f + (float)i * 0.9f;
            y[i]     = 2.0f + 0.6f * (float)i +
                       1.5f * sinf((float)i * 0.7f);
            x_err[i] = 0.12f + (float)i * 0.02f;
            y_err[i] = 0.3f + fabsf(sinf((float)i * 0.7f)) * 0.5f;
        }
        plm_errorbar_style es = {
            PLM_BLUE,       /* line_color */
            1.5f,           /* line_width */
            5.0f,           /* cap_size */
            PLM_RED,        /* marker_color */
            3.5f,           /* marker_radius */
            "data"
        };
        plm_plot_add_errorbar(p, x, y, x_err, y_err, n, es);
        free(x); free(y); free(x_err); free(y_err);
    }

    /* ================================================================ */
    /* Top-right: Error bars with y-errors only + reference line        */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 1);
        p->title   = "Linear fit with y errors";
        p->x_label = "x";
        p->y_label = "y";

        /* error bar points */
        const int n = 11;
        float *x     = (float *)malloc((size_t)n * sizeof(float));
        float *y     = (float *)malloc((size_t)n * sizeof(float));
        float *y_err = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i]     = (float)i;
            y[i]     = 1.5f + 0.8f * (float)i +
                       0.6f * ((float)rand() / (float)RAND_MAX - 0.5f);
            y_err[i] = 0.40f;
        }
        plm_errorbar_style es = {
            PLM_GREY(30),   /* line_color */
            1.2f,           /* line_width */
            4.0f,           /* cap_size */
            PLM_GREEN,      /* marker_color */
            3.0f,           /* marker_radius */
            "points ± σ"
        };
        plm_plot_add_errorbar(p, x, y, NULL, y_err, n, es);
        free(x); free(y); free(y_err);

        /* best-fit line overlaid */
        {
            float lx[] = { 0.0f, 10.0f };
            float ly[] = { 1.5f, 1.5f + 0.8f * 10.0f };
            plm_line_style ls = { PLM_RED, 1.8f, "fit: y=0.8x+1.5", 0};
            plm_plot_add_line(p, lx, ly, 2, ls);
        }
    }

    /* ================================================================ */
    /* Bottom-left: Error band (confidence interval around sine)        */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 0);
        p->title   = "sin(x) with 95% confidence band";
        p->x_label = "x";
        p->y_label = "y";

        const int n = 200;
        float *x       = (float *)malloc((size_t)n * sizeof(float));
        float *y_lower = (float *)malloc((size_t)n * sizeof(float));
        float *y_upper = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i]  = (float)i * 0.05f * (float)M_PI;
            float base = sinf(x[i]);
            /* envelope that widens at peaks */
            float env  = 0.25f + fabsf(base) * 0.35f;
            y_lower[i] = base - env;
            y_upper[i] = base + env;
        }
        plm_band_style bs = {
            PLM_RGBA(255, 180, 180, 100),  /* light red, semi-transparent */
            PLM_RED,                        /* stroke */
            1.0f,                           /* stroke_width */
            "95% CI"
        };
        plm_plot_add_band(p, x, y_lower, y_upper, n, bs);
        free(x); free(y_lower); free(y_upper);

        /* sine line on top */
        {
            float *sx = (float *)malloc((size_t)n * sizeof(float));
            float *sy = (float *)malloc((size_t)n * sizeof(float));
            for (int i = 0; i < n; i++) {
                sx[i] = (float)i * 0.05f * (float)M_PI;
                sy[i] = sinf(sx[i]);
            }
            plm_line_style ls = { PLM_BLUE, 2.0f, "sin(x)", 0};
            plm_plot_add_line(p, sx, sy, n, ls);
            free(sx); free(sy);
        }
    }

    /* ================================================================ */
    /* Bottom-right: Error bars OVER an error band (combined)           */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 1);
        p->title   = "Damped oscillator ± envelope";
        p->x_label = "t";
        p->y_label = "amplitude";

        const int n = 200;
        float *tx      = (float *)malloc((size_t)n * sizeof(float));
        float *ty_low  = (float *)malloc((size_t)n * sizeof(float));
        float *ty_high = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            tx[i]        = (float)i * 0.04f * (float)M_PI;
            float decay  = expf(-tx[i] * 0.25f);
            float env    = decay;
            ty_low[i]    = -env;
            ty_high[i]   =  env;
        }
        /* band: the theoretical envelope */
        plm_band_style bs = {
            PLM_RGBA(180, 180, 255, 100),  /* light blue, semi-transparent */
            PLM_BLUE,                       /* stroke */
            0.8f,                           /* stroke_width */
            "± envelope"
        };
        plm_plot_add_band(p, tx, ty_low, ty_high, n, bs);
        free(tx); free(ty_low); free(ty_high);

        /* error bars: sampled measurements at sparse t values */
        {
            const int m = 12;
            float *sx     = (float *)malloc((size_t)m * sizeof(float));
            float *sy     = (float *)malloc((size_t)m * sizeof(float));
            float *sy_err = (float *)malloc((size_t)m * sizeof(float));
            for (int i = 0; i < m; i++) {
                float t = (float)i * 0.6f * (float)M_PI;
                float decay = expf(-t * 0.25f);
                sx[i]     = t;
                sy[i]     = decay * sinf(t * 3.0f) +
                             0.12f * ((float)rand() / (float)RAND_MAX - 0.5f);
                sy_err[i] = 0.25f;
            }
            plm_errorbar_style es = {
                PLM_GREY(20),   /* line_color */
                1.5f,           /* line_width */
                5.0f,           /* cap_size */
                PLM_RED,        /* marker_color */
                4.0f,           /* marker_radius */
                "samples"
            };
            plm_plot_add_errorbar(p, sx, sy, NULL, sy_err, m, es);
            free(sx); free(sy); free(sy_err);
        }
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
