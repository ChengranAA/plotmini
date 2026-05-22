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

int main(void) {
    const int win_w = 900;
    const int win_h = 680;

    struct mfb_window *win = mfb_open("plotmini3d — interactive viewer", win_w, win_h);
    if (!win) {
        fprintf(stderr, "Failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build surface: sinc·gaussian + ripple ---- */
    int nx = 50, ny = 50;
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

    /* ---- 3D spiral line ---- */
    int nline = 300;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline - 1) * 5.0f * (float)M_PI;
        float r = 3.0f + 1.5f * sinf(t * 0.7f);
        lx[i] = r * cosf(t);
        ly[i] = r * sinf(t);
        lz[i] = t * 0.3f - 1.5f;
    }

    /* ---- View state ---- */
    double azimuth   = -50.0;
    double elevation =  25.0;
    float  light     =  0.8f;

    int needs_redraw = 1;

    while (1) {
        int state = mfb_update(win, fb.pixels);
        if (state < 0) break;  /* window closed */

        /* ---- Handle keyboard ---- */
        const uint8_t *keys = mfb_get_key_buffer(win);
        int changed = 0;

        if (keys[MFB_KB_KEY_ESCAPE]) break;

        if (keys[MFB_KB_KEY_LEFT])  { azimuth   -= 1.5; changed = 1; }
        if (keys[MFB_KB_KEY_RIGHT]) { azimuth   += 1.5; changed = 1; }
        if (keys[MFB_KB_KEY_UP])    { elevation += 1.0; changed = 1; }
        if (keys[MFB_KB_KEY_DOWN])  { elevation -= 1.0; changed = 1; }

        if (keys[MFB_KB_KEY_R]) { azimuth = -50.0; elevation = 25.0; changed = 1; }

        {
            static int l_was_down = 0;
            if (keys[MFB_KB_KEY_L]) {
                if (!l_was_down) {
                    light = (light > 0.5f) ? 0.0f : 0.8f;
                    changed = 1;
                }
                l_was_down = 1;
            } else {
                l_was_down = 0;
            }
        }

        /* Clamp elevation */
        if (elevation >  89.0) elevation =  89.0;
        if (elevation < -89.0) elevation = -89.0;

        if (changed) needs_redraw = 1;

        if (!needs_redraw) {
            mfb_wait_sync(win);
            continue;
        }

        /* ---- Build plot ---- */
        plm3d_plot p;
        plm3d_plot_init(&p);

        {
            char title[128];
            snprintf(title, sizeof(title),
                     "az=%.0f  el=%.0f  light=%.1f  [arrows/R/L/ESC]",
                     azimuth, elevation, (double)light);
            p.title = title;
        }
        p.x_label = "X";
        p.y_label = "Y";
        p.z_label = "Z";

        p.view.azimuth   = azimuth;
        p.view.elevation = elevation;

        /* Surface with colormap + lighting */
        plm3d_plot_add_surface(&p, surf_x, surf_y, surf_z, nx, ny,
            (plm3d_surface_style){PLM_GREY(180), 0.5f, PLM_CMAP_TURBO, light, NULL, 0, 0, 0.0f, 0.0f});

        /* Spiral line */
        plm3d_plot_add_line(&p, lx, ly, lz, nline,
            (plm3d_line_style){PLM_RED, 1.2f, NULL, PLM_CMAP_NONE});

        /* ---- Render ---- */
        plm3d_render(&p, &fb);
        plm_fb_swizzle_rgba_bgra(&fb);

        plm3d_plot_reset(&p);
        needs_redraw = 0;

        mfb_wait_sync(win);
    }

    mfb_close(win);

    free(surf_x); free(surf_y); free(surf_z);
    free(lx); free(ly); free(lz);
    free(pixels);
    return 0;
}
