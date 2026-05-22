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
/*  3D Histogram + Bar Baseline Control                                */
/*  Interactive: arrows=rotate, R=reset, B=cycle baseline, G=grid     */
/* ------------------------------------------------------------------ */

int main(void) {
    const int win_w = 960;
    const int win_h = 640;

    struct mfb_window *win = mfb_open("plotmini3d — Histogram + Baseline", win_w, win_h);
    if (!win) { fprintf(stderr, "Failed to open window\n"); return 1; }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Histogram: correlated bivariate normal samples ---- */
    int nsamp = 2500;
    float *hx = (float *)malloc((size_t)nsamp * sizeof(float));
    float *hy = (float *)malloc((size_t)nsamp * sizeof(float));
    for (int i = 0; i < nsamp; i++) {
        float u1 = (float)rand() / (float)RAND_MAX;
        float u2 = (float)rand() / (float)RAND_MAX;
        float z1 = sqrtf(-2.0f * logf(u1 + 1e-9f)) * cosf(2.0f * (float)M_PI * u2);
        float z2 = sqrtf(-2.0f * logf(u1 + 1e-9f)) * sinf(2.0f * (float)M_PI * u2);
        hx[i] = z1;
        hy[i] = z1 * 0.6f + z2 * 0.8f;  /* correlated */
    }

    /* ---- Bar data (for baseline demo) ---- */
    int nbars = 16;
    float bx[16], by[16], bz[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int idx = i * 4 + j;
            bx[idx] = (float)i;
            by[idx] = (float)j;
            bz[idx] = (float)((i + 1) * (j + 1)) * 0.6f;
        }
    }

    /* ---- State ---- */
    double azimuth   = -50.0;
    double elevation =  28.0;
    int    grid_on   = 1;
    int    baseline_mode = 0;  /* 0=auto, 1=b=0, 2=b=2 */
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
        if (keys[MFB_KB_KEY_R])     { azimuth = -50.0; elevation = 28.0; changed = 1; }

        { static int w=0; if (keys[MFB_KB_KEY_G]) { if(!w){grid_on=!grid_on;changed=1;} w=1; } else w=0; }
        { static int w=0; if (keys[MFB_KB_KEY_B]) {
            if(!w){baseline_mode=(baseline_mode+1)%3;changed=1;} w=1; } else w=0; }

        if (elevation >  89.0) elevation =  89.0;
        if (elevation < -89.0) elevation = -89.0;
        if (changed) needs_redraw = 1;
        if (!needs_redraw) { mfb_wait_sync(win); continue; }

        /* ---- Build figure with 2 cells ---- */
        plm_figure fig;
        plm_figure_init(&fig, 1, 2);
        fig.hgap = 20; fig.vgap = 16;

        {
            char title[256];
            const char *blabels[] = {"auto", "b=0", "b=2"};
            snprintf(title, sizeof(title),
                     "3D Histogram + Bars  baseline=%s  az=%.0f el=%.0f  [arrows/B/G/R/ESC]",
                     blabels[baseline_mode], azimuth, elevation);
            fig.title = title;
        }

        /* Left: 2D histogram */
        {
            plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 0);
            p->title = "Bivariate Histogram";
            p->view.azimuth   = azimuth;
            p->view.elevation = elevation;
            p->x_axis.grid = grid_on;
            p->y_axis.grid = grid_on;
            p->legend_position = PLM_LEGEND_TOP_RIGHT;

            plm3d_plot_add_hist(p, hx, hy, nsamp, 10, 10,
                (plm3d_bar_style){PLM_GREY(180), PLM_GREY(40), 0.4f,
                                                  PLM_CMAP_INFERNO, "bivariate N(0,1)", NAN});
        }

        /* Right: Bar chart with configurable baseline */
        {
            plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 1);
            p->title = "Bar Chart";
            p->view.azimuth   = azimuth;
            p->view.elevation = elevation;
            p->x_axis.grid = grid_on;
            p->y_axis.grid = grid_on;
            p->legend_position = PLM_LEGEND_TOP_RIGHT;
            p->z_axis.min = -1.0;
            p->z_axis.max = 10.0;

            float bl;
            if (baseline_mode == 0) bl = NAN;       /* auto */
            else if (baseline_mode == 1) bl = 0.0f;  /* from 0 */
            else bl = 2.0f;                          /* from 2 */

            plm3d_plot_add_bar(p, bx, by, bz, nbars, 0.38f, 0.38f,
                (plm3d_bar_style){PLM_BLUE, PLM_GREY(40), 0.7f,
                                  PLM_CMAP_VIRIDIS, "bars", bl});
        }

        /* ---- Render ---- */
        plm_figure_render(&fig, &fb);
        plm_fb_swizzle_rgba_bgra(&fb);

        plm_figure_reset(&fig);
        needs_redraw = 0;
        mfb_wait_sync(win);
    }

    mfb_close(win);
    free(hx); free(hy);
    free(pixels);
    return 0;
}
