#define PLOTMINI_IMPLEMENTATION
#define PLOTMINI3D_IMPLEMENTATION
#include "../../plotmini.h"
#include "../../plotmini3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    const int win_w = 800;
    const int win_h = 600;

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    /* ---- 1. surface (wireframe): z = sin(sqrt(x*x + y*y)) --------- */
    int nx = 40, ny = 40;
    float *surf_x = (float *)malloc((size_t)nx * sizeof(float));
    float *surf_y = (float *)malloc((size_t)ny * sizeof(float));
    float *surf_z = (float *)malloc((size_t)nx * ny * sizeof(float));

    for (int i = 0; i < nx; i++) surf_x[i] = -8.0f + i * 16.0f / (nx - 1);
    for (int j = 0; j < ny; j++) surf_y[j] = -8.0f + j * 16.0f / (ny - 1);
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            float r = sqrtf(surf_x[i] * surf_x[i] + surf_y[j] * surf_y[j]);
            surf_z[i * ny + j] = r < 1e-4f ? 1.0f : sinf(r) / r * 3.0f;
        }
    }

    /* ---- 2. 3D line: spiral in 3D --------------------------------- */
    int nline = 200;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline - 1) * 4.0f * (float)M_PI;
        lx[i] = 6.0f * cosf(t);
        ly[i] = 6.0f * sinf(t);
        lz[i] = t * 0.5f - 1.0f;
    }

    /* ---- 3. scatter: random points in a cloud --------------------- */
    int nscat = 60;
    float *sx = (float *)malloc((size_t)nscat * sizeof(float));
    float *sy = (float *)malloc((size_t)nscat * sizeof(float));
    float *sz = (float *)malloc((size_t)nscat * sizeof(float));
    for (int i = 0; i < nscat; i++) {
        sx[i] = ((float)rand() / RAND_MAX - 0.5f) * 14.0f;
        sy[i] = ((float)rand() / RAND_MAX - 0.5f) * 14.0f;
        sz[i] = ((float)rand() / RAND_MAX) * 4.0f - 1.0f;
    }

    /* ---- build plot ----------------------------------------------- */
    plm3d_plot p;
    plm3d_plot_init(&p);
    p.title   = "plotmini3d - wireframe + line + scatter";
    p.x_label = "X";
    p.y_label = "Y";
    p.z_label = "Z";

    p.view.azimuth   = -50.0;
    p.view.elevation = 30.0;

    plm3d_plot_add_surface(&p, surf_x, surf_y, surf_z, nx, ny,
        (plm3d_surface_style){PLM_GREY(140), 1.0f});

    plm3d_plot_add_line(&p, lx, ly, lz, nline,
        (plm3d_line_style){PLM_RED, 1.0f});

    plm3d_plot_add_scatter(&p, sx, sy, sz, nscat,
        (plm3d_scatter_style){PLM_BLUE, 3.5f});

    /* ---- render and save ------------------------------------------ */
    plm3d_render(&p, &fb);
    plm_fb_save_bmp(&fb, "plot3d_basic.bmp");
    printf("Saved plot3d_basic.bmp\n");

    /* ---- cleanup -------------------------------------------------- */
    plm3d_plot_reset(&p);
    free(surf_x); free(surf_y); free(surf_z);
    free(lx); free(ly); free(lz);
    free(sx); free(sy); free(sz);
    free(pixels);

    return 0;
}
