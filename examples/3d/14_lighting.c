#define PLOTMINI_IMPLEMENTATION
#define PLOTMINI3D_IMPLEMENTATION
#include "../../plotmini.h"
#include "../../plotmini3d.h"
#include "../dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------ */
/*  Lighting comparison grid — now uses plm3d_figure for subplots     */
/*  Renders the same surface at 3 lighting levels × 2 colormaps       */
/* ------------------------------------------------------------------ */

int main(void) {
    int cols = 3;   /* light = 0.0, 0.5, 1.0 */
    int rows = 2;   /* viridis, turbo           */

    /* Window size: let the figure divide the space */
    int win_w = 1020;
    int win_h = 560;

    struct mfb_window *win = mfb_open("plotmini3d — surface lighting comparison", win_w, win_h);
    if (!win) {
        fprintf(stderr, "Failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)calloc((size_t)win_w * win_h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build the surface data once ---- */
    plm_cmap cmaps[2]         = { PLM_CMAP_VIRIDIS, PLM_CMAP_TURBO };
    const char *cmap_names[2]   = { "Viridis", "Turbo" };
    float lights[3]             = { 0.0f, 0.5f, 1.0f };
    const char *light_labels[3] = { "Flat  (light = 0.0)", "Half  (light = 0.5)", "Full  (light = 1.0)" };

    int nx = 40, ny = 40;
    float *surf_x = (float *)malloc((size_t)nx * sizeof(float));
    float *surf_y = (float *)malloc((size_t)ny * sizeof(float));
    float *surf_z = (float *)malloc((size_t)nx * ny * sizeof(float));

    for (int i = 0; i < nx; i++) surf_x[i] = -4.0f + i * 8.0f / (nx - 1);
    for (int j = 0; j < ny; j++) surf_y[j] = -4.0f + j * 8.0f / (ny - 1);

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            float x = surf_x[i], y = surf_y[j];
            float r = sqrtf(x * x + y * y);
            float sinc = (r < 1e-4f) ? 1.0f : sinf(r * 2.5f) / (r * 2.5f);
            float gauss = expf(-0.3f * (x * x + y * y));
            float ripple = 0.15f * sinf(x * 2.0f) * cosf(y * 2.0f);
            surf_z[i * ny + j] = 2.5f * sinc * gauss + ripple;
        }
    }

    /* ---- Build figure with plm3d_figure ---- */
    plm3d_figure fig;
    plm3d_figure_init(&fig, rows, cols);
    fig.title = "Surface Lighting Comparison  —  Viridis vs Turbo  ×  Flat / Half / Full";

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            plm3d_plot *p = plm3d_figure_plot_3d(&fig, row, col);

            char title[64];
            snprintf(title, sizeof(title), "%s  |  %s",
                     cmap_names[row], light_labels[col]);
            p->title         = title;
            p->view.azimuth   = -50.0;
            p->view.elevation =  25.0;

            plm3d_plot_add_surface(p, surf_x, surf_y, surf_z, nx, ny,
                (plm3d_surface_style){PLM_GREY(150), 0.35f, cmaps[row], lights[col], NULL, 0});
        }
    }

    /* ---- Render ---- */
    plm3d_figure_render(&fig, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    /* ---- Display ---- */
    while (1) {
        int state = mfb_update(win, fb.pixels);
        if (state < 0) break;
        if (mfb_get_key_buffer(win)[MFB_KB_KEY_ESCAPE]) break;
        mfb_wait_sync(win);
    }

    mfb_close(win);

    free(surf_x); free(surf_y); free(surf_z);
    free(pixels);
    plm3d_figure_reset(&fig);
    return 0;
}
