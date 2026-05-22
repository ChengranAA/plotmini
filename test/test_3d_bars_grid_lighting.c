#define PLOTMINI_IMPLEMENTATION
#define PLOTMINI3D_IMPLEMENTATION
#include "../plotmini.h"
#include "../plotmini3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------ */
/*  BMP-output stress test for all 4 new 3D features:                  */
/*    1. 3D bar charts                                                 */
/*    2. Wireframe-only surface mode                                   */
/*    3. Custom light direction                                        */
/*    4. Floor grid                                                    */
/* ------------------------------------------------------------------ */

static int test_bar_chart(void) {
    printf("  [TEST] 3D bar chart ... ");
    fflush(stdout);

    int w = 400, h = 350;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    /* Simple 3x3 grid of bars */
    int n = 9;
    float bx[9] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
    float by[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    float bz[9] = {1.0f, 2.5f, 1.8f, 0.5f, 4.0f, 2.2f, 3.0f, 1.5f, 3.5f};

    plm3d_plot p;
    plm3d_plot_init(&p);
    p.title = "3D Bar Test";
    p.view.azimuth = -45.0;
    p.view.elevation = 28.0;
    p.x_axis.grid = 1;
    p.y_axis.grid = 1;

    plm3d_plot_add_bar(&p, bx, by, bz, n, 0.35f, 0.35f,
        (plm3d_bar_style){PLM_BLUE, PLM_BLACK, 0.8f, PLM_CMAP_TURBO, "bars", NAN});

    plm3d_render(&p, &fb);
    plm3d_plot_reset(&p);

    int err = plm_fb_save_bmp(&fb, "test_bar.bmp");
    free(pixels);
    int ok = (err == 0);
    printf("%s\n", ok ? "PASS" : "FAIL (bmp write)");
    return ok ? 0 : 1;
}

static int test_wireframe_only(void) {
    printf("  [TEST] wireframe-only surface ... ");
    fflush(stdout);

    int w = 400, h = 350;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    int nx = 25, ny = 25;
    float *sx = (float *)malloc((size_t)nx * sizeof(float));
    float *sy = (float *)malloc((size_t)ny * sizeof(float));
    float *sz = (float *)malloc((size_t)nx * ny * sizeof(float));
    for (int i = 0; i < nx; i++) sx[i] = -3.0f + i * 6.0f / (nx - 1);
    for (int j = 0; j < ny; j++) sy[j] = -3.0f + j * 6.0f / (ny - 1);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            float x = sx[i], y = sy[j];
            sz[i * ny + j] = sinf(x) * cosf(y) * 2.0f;
        }

    plm3d_plot p;
    plm3d_plot_init(&p);
    p.title = "Wireframe-Only Test";
    p.view.azimuth = -50.0;
    p.view.elevation = 25.0;
    p.x_axis.grid = 1;
    p.y_axis.grid = 1;

    plm3d_plot_add_surface(&p, sx, sy, sz, nx, ny,
        (plm3d_surface_style){
            PLM_RED, 1.0f, PLM_CMAP_NONE, 0.0f, NULL, 0,
            1,           /* wireframe_only = 1 */
            0.0f, 0.0f   /* light = camera default */
        });

    plm3d_render(&p, &fb);
    plm3d_plot_reset(&p);

    int err = plm_fb_save_bmp(&fb, "test_wireframe.bmp");
    free(sx); free(sy); free(sz);
    free(pixels);
    int ok = (err == 0);
    printf("%s\n", ok ? "PASS" : "FAIL (bmp write)");
    return ok ? 0 : 1;
}

