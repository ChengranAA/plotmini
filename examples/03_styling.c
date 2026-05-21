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
    const int win_w = 900;
    const int win_h = 650;

    struct mfb_window *win = mfb_open("plotmini - styling", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    plm_plot plot;
    plm_plot_init(&plot);
    plot.title   = "Styling Demo";
    plot.x_label = "x";
    plot.y_label = "y";

    plot.margin_left   = 70;
    plot.margin_top    = 35;
    plot.margin_right  = 25;
    plot.margin_bottom = 50;

    plot.x_axis.grid = 1;
    plot.y_axis.grid = 2;

    {
        const int n = 400;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.05f;
            y[i] = sinf(x[i]) * 1.5f;
        }
        plm_line_style s = { PLM_RED, 0.0f, "sin(x)" };
        plm_plot_add_line(&plot, x, y, n, s);
        free(x); free(y);
    }

    {
        const int n = 30;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.7f;
            y[i] = 0.5f * sinf(x[i] * 0.7f) * cosf(x[i] * 0.3f);
        }
        plm_line_style s = { PLM_BLUE, 0.0f, "modulated" };
        plm_plot_add_line(&plot, x, y, n, s);
        free(x); free(y);
    }

    {
        const int n = 100;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.2f;
            y[i] = 1.0f - (float)i * 0.01f + 0.2f * cosf((float)i * 0.3f);
        }
        plm_line_style s = { PLM_GREEN, 5.0f, "decay" };
        plm_plot_add_line(&plot, x, y, n, s);
        free(x); free(y);
    }

    plm_render(&plot, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {}

    plm_plot_reset(&plot);
    free(pixels);
    return 0;
}
