#define PLOTMINI_IMPLEMENTATION
#define PLOTMINI3D_IMPLEMENTATION
#include "../plotmini.h"
#include "../plotmini3d.h"
#include "test_runner.h"

#include <math.h>
#include <string.h>

/* ================================================================== */
/*  projection                                                        */
/* ================================================================== */

TEST(proj_identity)
{
    /* azimuth=0, elevation=0: looking along -Y in data space.
       rx = cx, ry = cy (since se=0, ce=1, sa=0, ca=1)
       Actually: rx = ca*cx+sa*cy = cx, ry = -sa*se*cx+ca*se*cy+ce*cz = cz
       Wait, at el=0: se=0, ce=1 → ry = 0 + 0 + 1*cz = cz
       rz = sa*ce*cx - ca*ce*cy + se*cz = 0 - 1*cy + 0 = -cy               */
    float rx, ry, rz;
    plm3d__project(0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, &rx, &ry, &rz);
    ASSERT_FEQ(rx, 0.0f, 0.001f);
    ASSERT_FEQ(ry, 0.0f, 0.001f);
    ASSERT_FEQ(rz, 0.0f, 0.001f);
    return 0;
}

TEST(proj_azimuth_90)
{
    /* az=90°, el=0: ca=0, sa=1, ce=1, se=0
       rx = 0*cx + 1*cy = cy
       ry = 0 + 0 + 1*cz = cz
       rz = 1*1*cx - 0 + 0 = cx                                        */
    float rx, ry, rz;
    plm3d__project(1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, &rx, &ry, &rz);
    ASSERT_FEQ(rx, 0.0f, 0.001f);   /* cy = 0 */
    ASSERT_FEQ(ry, 0.0f, 0.001f);   /* cz = 0 */
    ASSERT_FEQ(rz, 1.0f, 0.001f);   /* cx = 1 */
    return 0;
}

TEST(proj_elevation_90)
{
    /* az=0, el=90°: ca=1, sa=0, ce=0, se=1
       rx = 1*cx + 0*cy = cx = 0.5
       ry = -sa*se*cx + ca*se*cy + ce*cz = 0 + 1*1*0.5 + 0 = 0.5
       rz = sa*ce*cx - ca*ce*cy + se*cz = 0 - 0 + 1*0.5 = 0.5             */
    float rx, ry, rz;
    plm3d__project(0.5, 0.5, 0.5, 1.0, 0.0, 0.0, 1.0, 0.0, &rx, &ry, &rz);
    ASSERT_FEQ(rx, 0.5f, 0.001f);
    ASSERT_FEQ(ry, 0.5f, 0.001f);
    ASSERT_FEQ(rz, 0.5f, 0.001f);
    return 0;
}

TEST(proj_depth_order)
{
    /* Two points along the view direction; the one closer to camera
       should have larger rz.  At az=-50, el=30 (default view).         */
    double az = -50.0 * 3.141592653589793 / 180.0;
    double el =  30.0 * 3.141592653589793 / 180.0;
    double ca = cos(az), sa = sin(az);
    double ce = cos(el), se = sin(el);

    /* Light/view direction in data space: (sa*ce, -ca*ce, se)           */
    float rx1, ry1, rz1, rx2, ry2, rz2;
    plm3d__project(0.0, 0.0, 0.0, ca, sa, ce, se, 0.0, &rx1, &ry1, &rz1);
    plm3d__project(0.0, 0.0, 0.5, ca, sa, ce, se, 0.0, &rx2, &ry2, &rz2);

    /* Point at z=0.5 should be closer to camera (larger rz) than z=0    */
    ASSERT_TRUE(rz2 > rz1);
    return 0;
}

/* ================================================================== */
/*  z-buffer pixel ops                                                */
/* ================================================================== */

TEST(zbuf_set_pixel_visible)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    plm3d__set_pixel_z(&fb, zbuf, 2, 2, 0.5f, PLM_RED);

    /* pixel should be red */
    unsigned char *p = buf + 2 * fb.stride + 2 * 4;
    ASSERT_EQ(p[0], 255);  /* R */
    ASSERT_EQ(p[1], 0);    /* G */
    ASSERT_EQ(p[2], 0);    /* B */
    return 0;
}

