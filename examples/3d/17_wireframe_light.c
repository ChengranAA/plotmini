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
/*  3D Wireframe + Custom Light Direction + Grid                       */
/*  Stress-tests: wireframe_only, light_azimuth/elevation, axis.grid   */
/* ------------------------------------------------------------------ */

int main(void) {
    const int win_w = 960;
    const int win_h = 640;

    struct mfb_window *win = mfb_open("plotmini3d — Wireframe + Custom Light", win_w, win_h);
    if (!win) {
        fprintf(stderr, "Failed to open window\n");
        return 1;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build a surface: sombrero (Mexican hat) ---- */
    int nx = 55, ny = 55;
    float *surf_x = (float *)malloc((size_t)nx * sizeof(float));
    float *surf_y = (float *)malloc((size_t)ny * sizeof(float));
    float *surf_z = (float *)malloc((size_t)nx * ny * sizeof(float));

    for (int i = 0; i < nx; i++) surf_x[i] = -8.0f + i * 16.0f / (nx - 1);
    for (int j = 0; j < ny; j++) surf_y[j] = -8.0f + j * 16.0f / (ny - 1);

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            float x = surf_x[i], y = surf_y[j];
            float r = sqrtf(x * x + y * y);
            /* Sombrero function: sin(r)/r with a ripple */
            float z;
            if (r < 1e-4f) z = 1.0f;
            else z = sinf(r) / r;
            z += 0.15f * cosf(x * 1.5f) * sinf(y * 1.5f);
            surf_z[i * ny + j] = z * 4.0f;
        }
    }

    /* ---- Interactive state ---- */
    double azimuth   = -45.0;
    double elevation =  28.0;
    double light_az  =  60.0;   /* light from upper-right */
    double light_el  =  40.0;
    int    wireframe = 0;       /* toggle between wireframe and filled */
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
        if (keys[MFB_KB_KEY_R])     { azimuth = -45.0; elevation = 28.0; changed = 1; }

        {
            static int w_was_down = 0;
            if (keys[MFB_KB_KEY_W]) {
                if (!w_was_down) { wireframe = !wireframe; changed = 1; }
                w_was_down = 1;
            } else { w_was_down = 0; }
        }
        {
            static int g_was_down = 0;
            if (keys[MFB_KB_KEY_G]) {
                if (!g_was_down) { grid_on = !grid_on; changed = 1; }
                g_was_down = 1;
            } else { g_was_down = 0; }
        }
        {
            static int l_was_down = 0;
            if (keys[MFB_KB_KEY_L]) {
                if (!l_was_down) {
                    /* Cycle light direction */
                    if (light_az == 0.0 && light_el == 0.0) { light_az = 60.0; light_el = 40.0; }
                    else if (light_az < 100.0) { light_az = 0.0; light_el = 0.0; }
                    else { light_az = 60.0; light_el = 40.0; }
                    changed = 1;
                }
                l_was_down = 1;
            } else { l_was_down = 0; }
        }

        if (elevation >  89.0) elevation =  89.0;
        if (elevation < -89.0) elevation = -89.0;

        if (changed) needs_redraw = 1;
        if (!needs_redraw) { mfb_wait_sync(win); continue; }

        /* ---- Build plot ---- */
        plm3d_plot p;
        plm3d_plot_init(&p);

        {
            const char *light_label = (light_az == 0.0 && light_el == 0.0) ? "camera" : "upper-right";
            const char *mode_label = wireframe ? "WIREFRAME" : "FILLED";
            char title[256];
            snprintf(title, sizeof(title),
                     "3D Surface — %s | light=%s | az=%.0f el=%.0f [arrows/W/L/G/R/ESC]",
                     mode_label, light_label, azimuth, elevation);
            p.title = title;
        }
        p.x_label = "X";
        p.y_label = "Y";
        p.z_label = "Z";

        p.view.azimuth   = azimuth;
        p.view.elevation = elevation;
        p.view.distance  = 3.5;
        p.view.projection = PLM3D_PERSPECTIVE;

        p.legend_position = PLM_LEGEND_TOP_RIGHT;

        /* Enable grid on all three axes */
        p.x_axis.grid = grid_on;
        p.y_axis.grid = grid_on;
        p.z_axis.grid = grid_on;

        /* Add surface with wireframe option and custom light */
        plm3d_plot_add_surface(&p, surf_x, surf_y, surf_z, nx, ny,
            (plm3d_surface_style){
                PLM_GREY(140),          /* line_color */
                0.6f,                   /* line_width */
                PLM_CMAP_VIRIDIS,       /* cmap */
                0.85f,                  /* light intensity */
                "sombrero",             /* legend */
                0,                      /* cull_backfaces */
                wireframe,              /* wireframe_only */
                (float)light_az,        /* light_azimuth (0,0 = camera) */
                (float)light_el,        /* light_elevation */
            });

        /* ---- Render ---- */
        plm3d_render(&p, &fb);
        plm_fb_swizzle_rgba_bgra(&fb);

        plm3d_plot_reset(&p);
        needs_redraw = 0;

        mfb_wait_sync(win);
    }

    mfb_close(win);

    free(surf_x); free(surf_y); free(surf_z);
    free(pixels);
    return 0;
}
