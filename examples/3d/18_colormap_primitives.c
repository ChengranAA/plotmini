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
/*  Colormap Primitives — line cmap, scatter cmap, stems, colorbar     */
/*  Interactive: arrows=rotate, R=reset, 1/2/3=toggle series, C=cbar  */
/* ------------------------------------------------------------------ */

int main(void) {
    const int win_w = 960;
    const int win_h = 640;

    struct mfb_window *win = mfb_open("plotmini3d — Colormap Primitives", win_w, win_h);
    if (!win) { fprintf(stderr, "Failed to open window\n"); return 1; }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build datasets ---- */
    /* Spiral line (Z increases along path -> cmap gradient) */
    int nline = 300;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline - 1) * 6.0f * (float)M_PI;
        float r = 2.5f + 1.0f * sinf(t * 0.5f);
        lx[i] = r * cosf(t);
        ly[i] = r * sinf(t);
        lz[i] = t * 0.4f;
    }

    /* Scatter cloud with custom color data */
    int nscat = 200;
    float *sx = (float *)malloc((size_t)nscat * sizeof(float));
    float *sy = (float *)malloc((size_t)nscat * sizeof(float));
    float *sz = (float *)malloc((size_t)nscat * sizeof(float));
    float *cd = (float *)malloc((size_t)nscat * sizeof(float));
    for (int i = 0; i < nscat; i++) {
        float a = (float)i / nscat * 5.0f * (float)M_PI;
        sx[i] = cosf(a * 1.3f) * (2.5f + 1.5f * sinf(a * 0.7f));
        sy[i] = sinf(a * 0.8f) * (2.5f + 1.5f * cosf(a * 0.6f));
        sz[i] = a * 0.3f;
        cd[i] = sinf(a * 2.0f);  /* oscillating — not monotonic with Z */
    }

    /* Stem data */
    int nstem = 60;
    float *tx = (float *)malloc((size_t)nstem * sizeof(float));
    float *ty = (float *)malloc((size_t)nstem * sizeof(float));
    float *tz = (float *)malloc((size_t)nstem * sizeof(float));
    for (int i = 0; i < nstem; i++) {
        float t = (float)i / (nstem - 1) * 4.0f * (float)M_PI;
        tx[i] = cosf(t * 1.7f) * 3.0f;
        ty[i] = sinf(t * 1.1f) * 3.0f;
        tz[i] = 0.5f + fabsf(cosf(t * 2.5f)) * 3.5f;
    }

    /* ---- State ---- */
    double azimuth   = -45.0;
    double elevation =  25.0;
    int    show_line    = 1;
    int    show_scatter = 1;
    int    show_stems   = 1;
    int    show_cbar    = 0;
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
        if (keys[MFB_KB_KEY_R])     { azimuth = -45.0; elevation = 25.0; changed = 1; }

        { static int w=0; if (keys[MFB_KB_KEY_1]) { if(!w){show_line=!show_line;changed=1;} w=1; } else w=0; }
        { static int w=0; if (keys[MFB_KB_KEY_2]) { if(!w){show_scatter=!show_scatter;changed=1;} w=1; } else w=0; }
        { static int w=0; if (keys[MFB_KB_KEY_3]) { if(!w){show_stems=!show_stems;changed=1;} w=1; } else w=0; }
        { static int w=0; if (keys[MFB_KB_KEY_C]) { if(!w){show_cbar=!show_cbar;changed=1;} w=1; } else w=0; }

        if (elevation >  89.0) elevation =  89.0;
        if (elevation < -89.0) elevation = -89.0;
        if (changed) needs_redraw = 1;
        if (!needs_redraw) { mfb_wait_sync(win); continue; }

        /* ---- Build plot ---- */
        plm3d_plot p;
        plm3d_plot_init(&p);

        {
            char title[256];
            snprintf(title, sizeof(title),
                     "Colormap Primitives  az=%.0f el=%.0f  [1:line 2:scat 3:stem C:cbar R:reset ESC]",
                     azimuth, elevation);
            p.title = title;
        }
        p.x_label = "X"; p.y_label = "Y"; p.z_label = "Z";
        p.view.azimuth   = azimuth;
        p.view.elevation = elevation;
        p.view.distance  = 3.5;
        p.view.projection = PLM3D_PERSPECTIVE;
        p.legend_position = PLM_LEGEND_TOP_RIGHT;
        if (show_cbar) p.margin_right = 80;

        /* Line with cmap */
        if (show_line)
            plm3d_plot_add_line(&p, lx, ly, lz, nline,
                (plm3d_line_style){PLM_RED, 1.5f, "spiral (Turbo)", PLM_CMAP_TURBO});

        /* Scatter with cmap + custom color data */
        if (show_scatter)
            plm3d_plot_add_scatter(&p, sx, sy, sz, nscat,
                (plm3d_scatter_style){PLM_WHITE, 3.0f, "cloud (Plasma)", PLM_CMAP_PLASMA, cd});

        /* Stem plot */
        if (show_stems)
            plm3d_plot_add_stem(&p, tx, ty, tz, nstem,
                (plm3d_stem_style){PLM_CYAN, 0.8f, PLM_YELLOW, 2.5f, "stems"});

        /* ---- Render ---- */
        plm3d_render(&p, &fb);

        /* Optional colorbar */
        if (show_cbar) {
            plm3d_colorbar(&p, &fb, PLM_CMAP_TURBO, 0.0, 7.5, "Z");
        }

        plm_fb_swizzle_rgba_bgra(&fb);
        plm3d_plot_reset(&p);
        needs_redraw = 0;
        mfb_wait_sync(win);
    }

    mfb_close(win);
    free(lx); free(ly); free(lz);
    free(sx); free(sy); free(sz); free(cd);
    free(tx); free(ty); free(tz);
    free(pixels);
    return 0;
}
