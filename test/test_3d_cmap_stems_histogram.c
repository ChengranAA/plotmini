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

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  [TEST] %s ... ", name); fflush(stdout); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define CHECK_BMP(fb, path) ((plm_fb_save_bmp((fb), (path)) == 0))

/* ------------------------------------------------------------------ */
static void test_line_cmap(void) {
    TEST("color-by-height lines");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    int n = 200;
    float *lx = (float *)malloc((size_t)n * sizeof(float));
    float *ly = (float *)malloc((size_t)n * sizeof(float));
    float *lz = (float *)malloc((size_t)n * sizeof(float));
    for (int i = 0; i < n; i++) {
        float t = (float)i / (n - 1) * 6.0f * (float)M_PI;
        lx[i] = cosf(t) * (1.0f + 0.5f * sinf(t * 3.0f));
        ly[i] = sinf(t) * (1.0f + 0.5f * cosf(t * 3.0f));
        lz[i] = t * 0.5f;  /* increasing Z -> color gradient */
    }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Line cmap test";
    plot.view.azimuth = -50.0;
    plot.view.elevation = 30.0;
    plm3d_plot_add_line(&plot, lx, ly, lz, n,
        (plm3d_line_style){PLM_RED, 1.5f, "spiral", PLM_CMAP_TURBO});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_line_cmap.bmp")) PASS(); else FAIL("bmp");
    free(lx); free(ly); free(lz); free(p);
}

/* ------------------------------------------------------------------ */
static void test_scatter_cmap(void) {
    TEST("colormapped scatters");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    int n = 150;
    float *sx = (float *)malloc((size_t)n * sizeof(float));
    float *sy = (float *)malloc((size_t)n * sizeof(float));
    float *sz = (float *)malloc((size_t)n * sizeof(float));
    float *cd = (float *)malloc((size_t)n * sizeof(float));  /* custom color data */
    for (int i = 0; i < n; i++) {
        float t = (float)i / (n - 1) * 4.0f * (float)M_PI;
        sx[i] = cosf(t) * 2.5f;
        sy[i] = sinf(t * 1.3f) * 2.5f;
        sz[i] = t * 0.4f;
        cd[i] = sinf(t);  /* oscillating color data, not monotonic Z */
    }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Scatter cmap test";
    plot.view.azimuth = -45.0;
    plot.view.elevation = 25.0;
    plm3d_plot_add_scatter(&plot, sx, sy, sz, n,
        (plm3d_scatter_style){PLM_WHITE, 3.5f, "points", PLM_CMAP_PLASMA, cd});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_scatter_cmap.bmp")) PASS(); else FAIL("bmp");
    free(sx); free(sy); free(sz); free(cd); free(p);
}

/* ------------------------------------------------------------------ */
static void test_bar_baseline(void) {
    TEST("bar custom baseline");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    int n = 12;
    float bx[12], by[12], bz[12];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            int idx = i * 3 + j;
            bx[idx] = (float)i;
            by[idx] = (float)j;
            bz[idx] = (float)(i + j + 1) * 0.8f;
        }
    }

    /* Half bars rise from baseline=2.0, half from auto */
    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Baseline=2.0 test";
    plot.view.azimuth = -45.0;
    plot.view.elevation = 28.0;
    plot.z_axis.min = 0.0; plot.z_axis.max = 6.0;
    plm3d_plot_add_bar(&plot, bx, by, bz, n, 0.35f, 0.35f,
        (plm3d_bar_style){PLM_BLUE, PLM_BLACK, 0.6f, PLM_CMAP_VIRIDIS, "bars", 2.0f});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_bar_baseline.bmp")) PASS(); else FAIL("bmp");
    free(p);
}

/* ------------------------------------------------------------------ */
static void test_stems(void) {
    TEST("3D stem plot");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    int n = 60;
    float *sx = (float *)malloc((size_t)n * sizeof(float));
    float *sy = (float *)malloc((size_t)n * sizeof(float));
    float *sz = (float *)malloc((size_t)n * sizeof(float));
    for (int i = 0; i < n; i++) {
        float t = (float)i / (n - 1) * 3.0f * (float)M_PI;
        sx[i] = cosf(t * 2.0f) * 2.0f;
        sy[i] = sinf(t * 1.5f) * 2.0f;
        sz[i] = 1.0f + fabsf(sinf(t * 3.0f)) * 3.0f;
    }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Stem plot test";
    plot.view.azimuth = -50.0;
    plot.view.elevation = 25.0;
    plm3d_plot_add_stem(&plot, sx, sy, sz, n,
        (plm3d_stem_style){PLM_GREEN, 1.0f, PLM_RED, 3.0f, "stems"});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_stems.bmp")) PASS(); else FAIL("bmp");
    free(sx); free(sy); free(sz); free(p);
}

