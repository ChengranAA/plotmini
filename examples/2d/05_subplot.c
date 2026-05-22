#define PLOTMINI_IMPLEMENTATION
#include "../../plotmini.h"
#include "../dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    const int win_w = 900;
    const int win_h = 680;

    struct mfb_window *win = mfb_open("plotmini - subplot demo", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- build a 2x2 subplot figure ---- */
    plm_figure fig;
    plm_figure_init(&fig, 2, 2);

    /* Top-left: sine wave */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 0);
        p->title   = "sin(x)";
        p->x_label = "x";
        p->y_label = "y";

        const int n = 100;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.1f;
            y[i] = sinf(x[i]);
        }
        plm_line_style s = { PLM_BLUE, 1.5f, "sin(x)", 0, 0};
        plm_plot_add_line(p, x, y, n, s);
        free(x); free(y);
    }

    /* Top-right: cosine with noise */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 1);
        p->title   = "cos(x) + noise";
        p->x_label = "x";
        p->y_label = "y";

        const int n = 120;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.1f;
            y[i] = cosf(x[i]) + 0.15f * ((float)rand() / (float)RAND_MAX - 0.5f);
        }
        plm_line_style s = { PLM_RED, 1.0f, "cos(x)+noise", 0, 0};
        plm_plot_add_line(p, x, y, n, s);
        free(x); free(y);
    }

    /* Bottom-left: histogram of normal distribution */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 0);
        p->title   = "N(0,1) histogram";
        p->x_label = "value";
        p->y_label = "count";

        const int n = 2000;
        float *samples = (float *)malloc((size_t)n * sizeof(float));
        /* Box-Muller transform */
        for (int i = 0; i < n; i += 2) {
            float u1 = (float)rand() / (float)RAND_MAX;
            float u2 = (float)rand() / (float)RAND_MAX;
            if (u1 < 1e-9f) u1 = 1e-9f;
            float r = sqrtf(-2.0f * logf(u1));
            float theta = 2.0f * (float)M_PI * u2;
            samples[i] = r * cosf(theta);
            if (i + 1 < n) samples[i + 1] = r * sinf(theta);
        }
        plm_bar_style bs = { PLM_GREEN, PLM_BLACK, 1.0f, "N(0,1)" };
        plm_plot_add_hist(p, samples, n, 30, 0, bs);
        free(samples);
    }

    /* Bottom-right: scatter-like line (x²) */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 1);
        p->title   = "x^2";
        p->x_label = "x";
        p->y_label = "y";
        p->y_axis.grid = 1;

        const int n = 80;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)(i - n/2) * 0.15f;
            y[i] = x[i] * x[i];
        }
        plm_line_style s = { PLM_MAGENTA, 2.0f, "x^2", 0, 0};
        plm_plot_add_line(p, x, y, n, s);
        free(x); free(y);
    }

    /* Render and display */
    plm_figure_render(&fig, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {
        /* idle -- static plot */
    }

    plm_figure_reset(&fig);
    free(pixels);
    return 0;
}
