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

    struct mfb_window *win = mfb_open("plotmini - animated demo", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* pre-allocate data arrays */
    const int n = 200;
    float *x = (float *)malloc((size_t)n * sizeof(float));
    float *y = (float *)malloc((size_t)n * sizeof(float));
    for (int i = 0; i < n; i++) {
        x[i] = (float)i * 0.1f;
    }

    plm_line_style style_blue = { PLM_BLUE,   1.5f, "sin(x + t)", 0, 0};
    plm_line_style style_red  = { PLM_RED,    1.0f, "cos(x + t)", 0, 0};
    plm_line_style style_grey = { PLM_GREY(180), 0.5f, NULL, 0, 0};

    float phase = 0.0f;
    int   frame = 0;

    while (mfb_update(win, fb.pixels) >= 0) {
        phase += 0.04f;
        frame++;

        /* update wave data */
        for (int i = 0; i < n; i++) {
            y[i] = sinf(x[i] + phase);
        }

        /* ---- rebuild plot each frame ---- */
        plm_plot plot;
        plm_plot_init(&plot);
        plot.y_axis.min = -1.2;      /* fixed range so axis doesn't jitter */
        plot.y_axis.max =  1.2;
        {
            static char title_buf[64];
            snprintf(title_buf, sizeof(title_buf), "Animated sine & cosine  (frame %d)", frame);
            plot.title = title_buf;
        }
        plot.x_label = "x";
        plot.y_label = "y";

        /* moving sine */
        plm_plot_add_line(&plot, x, y, n, style_blue);

        /* moving cosine */
        for (int i = 0; i < n; i++) {
            y[i] = cosf(x[i] + phase);
        }
        plm_plot_add_line(&plot, x, y, n, style_red);

        /* static reference line at y=0 */
        {
            float zx[2] = { x[0], x[n - 1] };
            float zy[2] = { 0.0f, 0.0f };
            plm_plot_add_line(&plot, zx, zy, 2, style_grey);
        }

        /* render */
        plm_render(&plot, &fb);
        plm_fb_swizzle_rgba_bgra(&fb);

        plm_plot_reset(&plot);

        /* throttle roughly to ~60 fps */
        struct mfb_timer *t = mfb_timer_create();
        while (mfb_timer_now(t) < 0.016) { /* 16 ms */ }
        mfb_timer_destroy(t);
    }

    free(x);
    free(y);
    free(pixels);
    return 0;
}