/* ------------------------------------------------------------------ */
static void test_stems_no_marker(void) {
    TEST("3D stem plot (no marker)");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    int n = 40;
    float *sx = (float *)malloc((size_t)n * sizeof(float));
    float *sy = (float *)malloc((size_t)n * sizeof(float));
    float *sz = (float *)malloc((size_t)n * sizeof(float));
    for (int i = 0; i < n; i++) {
        sx[i] = ((float)i / (n-1) - 0.5f) * 5.0f;
        sy[i] = sinf(sx[i] * 2.0f) * 1.5f;
        sz[i] = cosf(sx[i] * 2.0f) * 2.0f + 2.5f;
    }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Stems no marker";
    plot.view.azimuth = -40.0;
    plot.view.elevation = 20.0;
    plm3d_plot_add_stem(&plot, sx, sy, sz, n,
        (plm3d_stem_style){PLM_CYAN, 1.5f, {0,0,0,0}, 0.0f, NULL});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_stems_nomarker.bmp")) PASS(); else FAIL("bmp");
    free(sx); free(sy); free(sz); free(p);
}

/* ------------------------------------------------------------------ */
static void test_histogram(void) {
    TEST("3D bivariate histogram");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    /* Generate correlated bivariate normal samples */
    int n = 2000;
    float *hx = (float *)malloc((size_t)n * sizeof(float));
    float *hy = (float *)malloc((size_t)n * sizeof(float));
    for (int i = 0; i < n; i++) {
        float u1 = (float)rand() / (float)RAND_MAX;
        float u2 = (float)rand() / (float)RAND_MAX;
        float z1 = sqrtf(-2.0f * logf(u1 + 1e-9f)) * cosf(2.0f * (float)M_PI * u2);
        float z2 = sqrtf(-2.0f * logf(u1 + 1e-9f)) * sinf(2.0f * (float)M_PI * u2);
        hx[i] = z1;
        hy[i] = z1 * 0.5f + z2 * 0.866f;  /* correlated */
    }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "2D Histogram";
    plot.view.azimuth = -50.0;
    plot.view.elevation = 30.0;
    plot.x_axis.grid = 1;
    plot.y_axis.grid = 1;
    plot.legend_position = PLM_LEGEND_TOP_RIGHT;
    plm3d_plot_add_hist(&plot, hx, hy, n, 10, 10,
        (plm3d_bar_style){PLM_GREY(180), PLM_GREY(40), 0.5f, PLM_CMAP_INFERNO, "N(0,1)", NAN});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_histogram.bmp")) PASS(); else FAIL("bmp");
    free(hx); free(hy); free(p);
}

/* ------------------------------------------------------------------ */
static void test_colorbar(void) {
    TEST("colorbar on 3D plot");
    int w = 500, h = 380;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    int nx = 25, ny = 25;
    float *sx = (float *)malloc((size_t)nx * sizeof(float));
    float *sy = (float *)malloc((size_t)ny * sizeof(float));
    float *sz = (float *)malloc((size_t)nx * ny * sizeof(float));
    for (int i = 0; i < nx; i++) sx[i] = -3.0f + i * 6.0f / (nx - 1);
    for (int j = 0; j < ny; j++) sy[j] = -3.0f + j * 6.0f / (ny - 1);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            float x = sx[i], y = sy[j];
            sz[i * ny + j] = sinf(x) * cosf(y) * 2.5f + 2.0f;
        }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Colorbar test";
    plot.view.azimuth = -50.0;
    plot.view.elevation = 25.0;
    plot.margin_right = 80;  /* make room for colorbar */
    plm3d_plot_add_surface(&plot, sx, sy, sz, nx, ny,
        (plm3d_surface_style){PLM_GREY(160), 0.4f, PLM_CMAP_TURBO, 0.0f, NULL, 0,
         0, 0.0f, 0.0f});
    plm3d_render(&plot, &fb);
    plm3d_colorbar(&plot, &fb, PLM_CMAP_TURBO, -2.5, 4.5, "Z");
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_colorbar.bmp")) PASS(); else FAIL("bmp");
    free(sx); free(sy); free(sz); free(p);
}