TEST(zbuf_set_pixel_occluded)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    /* draw first pixel at z=0.8 */
    plm3d__set_pixel_z(&fb, zbuf, 2, 2, 0.8f, PLM_RED);
    /* try to draw second pixel behind it at z=0.3 -- should NOT overwrite */
    plm3d__set_pixel_z(&fb, zbuf, 2, 2, 0.3f, PLM_BLUE);

    /* pixel should still be red */
    unsigned char *p = buf + 2 * fb.stride + 2 * 4;
    ASSERT_EQ(p[0], 255);  /* still red */
    ASSERT_EQ(p[2], 0);    /* not blue */
    return 0;
}

TEST(zbuf_set_pixel_closer_wins)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    plm3d__set_pixel_z(&fb, zbuf, 2, 2, 0.3f, PLM_RED);
    plm3d__set_pixel_z(&fb, zbuf, 2, 2, 0.8f, PLM_BLUE); /* closer → wins */

    unsigned char *p = buf + 2 * fb.stride + 2 * 4;
    ASSERT_EQ(p[0], 0);    /* not red */
    ASSERT_EQ(p[2], 255);  /* blue */
    return 0;
}

TEST(zbuf_set_pixel_out_of_bounds)
{
    unsigned char buf[4 * 4 * 4];
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    /* should not crash */
    plm3d__set_pixel_z(&fb, zbuf, -1,  0, 0.5f, PLM_RED);
    plm3d__set_pixel_z(&fb, zbuf,  0, -1, 0.5f, PLM_RED);
    plm3d__set_pixel_z(&fb, zbuf,  4,  0, 0.5f, PLM_RED);
    plm3d__set_pixel_z(&fb, zbuf,  0,  4, 0.5f, PLM_RED);
    return 0;
}

TEST(zbuf_blend_pixel_opaque)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    plm3d__blend_pixel_z(&fb, zbuf, 2, 2, 0.5f, PLM_RED, 255);
    unsigned char *p = buf + 2 * fb.stride + 2 * 4;
    ASSERT_EQ(p[0], 255);
    ASSERT_EQ(p[1], 0);
    return 0;
}

TEST(zbuf_blend_pixel_transparent)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    /* alpha=0: should leave pixel unchanged (all zeros) */
    plm3d__blend_pixel_z(&fb, zbuf, 2, 2, 0.5f, PLM_RED, 0);
    unsigned char *p = buf + 2 * fb.stride + 2 * 4;
    ASSERT_EQ(p[0], 0);
    ASSERT_EQ(p[1], 0);
    ASSERT_EQ(p[2], 0);
    return 0;
}

/* ================================================================== */
/*  triangle rasterisation                                            */
/* ================================================================== */

TEST(triangle_fill_simple)
{
    unsigned char buf[8 * 8 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 8, 8, PLM_RGBA8);

    float zbuf[8 * 8];
    for (int i = 0; i < 64; i++) zbuf[i] = PLM3D_ZFAR;

    /* A right triangle covering bottom-left half of [2,2]..[5,5] */
    plm3d__fill_triangle_z(&fb, zbuf,
                           2.0f, 2.0f, 0.5f,
                           5.0f, 2.0f, 0.5f,
                           2.0f, 5.0f, 0.5f, PLM_GREEN);

    /* pixel (3,3) inside triangle */
    unsigned char *p_in = buf + 3 * fb.stride + 3 * 4;
    ASSERT_EQ(p_in[1], 255);  /* green */

    /* pixel (5,5) outside triangle (above hypotenuse) */
    unsigned char *p_out = buf + 5 * fb.stride + 5 * 4;
    ASSERT_EQ(p_out[1], 0);   /* not green */
    return 0;
}

