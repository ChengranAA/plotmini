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
    if (iter == max_iter) return (float)max_iter;
    double log_zn = log(x2 + y2) * 0.5;
    double nu = log(log_zn / log(2.0)) / log(2.0);
    return (float)((double)iter + 1.0 - nu);
}

int main(void) {
    const int win_w = 900;
    const int win_h = 680;

    struct mfb_window *win = mfb_open("plotmini - Animated Mandelbrot Zoom", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *fb_pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    if (!fb_pixels) {
        fprintf(stderr, "failed to allocate framebuffer\n");
        return 1;
    }

    /* data array at 1:1 framebuffer resolution — reused each frame */
    const int data_w = win_w;
    const int data_h = win_h;
    float *mb_data = (float *)malloc((size_t)data_w * data_h * sizeof(float));
    if (!mb_data) {
        fprintf(stderr, "failed to allocate mandelbrot data\n");
        free(fb_pixels);
        return 1;
    }

    /* -------- zoom target: Misiurewicz point on the boundary -------- */
    /* We zoom directly into a boundary point so the view never goes
       fully inside the set.  The base range is used only for the
       initial frame; after that we derive the viewport from the
       target center and the current zoom factor. */
    const double base_w = 3.1;   /* -2.1 .. 1.0 */
    const double base_h = 2.4;   /* -1.2 .. 1.2 */

    /* A well-known boundary point in the seahorse valley.
       Staying exactly on the boundary guarantees detail at every zoom. */
    const double target_cx = -0.743643887037151;
    const double target_cy =  0.131825904205330;

    int    frame       = 0;
    int    max_iter    = 256;
    double zoom_factor = 1.0;

    /* current viewport (updated each frame) */
    double cx0 = -2.1, cy0 = -1.2;
    double cx1 =  1.0, cy1 =  1.2;
    double center_x = target_cx;
    double center_y = target_cy;

    while (mfb_update(win, fb_pixels) >= 0) {
        frame++;

        /* ---- update zoom: slow exponential zoom ---- */
        zoom_factor *= 1.012;  /* ~2x every 58 frames */
        if (zoom_factor > 200000.0) {
            /* reset the zoom cycle */
            zoom_factor = 1.0;
            max_iter    = 256;
        }

        /* increase max iterations as we zoom deeper */
        max_iter = (int)(256.0 + log(zoom_factor) / log(2.0) * 40.0);
        if (max_iter > 2048) max_iter = 2048;
        if (max_iter < 256)  max_iter = 256;

        /* derive viewport from the fixed target center + zoom */
        {
            double range_x = base_w / zoom_factor;
            double range_y = base_h / zoom_factor;

            cx0 = center_x - range_x * 0.5;
            cx1 = center_x + range_x * 0.5;
            cy0 = center_y - range_y * 0.5;
            cy1 = center_y + range_y * 0.5;
        }

        /* ---- compute Mandelbrot at full resolution ---- */
        {
            double inv_w = (cx1 - cx0) / (double)(data_w - 1);
            double inv_h = (cy1 - cy0) / (double)(data_h - 1);
            for (int py = 0; py < data_h; py++) {
                double cy = cy0 + inv_h * (double)py;
                float *row = mb_data + (size_t)py * data_w;
                for (int px = 0; px < data_w; px++) {
                    row[px] = mandelbrot(cx0 + inv_w * (double)px, cy, max_iter);
                }
            }
        }

        /* ---- clear framebuffer and render fractal via imshow ---- */
        {
            plm_fb  fb   = plm_fb_create(fb_pixels, win_w, win_h, PLM_RGBA8);
            plm_irect rect = { 0, 0, fb.width, fb.height };
            plm_fb_clear(&fb, PLM_BLACK);
            plm_imshow(&fb, rect, mb_data, data_w, data_h,
                       0.0, (double)max_iter, PLM_CMAP_VIRIDIS);
        }

        /* ---- overlay: histogram of iteration counts ---- */
        {
            /* build a smaller sample grid for the histogram */
            const int sample_w = 200, sample_h = 150;
            float *samples = (float *)malloc((size_t)sample_w * sample_h * sizeof(float));
            if (samples) {
                double inv_sw = (cx1 - cx0) / (double)(sample_w - 1);
                double inv_sh = (cy1 - cy0) / (double)(sample_h - 1);
                for (int py = 0; py < sample_h; py++) {
                    double cy = cy0 + inv_sh * (double)py;
                    float *row_s = samples + (size_t)py * sample_w;
                    for (int px = 0; px < sample_w; px++) {
                        row_s[px] = mandelbrot(cx0 + inv_sw * (double)px, cy, max_iter);
                    }
                }

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

                {
                    plm_fb fb = plm_fb_create(fb_pixels, win_w, win_h, PLM_RGBA8);
                    int inset_w = 280, inset_h = 200;
                    int pad = 20;
                    int cell_x0 = fb.width  - inset_w - pad;
                    int cell_y0 = pad;
                    int cell_x1 = cell_x0 + inset_w;
                    int cell_y1 = cell_y0 + inset_h;

                    plm_irect bg = { cell_x0 - 4, cell_y0 - 4,
                                     cell_x1 + 4, cell_y1 + 4 };
                    plm_fb_fill_rect(&fb, bg, PLM_RGBA(10, 10, 10, 220));
                    plm__render_plot_into(&inset, &fb, cell_x0, cell_y0, cell_x1, cell_y1);
                }

                plm_plot_reset(&inset);
                free(samples);
            }
        }

        /* ---- overlay: info text (frame, zoom, max_iter) ---- */
        {
            plm_fb fb = plm_fb_create(fb_pixels, win_w, win_h, PLM_RGBA8);
            char buf[128];
            snprintf(buf, sizeof(buf), "frame: %d  |  zoom: %.0fx  |  max_iter: %d  |  res: %dx%d",
                     frame, zoom_factor, max_iter, data_w, data_h);
            int tw = plm_text_width(buf);
            int th = plm_text_height();
            int tx = (fb.width - tw) / 2;
            int ty = fb.height - th - 8;
            /* dark background bar for readability */
            plm_irect bg = { tx - 6, ty - 2, tx + tw + 6, ty + th + 2 };
            plm_fb_fill_rect(&fb, bg, PLM_RGBA(10, 10, 10, 200));
            plm_draw_text(&fb, tx, ty, buf, PLM_RGBA(220, 220, 220, 255));
        }

        /* ---- swizzle and display ---- */
        {
            plm_fb fb = plm_fb_create(fb_pixels, win_w, win_h, PLM_RGBA8);
            plm_fb_swizzle_rgba_bgra(&fb);
        }

        /* throttle to ~30 fps so the compute is the bottleneck, not vsync */
        {
            struct mfb_timer *t = mfb_timer_create();
            while (mfb_timer_now(t) < 0.033) { /* ~33 ms */ }
            mfb_timer_destroy(t);
        }

        if (frame % 30 == 0) {
            printf("frame %d  zoom %.1fx  max_iter %d  range [%.14f, %.14f] x [%.14f, %.14f]\n",
                   frame, zoom_factor, max_iter, cx0, cx1, cy0, cy1);
        }
    }

    free(mb_data);
    free(fb_pixels);
    return 0;
}
