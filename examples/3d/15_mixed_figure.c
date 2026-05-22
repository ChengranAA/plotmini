#define PLOTMINI_DARK_THEME
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
    const int gap     = 28;

    int cell_w = 320, cell_h = 280;
    int cols = 2, rows = 2;

    int win_w = cols * cell_w + (cols + 1) * gap;
    int win_h = rows * cell_h + (rows + 1) * gap;

    struct mfb_window *win = mfb_open("plotmini3d — mixed 2D/3D figure", win_w, win_h);
    if (!win) { fprintf(stderr, "Failed to open window\n"); return 1; }

    unsigned char *pixels = (unsigned char *)calloc((size_t)win_w * win_h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- Build 3D surface: sinc·gaussian + ripple ---- */
    int nx = 45, ny = 45;
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
    int nline = 250;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline - 1) * 4.0f * (float)M_PI;
        float r = 3.0f + 1.5f * sinf(t * 0.7f);
        lx[i] = r * cosf(t);
        ly[i] = r * sinf(t);
        lz[i] = t * 0.3f - 1.5f;
    }

    /* ---- 3D scatter points ---- */
    int nscat = 80;
    float *sx = (float *)malloc((size_t)nscat * sizeof(float));
    float *sy = (float *)malloc((size_t)nscat * sizeof(float));
    float *sz = (float *)malloc((size_t)nscat * sizeof(float));
    for (int i = 0; i < nscat; i++) {
        float t = (float)i / (nscat - 1) * 3.0f * (float)M_PI;
        sx[i] = 3.5f * cosf(t);
        sy[i] = 3.5f * sinf(t * 0.5f);
        sz[i] = 2.0f * sinf(t * 1.3f);
    }

    /* ---- 2D data for line plot ---- */
    int n2d = 120;
    float *x2d = (float *)malloc((size_t)n2d * sizeof(float));
    float *y2d_sin = (float *)malloc((size_t)n2d * sizeof(float));
    float *y2d_cos = (float *)malloc((size_t)n2d * sizeof(float));
    for (int i = 0; i < n2d; i++) {
        x2d[i] = (float)i / (n2d - 1) * 4.0f * (float)M_PI;
        y2d_sin[i] = sinf(x2d[i]);
        y2d_cos[i] = cosf(x2d[i]);
    }

    /* ---- 2D histogram data ---- */
    int nsamp = 2000;
    float *samples = (float *)malloc((size_t)nsamp * sizeof(float));
    for (int i = 0; i < nsamp; i++) {
        /* Box-Muller: normal distribution */
        float u1 = (float)rand() / (float)RAND_MAX;
        float u2 = (float)rand() / (float)RAND_MAX;
        samples[i] = sqrtf(-2.0f * logf(u1 + 1e-9f)) * cosf(2.0f * (float)M_PI * u2);
    }

    /* ---- Build mixed figure ---- */
    plm_figure fig;
    plm_figure_init(&fig, 2, 2);
    fig.title = "plotmini3d — Mixed 2D/3D Figure  |  "
                "Perspective · Backface Culling · Legends  |  ESC to quit";

    /* Top-left: 3D surface with PERSPECTIVE, backface culling, legend */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 0);
        p->title = "3D Surface — Perspective";
        p->view.azimuth   = -45.0;
        p->view.elevation = 28.0;
        p->view.distance  = 3.0;              /* perspective! */
        p->view.projection = PLM3D_PERSPECTIVE;
        p->legend_position = PLM_LEGEND_TOP_RIGHT;
        p->margin_left   = 34;
        p->margin_top    = 24;
        p->margin_right  = 8;
        p->margin_bottom = 28;

        plm3d_surface_style sstyle = {PLM_GREY(160), 0.35f, PLM_CMAP_TURBO, 0.85f, "sinc·gauss", 1, 0, 0.0f, 0.0f};
        plm3d_plot_add_surface(p, surf_x, surf_y, surf_z, nx, ny, sstyle);
    }

    /* Top-right: 2D line plot with legend */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 1);
        p->title = "2D Lines — Legend";
        p->x_label = "x";
        p->y_label = "y";
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        plm_plot_add_line(p, x2d, y2d_sin, n2d,
            (plm_line_style){PLM_RED, 1.8f, "sin(x)", 0, 0});
        plm_plot_add_line(p, x2d, y2d_cos, n2d,
            (plm_line_style){PLM_BLUE, 1.8f, "cos(x)", 0, PLM_DASH_DASHED});
    }

    /* Bottom-left: 3D scatter + line with legend */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 1, 0);
        p->title = "3D Scatter + Line — Legend";
        p->view.azimuth   = -55.0;
        p->view.elevation = 20.0;
        p->legend_position = PLM_LEGEND_BOTTOM_RIGHT;
        p->margin_left   = 34;
        p->margin_top    = 24;
        p->margin_right  = 8;
        p->margin_bottom = 28;

        plm3d_plot_add_line(p, lx, ly, lz, nline,
            (plm3d_line_style){PLM_RED, 1.2f, "spiral", PLM_CMAP_NONE});
        plm3d_plot_add_scatter(p, sx, sy, sz, nscat,
            (plm3d_scatter_style){PLM_CYAN, 3.5f, "points", PLM_CMAP_NONE, NULL});
    }

    /* Bottom-right: 2D histogram */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 1);
        p->title = "2D Histogram";
        p->x_label = "Value";
        p->y_label = "Count";
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        plm_plot_add_hist(p, samples, nsamp, 30, 0,
            (plm_bar_style){PLM_GREEN, PLM_GREY(60), 0.5f, "N(0,1)"});
    }

    /* ---- Render ---- */
    plm_figure_render(&fig, &fb);
    plm_fb_swizzle_rgba_bgra(&fb);

    while (1) {
        int state = mfb_update(win, fb.pixels);
        if (state < 0) break;
        if (mfb_get_key_buffer(win)[MFB_KB_KEY_ESCAPE]) break;
        mfb_wait_sync(win);
    }

    mfb_close(win);

    free(surf_x); free(surf_y); free(surf_z);
    free(lx); free(ly); free(lz);
    free(sx); free(sy); free(sz);
    free(x2d); free(y2d_sin); free(y2d_cos);
    free(samples);
    free(pixels);

    plm_figure_reset(&fig);
    return 0;
}
