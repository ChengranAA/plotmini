#define PLOTMINI_TEXT_SCALE 2
#define PLOTMINI_DARK_THEME
#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "dep/MiniFB.h"

#define STB_IMAGE_IMPLEMENTATION
#include "dep/stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    int img_w, img_h, ch;
    unsigned char *img = stbi_load("../assets/cat.jpeg", &img_w, &img_h, &ch, 1);
    if (!img) { fprintf(stderr, "failed to load image\n"); return 1; }

    /* Reserve bottom 110 px for the coolwarm horizontal demo */
    const int win_w = 1060, win_h = 720;
    const int coolwarm_h = 110;

    struct mfb_window *win = mfb_open(
        "plotmini — colormaps + colorbar showcase", win_w, win_h);
    if (!win) { stbi_image_free(img); return 1; }

    unsigned char *px = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(px, win_w, win_h, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_BG_COLOR);

    float *fimg = (float *)malloc((size_t)img_w * img_h * sizeof(float));
    int i;
    for (i = 0; i < img_w * img_h; i++) fimg[i] = (float)img[i];
    stbi_image_free(img);

    /* ================================================================ */
    /* 3x3 grid: 9 colormaps with vertical colorbars                    */
    /* ================================================================ */
    {
        plm_cmap cmaps[] = {
            PLM_CMAP_HEAT,    PLM_CMAP_VIRIDIS,  PLM_CMAP_GRAY,
            PLM_CMAP_PLASMA,  PLM_CMAP_INFERNO,  PLM_CMAP_MAGMA,
            PLM_CMAP_TURBO,   PLM_CMAP_CIVIDIS,  PLM_CMAP_JET,
        };
        const char *names[] = {
            "Heat",    "Viridis",  "Gray",
            "Plasma",  "Inferno",  "Magma",
            "Turbo",   "Cividis",  "Jet",
        };

        const int cols = 3, rows = 3;
        const int gap = 16;
        const int top_margin  = 80;
        const int left_margin = 16;
        const int right_margin = 70;

        /* grid fits in the space above the coolwarm demo */
        int grid_h = win_h - coolwarm_h - top_margin;
        int cell_w = (win_w - left_margin - right_margin - (cols - 1) * gap) / cols;
        int cell_h = (grid_h - (rows - 1) * gap) / rows;

        int r, c;
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                int idx = r * cols + c;
                int cx0 = left_margin + c * (cell_w + gap);
                int cy0 = top_margin  + r * (cell_h + gap);

                /* leave room for colorbar + name below */
                int cbar_width = 22;
                int cbar_gap   = 10;
                int img_avail_w = cell_w - cbar_width - cbar_gap - 8;
                int img_avail_h = cell_h - 18;
                if (img_avail_w < 40) img_avail_w = 40;
                if (img_avail_h < 40) img_avail_h = 40;

                plm_irect img_bounds = { cx0, cy0, cx0 + img_avail_w, cy0 + img_avail_h };
                plm_irect img_rect = plm_fit_into(img_bounds, img_w, img_h);

                /* colormapped image */
                plm_imshow(&fb, img_rect, fimg, img_w, img_h,
                           0.0, 255.0, cmaps[idx]);

                /* vertical colorbar alongside */
                int cbar_x0 = img_rect.x1 + cbar_gap;
                plm_irect cbar_rect = { cbar_x0, img_rect.y0,
                                        cbar_x0 + cbar_width, img_rect.y1 };
                plm_colorbar(&fb, cbar_rect, 0.0, 255.0, cmaps[idx],
                             PLM_CBAR_VERTICAL, NULL, 3);

                /* name below */
                int tw = plm_text_width(names[idx]);
                int tx = cx0 + (cell_w - tw) / 2;
                int ty = cy0 + img_avail_h + 2;
                plm_draw_text(&fb, tx, ty + plm_text_height(), names[idx], PLM_FG_COLOR);
            }
        }

        /* title */
        {
            const char *title = "Colormap Gallery  (all 9 maps with colorbars)";
            int tw = plm_text_width(title);
            plm_draw_text(&fb, (win_w - tw) / 2, 24, title, PLM_FG_COLOR);

            const char *sub = "Grayscale image mapped through each colormap";
            tw = plm_text_width(sub);
            plm_draw_text(&fb, (win_w - tw) / 2, 42, sub, PLM_GREY(140));
        }
    }

    /* ================================================================ */
    /* Bottom: coolwarm diverging colormap + horizontal colorbar        */
    /* ================================================================ */
    {
        const int demo_y0 = win_h - coolwarm_h + 16;
        const int demo_x0 = 40;
        const int demo_w = win_w - 80;

        const int n = 400;
        float *wave = (float *)malloc((size_t)n * sizeof(float));
        int i;
        for (i = 0; i < n; i++)
            wave[i] = sinf((float)i / (float)(n - 1) * 4.0f * (float)M_PI);

        int band_h = 30;
        plm_irect band_rect = { demo_x0, demo_y0, demo_x0 + demo_w, demo_y0 + band_h };

        float *band2d = (float *)malloc((size_t)n * (size_t)band_h * sizeof(float));
        int y;
        for (y = 0; y < band_h; y++) {
            for (i = 0; i < n; i++)
                band2d[y * n + i] = wave[i];
        }
        plm_imshow(&fb, band_rect, band2d, n, band_h,
                   -1.0, 1.0, PLM_CMAP_COOLWARM);
        free(band2d);

        /* horizontal colorbar below */
        {
            plm_irect hcbar = { demo_x0, demo_y0 + band_h + 8,
                                demo_x0 + demo_w, demo_y0 + band_h + 8 + 42 };
            plm_colorbar(&fb, hcbar, -1.0, 1.0, PLM_CMAP_COOLWARM,
                         PLM_CBAR_HORIZONTAL, "correlation  (coolwarm)", 5);
        }

        /* label above */
        {
            const char *lbl = "Diverging colormap: coolwarm  --  blue-white-red, zero-centered";
            plm_draw_text(&fb, demo_x0, demo_y0 - 8, lbl, PLM_FG_COLOR);
        }
        free(wave);
    }

    free(fimg);

    plm_fb_swizzle_rgba_bgra(&fb);
    while (mfb_update(win, fb.pixels) >= 0) { /* idle */ }

    free(px);
    return 0;
}