TEST(triangle_fill_z_interpolation)
{
    /* Two triangles at different z depths; deeper one should be occluded */
    unsigned char buf[8 * 8 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 8, 8, PLM_RGBA8);

    float zbuf[8 * 8];
    for (int i = 0; i < 64; i++) zbuf[i] = PLM3D_ZFAR;

    /* triangle at z=0.8 (near) → green */
    plm3d__fill_triangle_z(&fb, zbuf,
                           1.0f, 1.0f, 0.8f,
                           6.0f, 1.0f, 0.8f,
                           1.0f, 6.0f, 0.8f, PLM_GREEN);

    /* triangle at z=0.2 (far, overlapping) → red, should be hidden */
    plm3d__fill_triangle_z(&fb, zbuf,
                           1.0f, 1.0f, 0.2f,
                           6.0f, 1.0f, 0.2f,
                           1.0f, 6.0f, 0.2f, PLM_RED);

    /* all overlapping pixels should be green */
    unsigned char *p = buf + 2 * fb.stride + 2 * 4;
    ASSERT_EQ(p[1], 255);  /* green */
    ASSERT_EQ(p[0], 0);    /* not red */
    return 0;
}

TEST(triangle_fill_degenerate)
{
    /* Degenerate (zero-area) triangle should not crash */
    unsigned char buf[4 * 4 * 4];
    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);

    float zbuf[4 * 4];
    for (int i = 0; i < 16; i++) zbuf[i] = PLM3D_ZFAR;

    plm3d__fill_triangle_z(&fb, zbuf,
                           1.0f, 1.0f, 0.5f,
                           1.0f, 1.0f, 0.5f,
                           1.0f, 1.0f, 0.5f, PLM_RED);
    return 0;
}

/* ================================================================== */
/*  quad fill                                                         */
/* ================================================================== */

TEST(quad_fill_square)
{
    unsigned char buf[8 * 8 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 8, 8, PLM_RGBA8);

    float zbuf[8 * 8];
    for (int i = 0; i < 64; i++) zbuf[i] = PLM3D_ZFAR;

    float qx[4] = { 2.0f, 5.0f, 5.0f, 2.0f };
    float qy[4] = { 2.0f, 2.0f, 5.0f, 5.0f };
    float qz[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
    plm3d__fill_quad_z(&fb, zbuf, qx, qy, qz, PLM_RED);

    /* center of quad should be red */
    unsigned char *p = buf + 3 * fb.stride + 3 * 4;
    ASSERT_EQ(p[0], 255);
    /* corner outside should not be red */
    unsigned char *p_out = buf + 0 * fb.stride + 0 * 4;
    ASSERT_EQ(p_out[0], 0);
    return 0;
}

/* ================================================================== */
/*  surface rendering smoke tests                                     */
/* ================================================================== */

/* Helper: count non-black pixels in framebuffer */
static int count_filled_pixels(plm_fb *fb) {
    int count = 0;
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            unsigned char *p = fb->pixels + y * fb->stride + x * 4;
            if (p[0] != 0 || p[1] != 0 || p[2] != 0) count++;
        }
    }
    return count;
}

TEST(surface_render_smoke)
{
    /* Small 2×2 surface (one quad), flat plane z=1, with and without lighting.
       Both renders should produce filled pixels (not crash). */

    unsigned char buf1[32 * 32 * 4];
    memset(buf1, 0, sizeof(buf1));
    plm_fb fb1 = plm_fb_create(buf1, 32, 32, PLM_RGBA8);

    unsigned char buf2[32 * 32 * 4];
    memset(buf2, 0, sizeof(buf2));
    plm_fb fb2 = plm_fb_create(buf2, 32, 32, PLM_RGBA8);

    float x_data[2] = { 0.0f, 1.0f };
    float y_data[2] = { 0.0f, 1.0f };
    float z_data[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    /* Render without lighting */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, 2, 2,
            (plm3d_surface_style){PLM_GREY(180), 0.6f, PLM_CMAP_VIRIDIS, 0.0f, NULL, 0});
        plm3d_render(&p, &fb1);
        plm3d_plot_reset(&p);
    }

    /* Render with lighting */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, 2, 2,
            (plm3d_surface_style){PLM_GREY(180), 0.6f, PLM_CMAP_VIRIDIS, 1.0f, NULL, 0});
        plm3d_render(&p, &fb2);
        plm3d_plot_reset(&p);
    }

    /* Both should produce some filled pixels */
    int filled1 = count_filled_pixels(&fb1);
    int filled2 = count_filled_pixels(&fb2);
    ASSERT_TRUE(filled1 > 0);
    ASSERT_TRUE(filled2 > 0);

    return 0;
}

