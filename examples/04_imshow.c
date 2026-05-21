#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "dep/MiniFB.h"

#define STB_IMAGE_IMPLEMENTATION
#include "dep/stb_image.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int img_w, img_h, ch;
    unsigned char *img = stbi_load("assets/cat.jpeg", &img_w, &img_h, &ch, 1);
    if (!img) { fprintf(stderr, "failed to load image\n"); return 1; }

    const int win_w = 800, win_h = 600;
    struct mfb_window *win = mfb_open("plotmini - imshow", win_w, win_h);
    if (!win) { stbi_image_free(img); return 1; }

    unsigned char *px = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(px, win_w, win_h, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_BLACK);

    plm_plot plot;
    plm_plot_init(&plot);
    plot.title = "imshow  (viridis)";

    plm_irect bounds = {
        plot.margin_left,
        plot.margin_top,
        fb.width  - plot.margin_right,
        fb.height - plot.margin_bottom
    };
    plm_irect rect = plm_fit_into(bounds, img_w, img_h);

    float *fimg = (float *)malloc((size_t)img_w * img_h * sizeof(float));
    for (int i = 0; i < img_w * img_h; i++) fimg[i] = (float)img[i];
    stbi_image_free(img);

    plm_imshow(&fb, rect, fimg, img_w, img_h, 0.0, 255.0, PLM_CMAP_VIRIDIS);
    free(fimg);

    if (plot.title) {
        int tw = plm_text_width(plot.title);
        plm_draw_text(&fb, (fb.width - tw) / 2, 20, plot.title, PLM_WHITE);
    }

    plm_fb_swizzle_rgba_bgra(&fb);
    while (mfb_update(win, fb.pixels) >= 0) {}
    plm_plot_reset(&plot);
    free(px);
    return 0;
}
