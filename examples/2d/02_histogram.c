#define PLOTMINI_IMPLEMENTATION
#include "../../plotmini.h"
#include "dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float box_muller(void) {
    static float z1;
    float u1 = (float)rand() / (float)RAND_MAX;
    float u2 = (float)rand() / (float)RAND_MAX;
    if (u1 == 0.0f) u1 = 1e-10f;
    z1 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * (float)M_PI * u2);
    return z1;
}

int main(void) {
    const int win_w = 800;
    const int win_h = 600;

    struct mfb_window *win = mfb_open("plotmini - histogram", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    plm_plot plot;
    plm_plot_init(&plot);
    plot.title   = "Histogram  (Normal Distribution)";
    plot.x_label = "value";
    plot.y_label = "count";

    /* generate 10000 samples from N(0,1) */
    {
        const int n = 10000;
        float *samples = (float *)malloc((size_t)n * sizeof(float));
        for (int i = 0; i < n; i++) samples[i] = box_muller();

        plm_bar_style s = { PLM_BLUE, PLM_BLACK, 1.0f, "N(0,1) samples" };
        plm_plot_add_hist(&plot, samples, n, 40, 0, s);
        free(samples);
    }

    plm_render(&plot, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {}

    plm_plot_reset(&plot);
    free(pixels);
    return 0;
}
