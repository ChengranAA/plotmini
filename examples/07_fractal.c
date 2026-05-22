#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Mandelbrot iteration.
   Returns smoothed iteration count (fractional), or 0 for points that escape. */
static float mandelbrot(double cx, double cy, int max_iter) {
    double x = 0.0, y = 0.0;
    double x2 = 0.0, y2 = 0.0;
    int iter;
    for (iter = 0; iter < max_iter; iter++) {
        y = 2.0 * x * y + cy;
        x = x2 - y2 + cx;
        x2 = x * x;
        y2 = y * y;
        if (x2 + y2 > 4.0) break;
    }
    if (iter == max_iter) return (float)max_iter;  /* inside set */
    /* smooth escape: log2(log2(|z|)) */
    double log_zn = log(x2 + y2) * 0.5;
    double nu = log(log_zn / log(2.0)) / log(2.0);
    return (float)((double)iter + 1.0 - nu);
}

int main(void) {
    const int win_w = 900;
    const int win_h = 680;

    struct mfb_window *win = mfb_open("plotmini - Mandelbrot fractal", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *fb_pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(fb_pixels, win_w, win_h, PLM_RGBA8);

    /* ---- 1. Mandelbrot set via imshow ---- */

    /* data array at 1:1 framebuffer resolution */
    const int data_w = win_w;
    const int data_h = win_h;
    float *mb_data = (float *)malloc((size_t)data_w * data_h * sizeof(float));

    /* viewport in complex plane */
    const double cx0 = -2.1, cy0 = -1.2;
    const double cx1 =  1.0, cy1 =  1.2;
    const int    max_iter = 256;

    printf("Computing Mandelbrot set (%d x %d, max %d iterations)...\n",
           data_w, data_h, max_iter);

    for (int py = 0; py < data_h; py++) {
        double cy = cy0 + (cy1 - cy0) * (double)py / (double)(data_h - 1);
        for (int px = 0; px < data_w; px++) {
            double cx = cx0 + (cx1 - cx0) * (double)px / (double)(data_w - 1);
            mb_data[py * data_w + px] = mandelbrot(cx, cy, max_iter);
        }
    }

    printf("Rendering...\n");

    /* clear to black, then imshow the fractal */
    plm_fb_clear(&fb, PLM_BLACK);
    {
        plm_irect rect = { 0, 0, fb.width, fb.height };
        plm_imshow(&fb, rect, mb_data, data_w, data_h,
                   0.0, (double)max_iter, PLM_CMAP_VIRIDIS);
    }
    free(mb_data);

    /* ---- 2. Overlay a small histogram of iteration counts ---- */

    /* re-sample a smaller grid to build a histogram cheaply */
    const int sample_w = 200, sample_h = 150;
    float *samples = (float *)malloc((size_t)sample_w * sample_h * sizeof(float));

    for (int py = 0; py < sample_h; py++) {
        double cy = cy0 + (cy1 - cy0) * (double)py / (double)(sample_h - 1);
        for (int px = 0; px < sample_w; px++) {
            double cx = cx0 + (cx1 - cx0) * (double)px / (double)(sample_w - 1);
            samples[py * sample_w + px] = mandelbrot(cx, cy, max_iter);
        }
    }

    /* build an inset plot in the top-right corner */
    plm_plot inset;
    plm_plot_init(&inset);
    inset.title           = "Iteration histogram";
    inset.x_label         = "iters";
    inset.y_label         = "freq";
    inset.margin_left     = 35;
    inset.margin_top      = 16;
    inset.margin_right    = 8;
    inset.margin_bottom   = 22;
    inset.x_axis.grid     = 0;
    inset.y_axis.grid     = 0;
    inset.x_axis.tick_hint = 3;
    inset.y_axis.tick_hint = 3;
    inset.x_axis.min      = 0.0;
    inset.x_axis.max      = (double)max_iter;

    plm_bar_style bs = { PLM_RGBA(253, 231, 37, 255), PLM_BLACK, 0.5f, NULL };
    plm_plot_add_hist(&inset, samples, sample_w * sample_h, 40, 0, bs);

    /* Position inset: top-right of framebuffer, 280x200 px */
    {
        int inset_w = 280, inset_h = 200;
        int pad = 20;
        int cell_x0 = fb.width  - inset_w - pad;
        int cell_y0 = pad;
        int cell_x1 = cell_x0 + inset_w;
        int cell_y1 = cell_y0 + inset_h;

        /* dark background behind the inset */
        plm_irect bg = { cell_x0 - 4, cell_y0 - 4,
                         cell_x1 + 4, cell_y1 + 4 };
        plm_fb_fill_rect(&fb, bg, PLM_RGBA(10, 10, 10, 220));

        /* render the plot into the inset cell */
        plm__render_plot_into(&inset, &fb, cell_x0, cell_y0, cell_x1, cell_y1);
    }

    free(samples);

    /* ---- display ---- */

    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {
        /* static */
    }

    free(fb_pixels);
    return 0;
}
