#define PLOTMINI_DARK_THEME
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
    int w = 960, h = 680;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    /* ---- Multi-peak surface ---- */
    int nx = 70, ny = 70;
    float *sx = (float *)malloc((size_t)nx * sizeof(float));
    float *sy = (float *)malloc((size_t)ny * sizeof(float));
    float *sz = (float *)malloc((size_t)nx * ny * sizeof(float));
    for (int i = 0; i < nx; i++) sx[i] = -5.0f + i * 10.0f / (nx - 1);
    for (int j = 0; j < ny; j++) sy[j] = -5.0f + j * 10.0f / (ny - 1);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            float x = sx[i], y = sy[j];
            float z  = 3.5f * expf(-0.3f * (x*x + y*y));
            z += 2.0f * expf(-0.8f * ((x-2.5f)*(x-2.5f) + (y+1.5f)*(y+1.5f)));
            z += 2.5f * expf(-0.6f * ((x+2.0f)*(x+2.0f) + (y-2.0f)*(y-2.0f)));
            z += 0.4f * sinf(x * 1.8f) * cosf(y * 1.8f);
            sz[i * ny + j] = z;
        }

    /* ---- Spiral ---- */
    int nline = 400;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline - 1) * 6.0f * (float)M_PI;
        float r = 3.8f + 0.9f * sinf(t * 0.55f);
        lx[i] = r * cosf(t); ly[i] = r * sinf(t); lz[i] = t * 0.28f + 0.5f;
    }

    /* ---- Build ---- */
    plm3d_plot p;
    plm3d_plot_init(&p);
    p.title = "plotmini3d";
    p.x_label = "X"; p.y_label = "Y"; p.z_label = "Z";
    p.view.azimuth   = -42.0;
    p.view.elevation =  28.0;
    p.view.distance  = 3.8;
    p.view.projection = PLM3D_PERSPECTIVE;
    p.margin_right   = 80;
    p.x_axis.grid = 1;
    p.y_axis.grid = 1;

    plm3d_plot_add_surface(&p, sx, sy, sz, nx, ny,
        (plm3d_surface_style){PLM_GREY(180), 0.3f, PLM_CMAP_TURBO,
            0.85f, NULL, 0, 0, 135.0f, 55.0f});
    plm3d_plot_add_line(&p, lx, ly, lz, nline,
        (plm3d_line_style){PLM_RED, 1.2f, NULL, PLM_CMAP_PLASMA});

    plm3d_render(&p, &fb);
    plm3d_colorbar(&p, &fb, PLM_CMAP_TURBO, p.z_axis.min, p.z_axis.max, "Z");

    plm_fb_save_bmp(&fb, "showcase.bmp");
    printf("Wrote showcase.bmp (%dx%d)\n", w, h);

    plm3d_plot_reset(&p);
    free(sx); free(sy); free(sz);
    free(lx); free(ly); free(lz);
    free(pixels);
    return 0;
}
