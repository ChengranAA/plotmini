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
    const int win_w = 800;
    const int win_h = 600;

    struct mfb_window *win = mfb_open("plotmini - basic demo", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    plm_plot plot;
    plm_plot_init(&plot);
    plot.title   = "plotmini + minifb demo";
    plot.x_label = "x";
    plot.y_label = "sin(x)";

    {
        const int n = 200;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.1f;
            y[i] = sinf(x[i]) + 0.2f * ((float)rand() / (float)RAND_MAX - 0.5f);
        }
        plm_line_style s = { PLM_BLUE, 1.0f, "sin(x) + noise", 0, 0};
        plm_plot_add_line(&plot, x, y, n, s);
        free(x);
        free(y);
    }

    {
        const int n = 150;
        float *x = (float *)malloc((size_t)n * sizeof(float));
        float *y = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) {
            x[i] = (float)i * 0.1f;
            y[i] = cosf(x[i]);
        }
        plm_line_style s = { PLM_RED, 1.0f, "cos(x)", 0, 0};
        plm_plot_add_line(&plot, x, y, n, s);
        free(x);
        free(y);
    }

    plm_render(&plot, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {
        /* plm_render(&plot, &fb); plm_fb_swizzle_rgba_bgra(&fb); */
    }

    plm_plot_reset(&plot);
    free(pixels);
    return 0;
}