static int test_custom_light(void) {
    printf("  [TEST] custom light direction ... ");
    fflush(stdout);

    int w = 400, h = 350;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    int nx = 25, ny = 25;
    float *sx = (float *)malloc((size_t)nx * sizeof(float));
    float *sy = (float *)malloc((size_t)ny * sizeof(float));
    float *sz = (float *)malloc((size_t)nx * ny * sizeof(float));
    for (int i = 0; i < nx; i++) sx[i] = -3.0f + i * 6.0f / (nx - 1);
    for (int j = 0; j < ny; j++) sy[j] = -3.0f + j * 6.0f / (ny - 1);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            float x = sx[i], y = sy[j];
            float r = sqrtf(x*x + y*y);
            sz[i * ny + j] = (r < 1e-4f) ? 3.0f : sinf(r*2.0f)/r * 2.5f;
        }

    plm3d_plot p;
    plm3d_plot_init(&p);
    p.title = "Custom Light Test";
    p.view.azimuth = -60.0;
    p.view.elevation = 30.0;
    p.x_axis.grid = 1;
    p.y_axis.grid = 1;

    /* Light from upper-left instead of camera direction */
    plm3d_plot_add_surface(&p, sx, sy, sz, nx, ny,
        (plm3d_surface_style){
            PLM_GREY(140), 0.5f, PLM_CMAP_VIRIDIS, 0.9f, NULL, 0,
            0,            /* wireframe_only = 0 (filled) */
            120.0f, 50.0f /* light from az=120, el=50 (upper-left) */
        });

    plm3d_render(&p, &fb);
    plm3d_plot_reset(&p);

    int err = plm_fb_save_bmp(&fb, "test_custom_light.bmp");
    free(sx); free(sy); free(sz);
    free(pixels);
    int ok = (err == 0);
    printf("%s\n", ok ? "PASS" : "FAIL (bmp write)");
    return ok ? 0 : 1;
}

static int test_grid(void) {
    printf("  [TEST] floor grid ... ");
    fflush(stdout);

    int w = 400, h = 350;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    int nx = 20, ny = 20;
    float *sx = (float *)malloc((size_t)nx * sizeof(float));
    float *sy = (float *)malloc((size_t)ny * sizeof(float));
    float *sz = (float *)malloc((size_t)nx * ny * sizeof(float));
    for (int i = 0; i < nx; i++) sx[i] = -4.0f + i * 8.0f / (nx - 1);
    for (int j = 0; j < ny; j++) sy[j] = -4.0f + j * 8.0f / (ny - 1);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            float x = sx[i], y = sy[j];
            sz[i * ny + j] = expf(-0.2f*(x*x + y*y)) * 5.0f;
        }

    /* Test 1: grid ON */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.title = "Grid ON";
        p.view.azimuth = -45.0;
        p.view.elevation = 25.0;
        p.x_axis.grid = 1;
        p.y_axis.grid = 1;
        p.z_axis.grid = 1;

        plm3d_plot_add_surface(&p, sx, sy, sz, nx, ny,
            (plm3d_surface_style){PLM_GREY(180), 0.4f, PLM_CMAP_HEAT, 0.5f, NULL, 0,
             0, 0.0f, 0.0f});

        plm3d_render(&p, &fb);
        plm3d_plot_reset(&p);
    }

    int err1 = plm_fb_save_bmp(&fb, "test_grid_on.bmp");
    int ok = (err1 == 0);

    /* Test 2: grid OFF */
    {
        unsigned char *p2 = (unsigned char *)calloc((size_t)w * h * 4, 1);
        plm_fb fb2 = plm_fb_create(p2, w, h, PLM_RGBA8);

        plm3d_plot p;
        plm3d_plot_init(&p);
        p.title = "Grid OFF";
        p.view.azimuth = -45.0;
        p.view.elevation = 25.0;

        plm3d_plot_add_surface(&p, sx, sy, sz, nx, ny,
            (plm3d_surface_style){PLM_GREY(180), 0.4f, PLM_CMAP_HEAT, 0.5f, NULL, 0,
             0, 0.0f, 0.0f});

        plm3d_render(&p, &fb2);
        plm3d_plot_reset(&p);

        int err2 = plm_fb_save_bmp(&fb2, "test_grid_off.bmp");
        free(p2);
        if (err2 != 0) ok = 0;
    }

    free(sx); free(sy); free(sz);
    free(pixels);
    printf("%s\n", ok ? "PASS" : "FAIL (bmp write)");
    return ok ? 0 : 1;
}