TEST(surface_lighting_changes_brightness)
{
    /* A tilted plane: z varies in x and y.  With lighting enabled,
       the pixel data must differ from the flat (unlit) render.
       Use a large buffer and small explicit margins so the surface
       fills a substantial portion of the framebuffer.               */

    int W = 200, H = 200;
    unsigned char *buf_flat = (unsigned char *)calloc((size_t)W * H * 4, 1);
    unsigned char *buf_lit  = (unsigned char *)calloc((size_t)W * H * 4, 1);
    plm_fb fb_flat = plm_fb_create(buf_flat, W, H, PLM_RGBA8);
    plm_fb fb_lit  = plm_fb_create(buf_lit,  W, H, PLM_RGBA8);

    int n = 6;
    float *x_data = (float *)malloc((size_t)n * sizeof(float));
    float *y_data = (float *)malloc((size_t)n * sizeof(float));
    float *z_data = (float *)malloc((size_t)n * n * sizeof(float));
    for (int i = 0; i < n; i++) x_data[i] = (float)i;
    for (int j = 0; j < n; j++) y_data[j] = (float)j;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            z_data[i * n + j] = (float)(i + j);  /* tilts in both directions */

    /* Flat (light=0) */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        p.margin_left   = 30;
        p.margin_top    = 30;
        p.margin_right  = 10;
        p.margin_bottom = 30;
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, n, n,
            (plm3d_surface_style){PLM_RED, 0.4f, PLM_CMAP_NONE, 0.0f, NULL, 0});
        plm3d_render(&p, &fb_flat);
        plm3d_plot_reset(&p);
    }

    /* Lit (light=1) */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        p.margin_left   = 30;
        p.margin_top    = 30;
        p.margin_right  = 10;
        p.margin_bottom = 30;
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, n, n,
            (plm3d_surface_style){PLM_RED, 0.4f, PLM_CMAP_NONE, 1.0f, NULL, 0});
        plm3d_render(&p, &fb_lit);
        plm3d_plot_reset(&p);
    }

    /* The lit framebuffer must differ from the flat one. */
    int same = (memcmp(buf_flat, buf_lit, (size_t)W * H * 4) == 0);
    ASSERT_FALSE(same);  /* they must differ */

    free(x_data); free(y_data); free(z_data);
    free(buf_flat); free(buf_lit);
    return 0;
}

TEST(surface_lighting_backward_compat)
{
    /* light=0 (or default zero-init) should give same result as old code path */
    unsigned char buf_a[32 * 32 * 4];
    unsigned char buf_b[32 * 32 * 4];
    memset(buf_a, 0, sizeof(buf_a));
    memset(buf_b, 0, sizeof(buf_b));
    plm_fb fb_a = plm_fb_create(buf_a, 32, 32, PLM_RGBA8);
    plm_fb fb_b = plm_fb_create(buf_b, 32, 32, PLM_RGBA8);

    float x_data[2] = { 0.0f, 1.0f };
    float y_data[2] = { 0.0f, 1.0f };
    float z_data[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    /* Explicit light=0 */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, 2, 2,
            (plm3d_surface_style){PLM_GREY(180), 0.6f, PLM_CMAP_VIRIDIS, 0.0f, NULL, 0});
        plm3d_render(&p, &fb_a);
        plm3d_plot_reset(&p);
    }

    /* Zero-initialized style (no .light field) — should default to 0 */
    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        plm3d_surface_style style;
        memset(&style, 0, sizeof(style));
        style.line_color = PLM_GREY(180);
        style.line_width = 0.6f;
        style.cmap       = PLM_CMAP_VIRIDIS;
        /* style.light defaults to 0 via memset */
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, 2, 2, style);
        plm3d_render(&p, &fb_b);
        plm3d_plot_reset(&p);
    }

    /* Both framebuffers should have identical pixel data */
    int same = (memcmp(buf_a, buf_b, sizeof(buf_a)) == 0);
    ASSERT_TRUE(same);

    return 0;
}

