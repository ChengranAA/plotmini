#define PLOTMINI_DARK_THEME
#define PLOTMINI_IMPLEMENTATION
#define PLOTMINI3D_IMPLEMENTATION
#include "../../plotmini.h"
#include "../../plotmini3d.h"
#include "../dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------ */
/*  plotmini3d — Dark Showcase                                         */
/*  A single dramatic surface + spiral, side-lit against dark theme.   */
/*  arrows=rotate  P=persp/ortho  G=grid  L=light  S=save  R=reset  ESC=quit  */
/* ------------------------------------------------------------------ */

int main(void) {
    const int win_w = 960;
    const int win_h = 680;

    struct mfb_window *win = mfb_open("plotmini3d — Dark Showcase", win_w, win_h);
    if (!win) { fprintf(stderr, "Failed to open window\n"); return 1; }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build a smooth multi-peak surface ---- */
    int nx = 70, ny = 70;
    float *surf_x = (float *)malloc((size_t)nx * sizeof(float));
    float *surf_y = (float *)malloc((size_t)ny * sizeof(float));
    float *surf_z = (float *)malloc((size_t)nx * ny * sizeof(float));

    for (int i = 0; i < nx; i++) surf_x[i] = -5.0f + i * 10.0f / (nx - 1);
    for (int j = 0; j < ny; j++) surf_y[j] = -5.0f + j * 10.0f / (ny - 1);

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            float x = surf_x[i], y = surf_y[j];
            /* Three overlapping gaussians + a gentle ripple */
            float z  = 3.5f * expf(-0.3f * (x*x + y*y));
            z += 2.0f * expf(-0.8f * ((x-2.5f)*(x-2.5f) + (y+1.5f)*(y+1.5f)));
            z += 2.5f * expf(-0.6f * ((x+2.0f)*(x+2.0f) + (y-2.0f)*(y-2.0f)));
            z += 0.4f * sinf(x * 1.8f) * cosf(y * 1.8f);
            surf_z[i * ny + j] = z;
        }
    }

    /* ---- A single elegant spiral wrapping around ---- */
    int nline = 400;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline - 1) * 6.0f * (float)M_PI;
        float r = 3.8f + 0.9f * sinf(t * 0.55f);
        lx[i] = r * cosf(t);
        ly[i] = r * sinf(t);
        lz[i] = t * 0.28f + 0.5f;
    }

    /* ---- State ---- */
    double azimuth   = -42.0;
    double elevation =  28.0;
    int    persp     = 1;
    int    grid_on   = 1;
    int    light_on  = 1;
    int    needs_redraw = 1;

    while (1) {
        int state = mfb_update(win, fb.pixels);
        if (state < 0) break;

        const uint8_t *keys = mfb_get_key_buffer(win);
        int changed = 0;

        if (keys[MFB_KB_KEY_ESCAPE]) break;
        if (keys[MFB_KB_KEY_LEFT])   { azimuth   -= 1.5; changed = 1; }
        if (keys[MFB_KB_KEY_RIGHT])  { azimuth   += 1.5; changed = 1; }
        if (keys[MFB_KB_KEY_UP])     { elevation += 1.0; changed = 1; }
        if (keys[MFB_KB_KEY_DOWN])   { elevation -= 1.0; changed = 1; }
        if (keys[MFB_KB_KEY_R])      { azimuth = -42.0; elevation = 28.0; changed = 1; }

        { static int w = 0; if (keys[MFB_KB_KEY_P]) { if (!w) { persp    = !persp;    changed = 1; } w = 1; } else w = 0; }
        { static int w = 0; if (keys[MFB_KB_KEY_G]) { if (!w) { grid_on  = !grid_on;  changed = 1; } w = 1; } else w = 0; }
        { static int w = 0; if (keys[MFB_KB_KEY_L]) { if (!w) { light_on = !light_on; changed = 1; } w = 1; } else w = 0; }
        { static int w = 0; if (keys[MFB_KB_KEY_S]) { if (!w) { plm_fb_save_bmp(&fb, "showcase.bmp"); } w = 1; } else w = 0; }

        if (elevation >  89.0) elevation =  89.0;
        if (elevation < -89.0) elevation = -89.0;
        if (changed) needs_redraw = 1;
        if (!needs_redraw) { mfb_wait_sync(win); continue; }

        /* ---- Build plot ---- */
        plm3d_plot p;
        plm3d_plot_init(&p);

        p.title = "plotmini3d";
        p.x_label = "X"; p.y_label = "Y"; p.z_label = "Z";
        p.view.azimuth    = azimuth;
        p.view.elevation  = elevation;
        p.view.distance   = persp ? 3.8 : 0.0;
        p.view.projection = persp ? PLM3D_PERSPECTIVE : PLM3D_ORTHO;
        p.legend_position = PLM_LEGEND_TOP_RIGHT;
        p.margin_right    = 80;  /* room for colorbar */

        p.x_axis.grid = grid_on;
        p.y_axis.grid = grid_on;

        /* Surface: side-lit from upper-left, Turbo colormap */
        plm3d_plot_add_surface(&p, surf_x, surf_y, surf_z, nx, ny,
            (plm3d_surface_style){
                PLM_GREY(180),           /* wireframe line color */
                0.3f,                    /* thin wireframe */
                PLM_CMAP_TURBO,          /* colormap */
                light_on ? 0.85f : 0.0f, /* lighting intensity */
                NULL,                    /* no legend */
                0,                       /* don't cull backfaces */
                0,                       /* filled + wireframe */
                135.0f, 55.0f            /* light from upper-left */
            });

        /* Spiral: color-by-height with Plasma */
        plm3d_plot_add_line(&p, lx, ly, lz, nline,
            (plm3d_line_style){PLM_RED, 1.2f, NULL, PLM_CMAP_PLASMA});

        /* ---- Render ---- */
        plm3d_render(&p, &fb);

        /* Colorbar on the right margin */
        plm3d_colorbar(&p, &fb, PLM_CMAP_TURBO,
                       p.z_axis.min, p.z_axis.max, "Z");

        /* Status line in bottom-left corner */
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "az:%.0f el:%.0f %s %s [S:save BMP]",
                                 azimuth, elevation,
                                 persp ? "persp" : "ortho",
                                 light_on ? "lit" : "flat");
            plm_draw_text(&fb, 8, win_h - 8, buf,
                          (plm_color){128, 128, 128, 255});
        }

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