static int test_mixed_all(void) {
    printf("  [TEST] mixed all features (subplot) ... ");
    fflush(stdout);

    int cols = 2, rows = 2;
    int cell_w = 300, cell_h = 260;
    int gap = 20;
    int w = cols * cell_w + (cols + 1) * gap;
    int h = rows * cell_h + (rows + 1) * gap;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    /* Shared surface data */
    int nx = 20, ny = 20;
    float *sx = (float *)malloc((size_t)nx * sizeof(float));
    float *sy = (float *)malloc((size_t)ny * sizeof(float));
    float *sz = (float *)malloc((size_t)nx * ny * sizeof(float));
    for (int i = 0; i < nx; i++) sx[i] = -4.0f + i * 8.0f / (nx - 1);
    for (int j = 0; j < ny; j++) sy[j] = -4.0f + j * 8.0f / (ny - 1);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            float x = sx[i], y = sy[j];
            float r = sqrtf(x*x + y*y);
            sz[i * ny + j] = (r < 1e-4f) ? 3.0f : sinf(r*2.5f)/r * 2.5f;
        }

    /* Bar data */
    int nbars = 16;
    float *bx = (float *)malloc((size_t)nbars * sizeof(float));
    float *by = (float *)malloc((size_t)nbars * sizeof(float));
    float *bz = (float *)malloc((size_t)nbars * sizeof(float));
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            int idx = i*4 + j;
            bx[idx] = (float)i;
            by[idx] = (float)j;
            bz[idx] = (float)((i+1)*(j+1)) * 0.5f;
        }

    plm_figure fig;
    plm_figure_init(&fig, 2, 2);
    fig.title = "All New Features Test";
    fig.hgap = gap;
    fig.vgap = gap;

    /* Cell (0,0): Bar chart with grid */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 0);
        p->title = "Bars + Grid";
        p->view.azimuth = -45.0;
        p->view.elevation = 28.0;
        p->x_axis.grid = 1;
        p->y_axis.grid = 1;
        plm3d_plot_add_bar(p, bx, by, bz, nbars, 0.35f, 0.35f,
            (plm3d_bar_style){PLM_GREEN, PLM_GREY(40), 0.7f, PLM_CMAP_TURBO, "bars", NAN});
    }

    /* Cell (0,1): Wireframe-only surface */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 1);
        p->title = "Wireframe Only";
        p->view.azimuth = -50.0;
        p->view.elevation = 22.0;
        p->x_axis.grid = 1;
        p->y_axis.grid = 1;
        plm3d_plot_add_surface(p, sx, sy, sz, nx, ny,
            (plm3d_surface_style){PLM_RED, 0.8f, PLM_CMAP_NONE, 0.0f, NULL, 0,
             1, 0.0f, 0.0f});
    }

    /* Cell (1,0): Filled surface with custom light direction */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 1, 0);
        p->title = "Custom Light (upper-left)";
        p->view.azimuth = -60.0;
        p->view.elevation = 30.0;
        p->x_axis.grid = 1;
        p->y_axis.grid = 1;
        plm3d_plot_add_surface(p, sx, sy, sz, nx, ny,
            (plm3d_surface_style){PLM_GREY(150), 0.5f, PLM_CMAP_VIRIDIS, 0.85f, NULL, 0,
             0, 120.0f, 45.0f});
    }

    /* Cell (1,1): Filled surface with default camera light + grid on all 3 axes */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 1, 1);
        p->title = "3-Axis Grid";
        p->view.azimuth = -40.0;
        p->view.elevation = 25.0;
        p->x_axis.grid = 1;
        p->y_axis.grid = 1;
        p->z_axis.grid = 1;
        plm3d_plot_add_surface(p, sx, sy, sz, nx, ny,
            (plm3d_surface_style){PLM_GREY(180), 0.4f, PLM_CMAP_INFERNO, 0.7f, NULL, 0,
             0, 0.0f, 0.0f});
    }

    plm_figure_render(&fig, &fb);

    int err = plm_fb_save_bmp(&fb, "test_mixed_all.bmp");
    int ok = (err == 0);

    plm_figure_reset(&fig);
    free(sx); free(sy); free(sz);
    free(bx); free(by); free(bz);
    free(pixels);
    printf("%s\n", ok ? "PASS" : "FAIL (bmp write)");
    return ok ? 0 : 1;
}

int main(void) {
    int failures = 0;
    printf("\n=== plotmini3d — New Features Test Suite ===\n\n");

    failures += test_bar_chart();
    failures += test_wireframe_only();
    failures += test_custom_light();
    failures += test_grid();
    failures += test_mixed_all();

    printf("\n=== %d/%d tests passed ===\n\n", 5 - failures, 5);
    return failures;
}
