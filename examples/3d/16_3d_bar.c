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
/*  3D Bar Chart — Cityscape / Bivariate Histogram                     */
/*  Stress-tests: bars, colormap, stroke, grid, legend, perspective    */
/* ------------------------------------------------------------------ */

int main(void) {
    const int win_w = 960;
    const int win_h = 640;

    struct mfb_window *win = mfb_open("plotmini3d — 3D Bar Chart", win_w, win_h);
    if (!win) {
        fprintf(stderr, "Failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build a bivariate normal distribution as bar heights ---- */
    int nx = 15, ny = 15;
    int nbars = nx * ny;
    float *bx = (float *)malloc((size_t)nbars * sizeof(float));
    float *by = (float *)malloc((size_t)nbars * sizeof(float));
    float *bz = (float *)malloc((size_t)nbars * sizeof(float));

    float cx = 0.0f, cy = 0.0f;   /* distribution centre */
    float sx = 1.5f, sy = 1.5f;   /* spread */
    float rho = 0.3f;             /* correlation */

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int idx = i * ny + j;
            float x = -3.0f + i * 6.0f / (nx - 1);
            float y = -3.0f + j * 6.0f / (ny - 1);
            bx[idx] = x;
            by[idx] = y;

            /* Bivariate normal PDF */
            float dx = (x - cx) / sx;
            float dy = (y - cy) / sy;
            float zval = expf(-0.5f / (1.0f - rho * rho) *
                              (dx * dx - 2.0f * rho * dx * dy + dy * dy));
            zval /= 2.0f * (float)M_PI * sx * sy * sqrtf(1.0f - rho * rho);
            /* Scale up for visibility */
            bz[idx] = zval * 60.0f;
        }
    }

    /* ---- Interactive view state ---- */
    double azimuth   = -45.0;
    double elevation =  30.0;
    int    grid_on   = 1;
    int    needs_redraw = 1;

    while (1) {
        int state = mfb_update(win, fb.pixels);
        if (state < 0) break;

        const uint8_t *keys = mfb_get_key_buffer(win);
        int changed = 0;

        if (keys[MFB_KB_KEY_ESCAPE]) break;

        if (keys[MFB_KB_KEY_LEFT])  { azimuth   -= 2.0; changed = 1; }
        if (keys[MFB_KB_KEY_RIGHT]) { azimuth   += 2.0; changed = 1; }
        if (keys[MFB_KB_KEY_UP])    { elevation += 1.5; changed = 1; }
        if (keys[MFB_KB_KEY_DOWN])  { elevation -= 1.5; changed = 1; }
        if (keys[MFB_KB_KEY_R])     { azimuth = -45.0; elevation = 30.0; changed = 1; }

        {
            static int g_was_down = 0;
            if (keys[MFB_KB_KEY_G]) {
                if (!g_was_down) { grid_on = !grid_on; changed = 1; }
                g_was_down = 1;
            } else { g_was_down = 0; }
        }

        if (elevation >  89.0) elevation =  89.0;
        if (elevation < -89.0) elevation = -89.0;

        if (changed) needs_redraw = 1;
        if (!needs_redraw) { mfb_wait_sync(win); continue; }

        /* ---- Build plot ---- */
        plm3d_plot p;
        plm3d_plot_init(&p);

        {
            char title[128];
            snprintf(title, sizeof(title),
                     "3D Bar Chart — az=%.0f el=%.0f [arrows/R/G/ESC]",
                     azimuth, elevation);
            p.title = title;
        }
        p.x_label = "X";
        p.y_label = "Y";
        p.z_label = "Height";

        p.view.azimuth   = azimuth;
        p.view.elevation = elevation;
        p.view.distance  = 4.0;    /* mild perspective */
        p.view.projection = PLM3D_PERSPECTIVE;

        p.legend_position = PLM_LEGEND_TOP_RIGHT;

        /* Enable floor grid */
        p.x_axis.grid = grid_on;
        p.y_axis.grid = grid_on;

        /* Add bars: colormapped by height, with stroke */
        plm3d_plot_add_bar(&p, bx, by, bz, nbars, 0.18f, 0.18f,
            (plm3d_bar_style){
                PLM_GREY(180),          /* fill_color (ignored when cmap is set) */
                PLM_GREY(40),           /* stroke_color */
                0.8f,                   /* stroke_width */
                PLM_CMAP_TURBO,         /* cmap */
                "bivariate normal",     /* legend */
                NAN                     /* baseline */
            });

        /* ---- Render ---- */
        plm3d_render(&p, &fb);
        plm_fb_swizzle_rgba_bgra(&fb);

        plm3d_plot_reset(&p);
        needs_redraw = 0;

        mfb_wait_sync(win);
    }

    mfb_close(win);

    free(bx); free(by); free(bz);
    free(pixels);
    return 0;
}