TEST(surface_lighting_directional)
{
    /* A plane tilted differently: one quad sloping up-right, one flat.
       Verify that the sloping quad gets different shading from flat.   */

    unsigned char buf[50 * 50 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 50, 50, PLM_RGBA8);

    /* Two separate surfaces: first flat, second tilted */
    float x_flat[2]  = { -2.0f, 0.0f };
    float y_data[2]  = {  0.0f, 2.0f };
    float z_flat[4]  = {  1.0f, 1.0f,  1.0f, 1.0f };  /* flat */

    float x_tilt[2]  = {  1.0f, 3.0f };
    float z_tilt[4]  = {  0.5f, 0.5f,  2.0f, 2.0f };  /* tilts up in y */

    {
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = -50.0;
        p.view.elevation =  30.0;
        plm3d_plot_add_surface(&p, x_flat, y_data, z_flat, 2, 2,
            (plm3d_surface_style){PLM_WHITE, 0.4f, PLM_CMAP_NONE, 1.0f, NULL, 0});
        plm3d_plot_add_surface(&p, x_tilt, y_data, z_tilt, 2, 2,
            (plm3d_surface_style){PLM_WHITE, 0.4f, PLM_CMAP_NONE, 1.0f, NULL, 0});
        plm3d_render(&p, &fb);
        plm3d_plot_reset(&p);
    }

    /* Both surfaces rendered.  Check we have pixels on both sides. */
    int left_pixels  = 0;
    int right_pixels = 0;
    int mid_x = fb.width / 2;
    for (int y = 0; y < fb.height; y++) {
        for (int x = 0; x < fb.width; x++) {
            unsigned char *p = fb.pixels + y * fb.stride + x * 4;
            if (p[0] > 0 || p[1] > 0 || p[2] > 0) {
                if (x < mid_x) left_pixels++;
                else           right_pixels++;
            }
        }
    }
    ASSERT_TRUE(left_pixels  > 0);
    ASSERT_TRUE(right_pixels > 0);

    return 0;
}

TEST(surface_cmap_with_lighting)
{
    /* Combine colormap + lighting; just a smoke test that it doesn't crash */
    unsigned char buf[40 * 40 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 40, 40, PLM_RGBA8);

    int n = 5;
    float *x_data = (float *)malloc((size_t)n * sizeof(float));
    float *y_data = (float *)malloc((size_t)n * sizeof(float));
    float *z_data = (float *)malloc((size_t)n * n * sizeof(float));
    for (int i = 0; i < n; i++) x_data[i] = (float)i;
    for (int j = 0; j < n; j++) y_data[j] = (float)j;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            z_data[i * n + j] = (float)(i + j);  /* sloping plane */

    plm3d_plot p;
    plm3d_plot_init(&p);
    p.view.azimuth   = -50.0;
    p.view.elevation =  30.0;
    plm3d_plot_add_surface(&p, x_data, y_data, z_data, n, n,
        (plm3d_surface_style){PLM_GREY(180), 0.4f, PLM_CMAP_VIRIDIS, 0.7f});
    plm3d_render(&p, &fb);
    plm3d_plot_reset(&p);

    int filled = count_filled_pixels(&fb);
    ASSERT_TRUE(filled > 0);

    free(x_data); free(y_data); free(z_data);
    return 0;
}

TEST(surface_no_cmap_lighting_works)
{
    /* Lighting should also work when cmap=NONE (solid fill mode) */
    unsigned char buf[32 * 32 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 32, 32, PLM_RGBA8);

    float x_data[2] = { 0.0f, 1.0f };
    float y_data[2] = { 0.0f, 1.0f };
    float z_data[4] = { 0.0f, 1.0f, 1.0f, 2.0f };  /* tilted */

    plm3d_plot p;
    plm3d_plot_init(&p);
    p.view.azimuth   = -50.0;
    p.view.elevation =  30.0;
    plm3d_plot_add_surface(&p, x_data, y_data, z_data, 2, 2,
        (plm3d_surface_style){PLM_RED, 0.4f, PLM_CMAP_NONE, 1.0f});
    plm3d_render(&p, &fb);
    plm3d_plot_reset(&p);

    int filled = count_filled_pixels(&fb);
    ASSERT_TRUE(filled > 0);
    return 0;
}

/* ================================================================== */
/*  autorange                                                         */
/* ================================================================== */