/* ------------------------------------------------------------------ */
static void test_edge_nan_data(void) {
    TEST("NaN handling in scatter/lines");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    /* Scatter with some NaN points mixed in */
    int n = 30;
    float sx[30], sy[30], sz[30];
    for (int i = 0; i < n; i++) {
        sx[i] = (float)i * 0.2f;
        sy[i] = sinf((float)i * 0.5f);
        sz[i] = (i == 10 || i == 20) ? NAN : cosf((float)i * 0.5f);
    }

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "NaN edge test";
    plot.view.azimuth = -45.0;
    plot.view.elevation = 25.0;
    plm3d_plot_add_scatter(&plot, sx, sy, sz, n,
        (plm3d_scatter_style){PLM_RED, 4.0f, "scatter", PLM_CMAP_VIRIDIS, NULL});
    plm3d_plot_add_line(&plot, sx, sy, sz, n,
        (plm3d_line_style){PLM_BLUE, 1.0f, "line", PLM_CMAP_NONE});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_edge_nan.bmp")) PASS(); else FAIL("bmp");
    free(p);
}

/* ------------------------------------------------------------------ */
static void test_edge_zero_data(void) {
    TEST("zero-height bars / empty data");
    int w = 400, h = 350;
    unsigned char *p = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(p, w, h, PLM_RGBA8);

    float bx[4] = {0, 1, 0, 1};
    float by[4] = {0, 0, 1, 1};
    float bz[4] = {0, 2, 1.5f, 0};  /* two zero-height bars */

    plm3d_plot plot;
    plm3d_plot_init(&plot);
    plot.title = "Zero-height bars";
    plot.view.azimuth = -45.0;
    plot.view.elevation = 28.0;
    plm3d_plot_add_bar(&plot, bx, by, bz, 4, 0.4f, 0.4f,
        (plm3d_bar_style){PLM_GREEN, PLM_BLACK, 0.8f, PLM_CMAP_TURBO, "bars", NAN});
    plm3d_render(&plot, &fb);
    plm3d_plot_reset(&plot);

    if (CHECK_BMP(&fb, "test_edge_zero.bmp")) PASS(); else FAIL("bmp");
    free(p);
}

