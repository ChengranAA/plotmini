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
    const int win_w = 800;
    const int win_h = 600;

    struct mfb_window *win = mfb_open("plotmini - scatter demo", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    plm_plot plot;
    plm_plot_init(&plot);
    plot.title   = "Scatter plot demo";
    plot.x_label = "x";
    plot.y_label = "y";

    /* ---- scatter: 200 random points ---- */
    {
        const int n = 200;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)rand() / (float)RAND_MAX * 10.0f;
            y[i] = (float)rand() / (float)RAND_MAX * 10.0f;
        }
        plm_scatter_style ss = { PLM_RED, 3.0f, "random" };
        plm_plot_add_scatter(&plot, x, y, n, ss);
        free(x); free(y);
    }

    /* ---- scatter: sine wave sampled points ---- */
    {
        const int n = 50;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.2f;
            y[i] = sinf(x[i]) * 2.0f + 5.0f;
        }
        plm_scatter_style ss = { PLM_BLUE, 4.0f, "sin samples" };
        plm_plot_add_scatter(&plot, x, y, n, ss);
        free(x); free(y);
    }

    /* ---- line: connect the sine samples ---- */
    {
        const int n = 100;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.1f;
            y[i] = sinf(x[i]) * 2.0f + 5.0f;
        }
        plm_line_style ls = { PLM_GREY(80), 0.5f, "sin curve", 0, 0};
        plm_plot_add_line(&plot, x, y, n, ls);
        free(x); free(y);
    }

    plm_render(&plot, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {
        /* static */
    }

    plm_plot_reset(&plot);
    free(pixels);
    return 0;
}