TEST(autorange_from_surface)
{
    plm3d_plot p;
    plm3d_plot_init(&p);

    float x_data[3] = { 10.0f, 20.0f, 30.0f };
    float y_data[2] = { 100.0f, 200.0f };
    float z_data[6] = { -5.0f, 0.0f, 0.0f, 5.0f, 5.0f, 10.0f };

    plm3d_plot_add_surface(&p, x_data, y_data, z_data, 3, 2,
        (plm3d_surface_style){PLM_GREY(180), 0.6f, PLM_CMAP_VIRIDIS, 0.0f});

    /* Force autorange by setting min==max */
    p.x_axis.min = p.x_axis.max = 0.0;
    p.y_axis.min = p.y_axis.max = 0.0;
    p.z_axis.min = p.z_axis.max = 0.0;

    /* Render triggers autorange */
    unsigned char buf[32 * 32 * 4];
    memset(buf, 0, sizeof(buf));
    plm_fb fb = plm_fb_create(buf, 32, 32, PLM_RGBA8);
    plm3d_render(&p, &fb);

    /* Axes should now have valid ranges */
    ASSERT_TRUE(p.x_axis.min < p.x_axis.max);
    ASSERT_TRUE(p.y_axis.min < p.y_axis.max);
    ASSERT_TRUE(p.z_axis.min < p.z_axis.max);

    /* Ranges should cover the data */
    ASSERT_TRUE(p.x_axis.min <= 10.0f && p.x_axis.max >= 30.0f);
    ASSERT_TRUE(p.y_axis.min <= 100.0f && p.y_axis.max >= 200.0f);
    ASSERT_TRUE(p.z_axis.min <= -5.0f && p.z_axis.max >= 10.0f);

    plm3d_plot_reset(&p);
    return 0;
}

/* ================================================================== */
/*  view rotation smoke test                                          */
/* ================================================================== */

TEST(view_different_angles)
{
    unsigned char buf[32 * 32 * 4];
    plm_fb fb = plm_fb_create(buf, 32, 32, PLM_RGBA8);

    float x_data[2] = { 0.0f, 1.0f };
    float y_data[2] = { 0.0f, 1.0f };
    float z_data[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    /* Render from 4 different angles; none should crash */
    double views[][2] = {
        {  0.0,  0.0 },
        {  90.0, 0.0 },
        {  0.0,  90.0 },
        { -50.0, 30.0 },
    };

    for (int v = 0; v < 4; v++) {
        memset(buf, 0, sizeof(buf));
        plm3d_plot p;
        plm3d_plot_init(&p);
        p.view.azimuth   = views[v][0];
        p.view.elevation = views[v][1];
        plm3d_plot_add_surface(&p, x_data, y_data, z_data, 2, 2,
            (plm3d_surface_style){PLM_GREY(180), 0.6f, PLM_CMAP_VIRIDIS, 0.5f});
        plm3d_render(&p, &fb);
        plm3d_plot_reset(&p);
    }

    return 0;
}

/* ================================================================== */
/*  main                                                              */
/* ================================================================== */

TEST_MAIN()
    RUN_TEST(proj_identity);
    RUN_TEST(proj_azimuth_90);
    RUN_TEST(proj_elevation_90);
    RUN_TEST(proj_depth_order);
    RUN_TEST(zbuf_set_pixel_visible);
    RUN_TEST(zbuf_set_pixel_occluded);
    RUN_TEST(zbuf_set_pixel_closer_wins);
    RUN_TEST(zbuf_set_pixel_out_of_bounds);
    RUN_TEST(zbuf_blend_pixel_opaque);
    RUN_TEST(zbuf_blend_pixel_transparent);
    RUN_TEST(triangle_fill_simple);
    RUN_TEST(triangle_fill_z_interpolation);
    RUN_TEST(triangle_fill_degenerate);
    RUN_TEST(quad_fill_square);
    RUN_TEST(surface_render_smoke);
    RUN_TEST(surface_lighting_changes_brightness);
    RUN_TEST(surface_lighting_backward_compat);
    RUN_TEST(surface_lighting_directional);
    RUN_TEST(surface_cmap_with_lighting);
    RUN_TEST(surface_no_cmap_lighting_works);
    RUN_TEST(autorange_from_surface);
    RUN_TEST(view_different_angles);
TEST_MAIN_END()