/* ------------------------------------------------------------------ */
static void test_mixed_all_tier2(void) {
    TEST("mixed all tier2 features (subplot)");
    int cols = 2, rows = 3;
    int cell_w = 280, cell_h = 240;
    int gap = 16;
    int w = cols * cell_w + (cols + 1) * gap;
    int h = rows * cell_h + (rows + 1) * gap;
    unsigned char *pixels = (unsigned char *)calloc((size_t)w * h * 4, 1);
    plm_fb fb = plm_fb_create(pixels, w, h, PLM_RGBA8);

    /* Shared data */
    int nline = 150;
    float *lx = (float *)malloc((size_t)nline * sizeof(float));
    float *ly = (float *)malloc((size_t)nline * sizeof(float));
    float *lz = (float *)malloc((size_t)nline * sizeof(float));
    for (int i = 0; i < nline; i++) {
        float t = (float)i / (nline-1) * 5.0f * (float)M_PI;
        lx[i] = cosf(t) * (2.0f + sinf(t*0.7f));
        ly[i] = sinf(t) * (2.0f + cosf(t*0.7f));
        lz[i] = t * 0.3f;
    }

    int nscat = 80;
    float *sx = (float *)malloc((size_t)nscat * sizeof(float));
    float *sy = (float *)malloc((size_t)nscat * sizeof(float));
    float *sz = (float *)malloc((size_t)nscat * sizeof(float));
    for (int i = 0; i < nscat; i++) {
        float t = (float)i / (nscat-1) * 3.0f * (float)M_PI;
        sx[i] = cosf(t*1.7f) * 2.5f;
        sy[i] = sinf(t*0.9f) * 2.5f;
        sz[i] = 1.5f + sinf(t*2.0f)*2.0f;
    }

    int nhist = 1500;
    float *hx = (float *)malloc((size_t)nhist * sizeof(float));
    float *hy = (float *)malloc((size_t)nhist * sizeof(float));
    for (int i = 0; i < nhist; i++) {
        float u1 = (float)rand()/RAND_MAX, u2 = (float)rand()/RAND_MAX;
        hx[i] = sqrtf(-2*logf(u1+1e-9f))*cosf(2*(float)M_PI*u2);
        hy[i] = sqrtf(-2*logf(u1+1e-9f))*sinf(2*(float)M_PI*u2);
    }

    plm_figure fig;
    plm_figure_init(&fig, rows, cols);
    fig.title = "Tier 1+2 Features — Mixed Figure";
    fig.hgap = gap;
    fig.vgap = gap;

    /* (0,0): Line with cmap */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 0);
        p->title = "Line cmap (Turbo)";
        p->view.azimuth = -45; p->view.elevation = 25;
        plm3d_plot_add_line(p, lx, ly, lz, nline,
            (plm3d_line_style){PLM_RED, 1.2f, "spiral", PLM_CMAP_TURBO});
    }

    /* (0,1): Scatter with cmap */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 0, 1);
        p->title = "Scatter cmap (Plasma)";
        p->view.azimuth = -50; p->view.elevation = 22;
        plm3d_plot_add_scatter(p, sx, sy, sz, nscat,
            (plm3d_scatter_style){PLM_WHITE, 3.5f, "pts", PLM_CMAP_PLASMA, NULL});
    }

    /* (1,0): Stem plot */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 1, 0);
        p->title = "Stems + Markers";
        p->view.azimuth = -55; p->view.elevation = 28;
        plm3d_plot_add_stem(p, sx, sy, sz, nscat,
            (plm3d_stem_style){PLM_CYAN, 0.8f, PLM_YELLOW, 2.5f, "stems"});
    }

    /* (1,1): Histogram */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 1, 1);
        p->title = "2D Histogram";
        p->view.azimuth = -50; p->view.elevation = 30;
        p->x_axis.grid = 1; p->y_axis.grid = 1;
        plm3d_plot_add_hist(p, hx, hy, nhist, 8, 8,
            (plm3d_bar_style){PLM_GREY(180), PLM_GREY(40), 0.4f, PLM_CMAP_INFERNO, "hist", NAN});
    }

    /* (2,0): Bar with baseline */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 2, 0);
        p->title = "Bars baseline=2";
        p->view.azimuth = -45; p->view.elevation = 28;
        p->z_axis.min = 0; p->z_axis.max = 6;
        float bx2[9], by2[9], bz2[9];
        for (int i=0;i<3;i++) for(int j=0;j<3;j++) {
            int idx=i*3+j;
            bx2[idx]=(float)i; by2[idx]=(float)j; bz2[idx]=(float)(i+j+1)*0.7f;
        }
        plm3d_plot_add_bar(p, bx2, by2, bz2, 9, 0.35f, 0.35f,
            (plm3d_bar_style){PLM_BLUE, PLM_BLACK, 0.6f, PLM_CMAP_VIRIDIS, "bars", 2.0f});
    }

    /* (2,1): Colorbar demo */
    {
        plm3d_plot *p = (plm3d_plot *)plm_figure_plot_3d(&fig, 2, 1);
        p->title = "Surface + Colorbar";
        p->view.azimuth = -50; p->view.elevation = 25;
        p->margin_right = 70;
        int nx=20, ny=20;
        float *sx2=(float*)malloc(nx*sizeof(float));
        float *sy2=(float*)malloc(ny*sizeof(float));
        float *sz2=(float*)malloc(nx*ny*sizeof(float));
        for(int i=0;i<nx;i++) sx2[i] = -3.0f+i*6.0f/(nx-1);
        for(int j=0;j<ny;j++) sy2[j] = -3.0f+j*6.0f/(ny-1);
        for(int i=0;i<nx;i++) for(int j=0;j<ny;j++)
            sz2[i*ny+j] = sinf(sx2[i])*cosf(sy2[j])*2.5f+2.0f;
        plm3d_plot_add_surface(p, sx2, sy2, sz2, nx, ny,
            (plm3d_surface_style){PLM_GREY(160),0.4f,PLM_CMAP_TURBO,0.0f,NULL,0,0,0,0});
        plm_figure_render(&fig, &fb);
        plm3d_colorbar(p, &fb, PLM_CMAP_TURBO, -2.5, 4.5, "Z");
        free(sx2);free(sy2);free(sz2);
        /* Already rendered, just check BMP */
        goto check_bmp;
    }

    plm_figure_render(&fig, &fb);

check_bmp:
    {
        int ok = CHECK_BMP(&fb, "test_mixed_tier2.bmp");
        plm_figure_reset(&fig);
        free(lx); free(ly); free(lz);
        free(sx); free(sy); free(sz);
        free(hx); free(hy);
        free(pixels);
        if (ok) PASS(); else FAIL("bmp");
    }
}

/* ------------------------------------------------------------------ */
int main(void) {
    printf("\n=== plotmini3d — Tier 1+2 Features Test Suite ===\n\n");

    /* Tier 1 */
    test_line_cmap();
    test_scatter_cmap();
    test_colorbar();

    /* Tier 2 */
    test_bar_baseline();
    test_stems();
    test_stems_no_marker();
    test_histogram();

    /* Edge cases */
    test_edge_nan_data();
    test_edge_zero_data();

    /* Integration */
    test_mixed_all_tier2();

    printf("\n=== %d/%d tests passed ===\n\n",
           tests_passed, tests_passed + tests_failed);
    return tests_failed;
}
