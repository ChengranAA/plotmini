#define PLOTMINI_TEXT_SCALE 2
#define PLOTMINI_DARK_THEME
#define PLOTMINI_IMPLEMENTATION
#include "../../plotmini.h"
#include "../dep/MiniFB.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const int win_w = 800;
    const int win_h = 600;

    struct mfb_window *win = mfb_open("plotmini - categorical bar demo", win_w, win_h);
    if (!win) {
        fprintf(stderr, "failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- simple categorical bar chart ---- */
    {
        plm_plot plot;
        plm_plot_init(&plot);
        plot.title   = "Categorical Bar Chart";
        plot.x_label = "Fruit";
        plot.y_label = "Quantity";

        /* set up categorical x-axis */
        plot.x_axis.scale = PLM_CATEGORICAL;
        plot.x_axis.num_categories = 5;
        static const char *categories[] = {"Apple", "Banana", "Cherry", "Date", "Fig"};
        plot.x_axis.categories = categories;
        /* turn off grid for cleaner look */
        plot.x_axis.grid = 0;

        /* bar data: category indices and heights */
        const int n = 5;
        float x[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
        float y[] = {25.0f, 40.0f, 15.0f, 30.0f, 10.0f};

        plm_bar_style bs = { PLM_RGBA(70, 130, 180, 255), PLM_BLACK, 1.0f, NULL };
        plm_plot_add_bar(&plot, x, y, n, 0.7f, bs);

        plm_render(&plot, &fb);
        plm_plot_reset(&plot);
    }

    plm_fb_swizzle_rgba_bgra(&fb);

    while (mfb_update(win, fb.pixels) >= 0) {
        /* static */
    }

    free(pixels);
    return 0;
}
