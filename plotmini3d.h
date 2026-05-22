/*
  plotmini3d.h  --  prototype  --  public domain

  Basic 3D plotting layered on top of plotmini.h.
  Orthographic projection + z-buffer.  Non-interactive.

  Requires plotmini.h to be included first.

  ======  USAGE  ======
  1. In *one* C file:

        #define PLOTMINI_IMPLEMENTATION
        #define PLOTMINI3D_IMPLEMENTATION
        #include "plotmini.h"
        #include "plotmini3d.h"

  2. Everywhere else just:

        #include "plotmini.h"
        #include "plotmini3d.h"
*/

#ifndef PLOTMINI3D_H
#define PLOTMINI3D_H

/* Include plotmini.h for its types, but prevent its implementation
   from being re-expanded if PLOTMINI_IMPLEMENTATION is already defined. */
#ifdef PLOTMINI_IMPLEMENTATION
  #define PLOTMINI3D_TMP_IMPL
  #undef PLOTMINI_IMPLEMENTATION
#endif
#include "plotmini.h"
#ifdef PLOTMINI3D_TMP_IMPL
  #define PLOTMINI_IMPLEMENTATION
  #undef PLOTMINI3D_TMP_IMPL
#endif

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/*  PUBLIC TYPES                                                      */
/* ================================================================== */

/* ---- camera / view -------------------------------------------------*/
typedef struct plm3d_view {
    double azimuth;    /* degrees, horizontal rotation (default -45)    */
    double elevation;  /* degrees, tilt above horizontal (default 25)  */
} plm3d_view;

/* ---- 3D scatter ----------------------------------------------------*/
typedef struct plm3d_scatter_style {
    plm_color color;
    float     radius;      /* pixel radius                             */
} plm3d_scatter_style;

typedef struct plm3d_scatter_series {
    const float          *x_data;
    const float          *y_data;
    const float          *z_data;
    int                   count;
    plm3d_scatter_style   style;
} plm3d_scatter_series;

/* ---- 3D line -------------------------------------------------------*/
typedef struct plm3d_line_style {
    plm_color color;
    float     width;       /* pixel width (1.0 = hairline)             */
} plm3d_line_style;

typedef struct plm3d_line_series {
    const float       *x_data;
    const float       *y_data;
    const float       *z_data;
    int                count;
    plm3d_line_style   style;
} plm3d_line_series;

/* ---- surface (wireframe) -------------------------------------------*/
typedef struct plm3d_surface_style {
    plm_color line_color;
    float     line_width;
    plm_cmap  cmap;        /* colormap for surface fill; PLM_CMAP_NONE = solid fill derived from line_color */
} plm3d_surface_style;

typedef struct plm3d_surface_series {
    const float          *x_data;   /* length nx                        */
    const float          *y_data;   /* length ny                        */
    const float          *z_data;   /* length nx*ny, row-major:         */
    int                   nx;       /*   z[i*ny + j] = f(x[i], y[j])   */
    int                   ny;
    plm3d_surface_style   style;
} plm3d_surface_series;

/* ---- 3D plot -------------------------------------------------------*/
typedef struct plm3d_plot {
    /* axes */
    plm_axis     x_axis;
    plm_axis     y_axis;
    plm_axis     z_axis;

    /* camera */
    plm3d_view   view;

    /* layout (pixels) -- -1 means auto-compute */
    int          margin_left;
    int          margin_top;
    int          margin_right;
    int          margin_bottom;

    /* labels */
    const char  *title;
    const char  *x_label;
    const char  *y_label;
    const char  *z_label;

    /* series (filled by plm3d_plot_add_*()) */
    plm3d_scatter_series *scatters;
    int                   scatter_count;
    int                   scatter_cap;

    plm3d_line_series    *lines;
    int                   line_count;
    int                   line_cap;

    plm3d_surface_series *surfaces;
    int                   surface_count;
    int                   surface_cap;

    /* internal state (read-only) */
    plm_irect    plot_area;   /* pixel rect of the 3D data region      */
    int          fb_w;        /* framebuffer width at last render       */
    int          fb_h;        /* framebuffer height at last render      */
} plm3d_plot;

/* ================================================================== */
/*  PUBLIC API  (declarations)                                        */
/* ================================================================== */

void plm3d_plot_init(plm3d_plot *p);
void plm3d_plot_reset(plm3d_plot *p);

void plm3d_plot_add_scatter(plm3d_plot *p,
                            const float *x, const float *y, const float *z,
                            int count, plm3d_scatter_style style);
void plm3d_plot_add_line(plm3d_plot *p,
                         const float *x, const float *y, const float *z,
                         int count, plm3d_line_style style);
void plm3d_plot_add_surface(plm3d_plot *p,
                            const float *x, const float *y, const float *z,
                            int nx, int ny, plm3d_surface_style style);

/* Render the 3D plot into a framebuffer.  The framebuffer must already
   be created with plm_fb_create().                                     */
void plm3d_render(plm3d_plot *p, plm_fb *fb);

#ifdef __cplusplus
}
#endif

#endif /* PLOTMINI3D_H */

/* ================================================================== */
/*  IMPLEMENTATION                                                    */
/* ================================================================== */
#ifdef PLOTMINI3D_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  internal helpers                                                  */
/* ------------------------------------------------------------------ */

#define PLM3D_ZFAR  (-1e30f)

static float plm3d__maxf(float a, float b) { return a > b ? a : b; }
static int   plm3d__maxi(int a, int b)   { return a > b ? a : b; }
static float plm3d__absf(float a)        { return a < 0 ? -a : a; }

/* ---- projection ---------------------------------------------------- */

/* Project a normalised [-0.5, 0.5]^3 point to screen space.
   Returns pixel-space (rx, ry) and depth rz.
   The caller then scales rx,ry to the actual plot area.                */
static void plm3d__project(double cx, double cy, double cz,
                           double ca, double sa,
                           double ce, double se,
                           float *rx, float *ry, float *rz) {
    /* Standard orthographic: rotate around Z by azimuth,
       then tilt by elevation.  z=up in data space.                     */
    *rx = (float)( ca * cx + sa * cy);
    *ry = (float)(-sa * se * cx + ca * se * cy + ce * cz);
    *rz = (float)( sa * ce * cx - ca * ce * cy + se * cz);
}

/* ---- depth-tested pixel ops ---------------------------------------- */

static void plm3d__set_pixel_z(plm_fb *fb, float *zbuf,
                               int x, int y, float z, plm_color c) {
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
    int idx = y * fb->width + x;
    if (z <= zbuf[idx]) return;   /* occluded */
    zbuf[idx] = z;
    if (fb->bpp == 1) {
        fb->pixels[y * fb->stride + x] =
            (unsigned char)((unsigned int)(c.r * 77 + c.g * 150 + c.b * 29) >> 8);
    } else {
        unsigned char *p = fb->pixels + y * fb->stride + x * 4;
        p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
    }
}

/* ---- depth-tested blend pixel -------------------------------------- */

static void plm3d__blend_pixel_z(plm_fb *fb, float *zbuf,
                                  int x, int y, float z,
                                  plm_color c, unsigned char alpha) {
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
    if (alpha == 0) return;
    if (alpha == 255) { plm3d__set_pixel_z(fb, zbuf, x, y, z, c); return; }
    int idx = y * fb->width + x;
    if (z <= zbuf[idx]) return;
    zbuf[idx] = z;
    if (fb->bpp == 1) {
        unsigned char *dst = fb->pixels + y * fb->stride + x;
        unsigned char lum = (unsigned char)((unsigned int)(c.r * 77 + c.g * 150 + c.b * 29) >> 8);
        unsigned int inv_a = 255 - alpha;
        *dst = (unsigned char)(((unsigned int)lum * alpha + (unsigned int)*dst * inv_a) >> 8);
    } else {
        unsigned char *dst = fb->pixels + y * fb->stride + x * 4;
        unsigned int inv_a = 255 - alpha;
        dst[0] = (unsigned char)(((unsigned int)c.r * alpha + (unsigned int)dst[0] * inv_a) >> 8);
        dst[1] = (unsigned char)(((unsigned int)c.g * alpha + (unsigned int)dst[1] * inv_a) >> 8);
        dst[2] = (unsigned char)(((unsigned int)c.b * alpha + (unsigned int)dst[2] * inv_a) >> 8);
    }
}

/* ---- depth-tested line (DDA) --------------------------------------- */

static void plm3d__line_z(plm_fb *fb, float *zbuf,
                          float x0, float y0, float z0,
                          float x1, float y1, float z1,
                          plm_color c) {
    float dx = x1 - x0, dy = y1 - y0;
    float steps = plm3d__maxf(plm3d__absf(dx), plm3d__absf(dy));
    if (steps < 1.0f) steps = 1.0f;
    float x_inc = dx / steps, y_inc = dy / steps, z_inc = (z1 - z0) / steps;
    float x = x0, y = y0, z = z0;

    for (int i = 0; i <= (int)steps; i++) {
        plm3d__set_pixel_z(fb, zbuf, (int)(x + 0.5f), (int)(y + 0.5f), z, c);
        x += x_inc; y += y_inc; z += z_inc;
    }
}

/* ---- depth-tested Wu anti-aliased line ------------------------------ */

static void plm3d__wu_line_z(plm_fb *fb, float *zbuf,
                              float x0, float y0, float z0,
                              float x1, float y1, float z1,
                              plm_color c) {
    int steep = (fabsf(y1 - y0) > fabsf(x1 - x0));
    if (steep) {
        { float t = x0; x0 = y0; y0 = t; }
        { float t = x1; x1 = y1; y1 = t; }
    }
    if (x0 > x1) {
        { float t = x0; x0 = x1; x1 = t; }
        { float t = y0; y0 = y1; y1 = t; }
        { float t = z0; z0 = z1; z1 = t; }
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float dz = z1 - z0;
    float gradient, zgrad;
    if (dx < 1e-6f) { gradient = 1.0f; zgrad = 0.0f; }
    else            { gradient = dy / dx; zgrad = dz / dx; }

    /* first endpoint */
    int   xpxl1 = (int)x0;
    float yend  = y0 + gradient * ((float)xpxl1 + 1.0f - x0);
    float zend  = z0 + zgrad    * ((float)xpxl1 + 1.0f - x0);
    float xgap  = 1.0f - (x0 - (float)xpxl1);
    {
        int   ipart = (int)yend;
        float fpart = yend - (float)ipart;
        unsigned char a1 = (unsigned char)((1.0f - fpart) * xgap * 255.0f);
        unsigned char a2 = (unsigned char)(fpart * xgap * 255.0f);
        if (steep) {
            plm3d__blend_pixel_z(fb, zbuf, ipart,     xpxl1, zend, c, a1);
            plm3d__blend_pixel_z(fb, zbuf, ipart + 1, xpxl1, zend, c, a2);
        } else {
            plm3d__blend_pixel_z(fb, zbuf, xpxl1, ipart,     zend, c, a1);
            plm3d__blend_pixel_z(fb, zbuf, xpxl1, ipart + 1, zend, c, a2);
        }
    }

    /* second endpoint */
    int   xpxl2 = (int)x1;
    float yend2 = y1 + gradient * ((float)xpxl2 - x1);
    float zend2 = z1 + zgrad    * ((float)xpxl2 - x1);
    float xgap2 = x1 - (float)xpxl2;
    if (xgap2 > 0.0f) {
        int   ipart = (int)yend2;
        float fpart = yend2 - (float)ipart;
        unsigned char a1 = (unsigned char)((1.0f - fpart) * xgap2 * 255.0f);
        unsigned char a2 = (unsigned char)(fpart * xgap2 * 255.0f);
        if (steep) {
            plm3d__blend_pixel_z(fb, zbuf, ipart,     xpxl2, zend2, c, a1);
            plm3d__blend_pixel_z(fb, zbuf, ipart + 1, xpxl2, zend2, c, a2);
        } else {
            plm3d__blend_pixel_z(fb, zbuf, xpxl2, ipart,     zend2, c, a1);
            plm3d__blend_pixel_z(fb, zbuf, xpxl2, ipart + 1, zend2, c, a2);
        }
    }

    /* main loop */
    float intery = yend + gradient;
    float interz = zend + zgrad;
    for (int x = xpxl1 + 1; x < xpxl2; x++) {
        int   ipart = (int)intery;
        float fpart = intery - (float)ipart;
        unsigned char a1 = (unsigned char)((1.0f - fpart) * 255.0f);
        unsigned char a2 = (unsigned char)(fpart * 255.0f);
        if (steep) {
            plm3d__blend_pixel_z(fb, zbuf, ipart,     x, interz, c, a1);
            plm3d__blend_pixel_z(fb, zbuf, ipart + 1, x, interz, c, a2);
        } else {
            plm3d__blend_pixel_z(fb, zbuf, x, ipart,     interz, c, a1);
            plm3d__blend_pixel_z(fb, zbuf, x, ipart + 1, interz, c, a2);
        }
        intery += gradient;
        interz += zgrad;
    }
}

/* ---- depth-tested thick line (multiple parallel passes) ------------- */

static void plm3d__thick_line_z(plm_fb *fb, float *zbuf,
                                float x0, float y0, float z0,
                                float x1, float y1, float z1,
                                float width, plm_color c) {
    if (width <= 1.0f) {
        plm3d__line_z(fb, zbuf, x0, y0, z0, x1, y1, z1, c);
        return;
    }
    /* Offset perpendicular to line direction */
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) {
        plm3d__line_z(fb, zbuf, x0, y0, z0, x1, y1, z1, c);
        return;
    }
    float nx = -dy / len, ny = dx / len;   /* perpendicular */
    float half = (width - 1.0f) * 0.5f;
    int n = (int)(half + 0.5f);

    for (int i = -n; i <= n; i++) {
        float off = (float)i;
        plm3d__line_z(fb, zbuf,
                      x0 + nx * off, y0 + ny * off, z0,
                      x1 + nx * off, y1 + ny * off, z1, c);
    }
}

/* ---- depth-tested filled circle ------------------------------------ */

static void plm3d__fill_circle_z(plm_fb *fb, float *zbuf,
                                 float cx, float cy, float cz,
                                 float r, plm_color c) {
    int min_x = (int)(cx - r - 1);
    int max_x = (int)(cx + r + 1);
    int min_y = (int)(cy - r - 1);
    int max_y = (int)(cy + r + 1);
    float r2 = r * r;

    for (int y = min_y; y <= max_y; y++) {
        float dy = (float)y - cy;
        for (int x = min_x; x <= max_x; x++) {
            float dx = (float)x - cx;
            if (dx * dx + dy * dy <= r2) {
                plm3d__set_pixel_z(fb, zbuf, x, y, cz, c);
            }
        }
    }
}

/* ---- depth-tested filled quad (barycentric triangle rasteriser) ---- */

/* Fill a triangle using barycentric coordinates with z-interpolation.  */
static void plm3d__fill_triangle_z(plm_fb *fb, float *zbuf,
                                    float x0, float y0, float z0,
                                    float x1, float y1, float z1,
                                    float x2, float y2, float z2,
                                    plm_color c) {
    float fmin_x = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    float fmin_y = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    float fmax_x = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    float fmax_y = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    int ix0 = (int)fmin_x;
    int iy0 = (int)fmin_y;
    int ix1 = (int)fmax_x;
    int iy1 = (int)fmax_y;

    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    float eps = -1e-6f;
    if (denom > -1e-12f && denom < 1e-12f) return;

    for (int y = iy0; y <= iy1; y++) {
        for (int x = ix0; x <= ix1; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;
            float w0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / denom;
            float w1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / denom;
            float w2 = 1.0f - w0 - w1;
            if (w0 >= eps && w1 >= eps && w2 >= eps) {
                float z = w0 * z0 + w1 * z1 + w2 * z2;
                plm3d__set_pixel_z(fb, zbuf, x, y, z, c);
            }
        }
    }
}

/* Fill a convex quad by splitting into two triangles.                  */
static void plm3d__fill_quad_z(plm_fb *fb, float *zbuf,
                                const float px[4], const float py[4],
                                const float pz[4], plm_color c) {
    plm3d__fill_triangle_z(fb, zbuf,
                           px[0], py[0], pz[0],
                           px[1], py[1], pz[1],
                           px[2], py[2], pz[2], c);
    plm3d__fill_triangle_z(fb, zbuf,
                           px[0], py[0], pz[0],
                           px[2], py[2], pz[2],
                           px[3], py[3], pz[3], c);
}

/* ---- data bounding-box helpers ------------------------------------- */

/* Autorange a single axis from all series data.                        */
static int plm3d__axis_autorange(plm_axis *ax, int dim,
                                 plm3d_scatter_series *sc, int sc_n,
                                 plm3d_line_series    *li, int li_n,
                                 plm3d_surface_series *su, int su_n) {
    double lo = 1e30, hi = -1e30;
    int    found = 0;

    /* helper: walk one series' dimension */
    #define CHECK_DATASET(d, n) do {               \
        if (!(d) || (n) <= 0) break;               \
        for (int _j = 0; _j < (n); _j++) {         \
            float v = (d)[_j];                     \
            if (v < lo) lo = v;                     \
            if (v > hi) hi = v;                     \
        }                                          \
        found = 1;                                  \
    } while(0)

    /* scatters */
    for (int i = 0; i < sc_n; i++) {
        if (dim == 0)       CHECK_DATASET(sc[i].x_data, sc[i].count);
        else if (dim == 1)  CHECK_DATASET(sc[i].y_data, sc[i].count);
        else                CHECK_DATASET(sc[i].z_data, sc[i].count);
    }
    /* lines */
    for (int i = 0; i < li_n; i++) {
        if (dim == 0)       CHECK_DATASET(li[i].x_data, li[i].count);
        else if (dim == 1)  CHECK_DATASET(li[i].y_data, li[i].count);
        else                CHECK_DATASET(li[i].z_data, li[i].count);
    }
    /* surfaces */
    for (int i = 0; i < su_n; i++) {
        int n = su[i].nx * su[i].ny;
        if (dim == 0)       CHECK_DATASET(su[i].x_data, su[i].nx);
        else if (dim == 1)  CHECK_DATASET(su[i].y_data, su[i].ny);
        else                CHECK_DATASET(su[i].z_data, n);
    }

    #undef CHECK_DATASET

    if (!found) return 0;

    /* add 5% padding */
    double pad = (hi - lo) * 0.05;
    if (pad <= 0.0) { pad = (lo == 0.0) ? 1.0 : fabs(lo) * 0.05; }
    if (pad <= 0.0) pad = 1.0;
    ax->min = lo - pad;
    ax->max = hi + pad;
    return 1;
}

/* ------------------------------------------------------------------ */
/*  axis rendering                                                    */
/* ------------------------------------------------------------------ */

/* Draw the three origin-frame axis lines with tick marks.              */
static void plm3d__draw_axes(plm3d_plot *p, plm_fb *fb, float *zbuf,
                             double ca, double sa, double ce, double se,
                             double plot_cx, double plot_cy, double scale) {
    plm_color axis_colors[3] = {
        {255, 80, 80, 255},    /* X: red-ish   */
        {80, 200, 80, 255},    /* Y: green-ish */
        {80, 80, 255, 255}     /* Z: blue-ish  */
    };

    double x_range = p->x_axis.max - p->x_axis.min;
    double y_range = p->y_axis.max - p->y_axis.min;
    double z_range = p->z_axis.max - p->z_axis.min;
    if (x_range <= 0.0) x_range = 1.0;
    if (y_range <= 0.0) y_range = 1.0;
    if (z_range <= 0.0) z_range = 1.0;

    /* Choose the corner of the data cube farthest from the camera.
       Test all 8 corners in normalised space; pick the one with
       smallest projected depth (rz = farthest).                        */
    double best_rz = 1e30;
    int best_cx = 0, best_cy = 0, best_cz = 0;
    for (int corner = 0; corner < 8; corner++) {
        double tcx = (corner & 1) ? 0.5 : -0.5;
        double tcy = (corner & 2) ? 0.5 : -0.5;
        double tcz = (corner & 4) ? 0.5 : -0.5;
        float rx, ry, rz;
        plm3d__project(tcx, tcy, tcz, ca, sa, ce, se, &rx, &ry, &rz);
        if (rz < best_rz) { best_rz = rz; best_cx = corner & 1;
                            best_cy = (corner >> 1) & 1;
                            best_cz = (corner >> 2) & 1; }
    }

    /* Map back to data-space corner */
    double ox = best_cx ? p->x_axis.max : p->x_axis.min;
    double oy = best_cy ? p->y_axis.max : p->y_axis.min;
    double oz = best_cz ? p->z_axis.max : p->z_axis.min;

    /* Axis endpoints: from this corner along each axis to the opposite face */
    double endpoints[3][3] = {
        {best_cx ? p->x_axis.min : p->x_axis.max, oy, oz},
        {ox, best_cy ? p->y_axis.min : p->y_axis.max, oz},
        {ox, oy, best_cz ? p->z_axis.min : p->z_axis.max}
    };

    for (int axis = 0; axis < 3; axis++) {
        double ex = endpoints[axis][0];
        double ey = endpoints[axis][1];
        double ez = endpoints[axis][2];

        /* Normalize origin and endpoint to [-0.5, 0.5] */
        double onx = (ox - p->x_axis.min) / x_range - 0.5;
        double ony = (oy - p->y_axis.min) / y_range - 0.5;
        double onz = (oz - p->z_axis.min) / z_range - 0.5;

        double enx = (ex - p->x_axis.min) / x_range - 0.5;
        double eny = (ey - p->y_axis.min) / y_range - 0.5;
        double enz = (ez - p->z_axis.min) / z_range - 0.5;

        float rx0, ry0, rz0, rx1, ry1, rz1;
        plm3d__project(onx, ony, onz, ca, sa, ce, se, &rx0, &ry0, &rz0);
        plm3d__project(enx, eny, enz, ca, sa, ce, se, &rx1, &ry1, &rz1);

        float px0 = (float)(plot_cx + rx0 * scale);
        float py0 = (float)(plot_cy - ry0 * scale);   /* Y flip */
        float px1 = (float)(plot_cx + rx1 * scale);
        float py1 = (float)(plot_cy - ry1 * scale);

        plm3d__thick_line_z(fb, zbuf, px0, py0, rz0, px1, py1, rz1,
                            2.0f, axis_colors[axis]);

        /* Tick marks along the axis */
        double range = (axis == 0) ? x_range :
                       (axis == 1) ? y_range : z_range;
        double lo    = (axis == 0) ? p->x_axis.min :
                       (axis == 1) ? p->y_axis.min : p->z_axis.min;

        /* Simple nice-step ticks */
        double rough = range / 5.0;
        double exp   = pow(10.0, floor(log10(rough)));
        double mant  = rough / exp;
        double nice;
        if      (mant <= 1.5) nice = 1.0;
        else if (mant <= 3.0) nice = 2.0;
        else if (mant <= 7.0) nice = 5.0;
        else                  nice = 10.0;
        double step = nice * exp;
        if (step <= 0.0) step = 1.0;

        double tick_start = ceil(lo / step) * step;
        double tick_len   = range * 0.02;  /* 2% of axis range */

        for (double v = tick_start; v <= lo + range; v += step) {
            double tx, ty, tz;
            tx = (axis == 0) ? v : ox;
            ty = (axis == 1) ? v : oy;
            tz = (axis == 2) ? v : oz;

            /* Tick direction in data space: -Z for X/Y axes, -X for Z axis */
            double tdx = (axis == 2) ? -tick_len : 0.0;
            double tdy = 0.0;
            double tdz = (axis == 2) ? 0.0 : -tick_len;

            double tnx0 = (tx - p->x_axis.min) / x_range - 0.5;
            double tny0 = (ty - p->y_axis.min) / y_range - 0.5;
            double tnz0 = (tz - p->z_axis.min) / z_range - 0.5;

            double tnx1 = ((tx + tdx) - p->x_axis.min) / x_range - 0.5;
            double tny1 = ((ty + tdy) - p->y_axis.min) / y_range - 0.5;
            double tnz1 = ((tz + tdz) - p->z_axis.min) / z_range - 0.5;

            float trx0, try0, trz0, trx1, try1, trz1;
            plm3d__project(tnx0, tny0, tnz0, ca, sa, ce, se, &trx0, &try0, &trz0);
            plm3d__project(tnx1, tny1, tnz1, ca, sa, ce, se, &trx1, &try1, &trz1);

            float tpx0 = (float)(plot_cx + trx0 * scale);
            float tpy0 = (float)(plot_cy - try0 * scale);
            float tpx1 = (float)(plot_cx + trx1 * scale);
            float tpy1 = (float)(plot_cy - try1 * scale);

            plm3d__line_z(fb, zbuf, tpx0, tpy0, trz0, tpx1, tpy1, trz1,
                          axis_colors[axis]);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  series rendering                                                  */
/* ------------------------------------------------------------------ */

static void plm3d__render_scatters(plm3d_plot *p, plm_fb *fb, float *zbuf,
                                   double ca, double sa, double ce, double se,
                                   double plot_cx, double plot_cy, double scale,
                                   double x_range, double y_range, double z_range) {
    for (int si = 0; si < p->scatter_count; si++) {
        plm3d_scatter_series *ss = &p->scatters[si];
        for (int i = 0; i < ss->count; i++) {
            double nx = (ss->x_data[i] - p->x_axis.min) / x_range - 0.5;
            double ny = (ss->y_data[i] - p->y_axis.min) / y_range - 0.5;
            double nz = (ss->z_data[i] - p->z_axis.min) / z_range - 0.5;

            float rx, ry, rz;
            plm3d__project(nx, ny, nz, ca, sa, ce, se, &rx, &ry, &rz);

            float px = (float)(plot_cx + rx * scale);
            float py = (float)(plot_cy - ry * scale);

            plm3d__fill_circle_z(fb, zbuf, px, py, rz,
                                 ss->style.radius, ss->style.color);
        }
    }
}

static void plm3d__render_lines(plm3d_plot *p, plm_fb *fb, float *zbuf,
                                double ca, double sa, double ce, double se,
                                double plot_cx, double plot_cy, double scale,
                                double x_range, double y_range, double z_range) {
    for (int si = 0; si < p->line_count; si++) {
        plm3d_line_series *ls = &p->lines[si];
        if (ls->count < 2) continue;

        /* Project first point */
        double nx0 = (ls->x_data[0] - p->x_axis.min) / x_range - 0.5;
        double ny0 = (ls->y_data[0] - p->y_axis.min) / y_range - 0.5;
        double nz0 = (ls->z_data[0] - p->z_axis.min) / z_range - 0.5;
        float rx0, ry0, rz0;
        plm3d__project(nx0, ny0, nz0, ca, sa, ce, se, &rx0, &ry0, &rz0);
        float px0 = (float)(plot_cx + rx0 * scale);
        float py0 = (float)(plot_cy - ry0 * scale);

        for (int i = 1; i < ls->count; i++) {
            double nx1 = (ls->x_data[i] - p->x_axis.min) / x_range - 0.5;
            double ny1 = (ls->y_data[i] - p->y_axis.min) / y_range - 0.5;
            double nz1 = (ls->z_data[i] - p->z_axis.min) / z_range - 0.5;
            float rx1, ry1, rz1;
            plm3d__project(nx1, ny1, nz1, ca, sa, ce, se, &rx1, &ry1, &rz1);
            float px1 = (float)(plot_cx + rx1 * scale);
            float py1 = (float)(plot_cy - ry1 * scale);

            if (ls->style.width <= 1.0f)
                plm3d__wu_line_z(fb, zbuf, px0, py0, rz0, px1, py1, rz1,
                                 ls->style.color);
            else
                plm3d__thick_line_z(fb, zbuf, px0, py0, rz0, px1, py1, rz1,
                                    ls->style.width, ls->style.color);

            px0 = px1; py0 = py1; rz0 = rz1;
        }
    }
}

static void plm3d__render_surfaces(plm3d_plot *p, plm_fb *fb, float *zbuf,
                                   double ca, double sa, double ce, double se,
                                   double plot_cx, double plot_cy, double scale,
                                   double x_range, double y_range, double z_range) {
    for (int si = 0; si < p->surface_count; si++) {
        plm3d_surface_series *ss = &p->surfaces[si];
        int nx = ss->nx, ny = ss->ny;
        if (nx < 2 || ny < 2) continue;

        /* Pre-project all grid points */
        /* We need nx*ny projected points. Use stack for small grids,
           heap for large ones. For prototype, allocate on heap.        */
        float *px = (float *)malloc((size_t)nx * ny * sizeof(float));
        float *py = (float *)malloc((size_t)nx * ny * sizeof(float));
        float *pz = (float *)malloc((size_t)nx * ny * sizeof(float));
        if (!px || !py || !pz) {
            free(px); free(py); free(pz);
            continue;
        }

        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                int idx = i * ny + j;
                double nxv = (ss->x_data[i] - p->x_axis.min) / x_range - 0.5;
                double nyv = (ss->y_data[j] - p->y_axis.min) / y_range - 0.5;
                double nzv = (ss->z_data[idx] - p->z_axis.min) / z_range - 0.5;

                float rx, ry, rz;
                plm3d__project(nxv, nyv, nzv, ca, sa, ce, se, &rx, &ry, &rz);
                px[idx] = (float)(plot_cx + rx * scale);
                py[idx] = (float)(plot_cy - ry * scale);
                pz[idx] = rz;
            }
        }

        /* ---- fill quads for proper occlusion (z-buffer) ---- */
        {
            int use_cmap = (ss->style.cmap != PLM_CMAP_NONE);
            plm_color solid_fill_c;
            if (!use_cmap) {
                int fr = ss->style.line_color.r + (255 - ss->style.line_color.r) * 2 / 3;
                int fg = ss->style.line_color.g + (255 - ss->style.line_color.g) * 2 / 3;
                int fb2 = ss->style.line_color.b + (255 - ss->style.line_color.b) * 2 / 3;
                solid_fill_c = PLM_RGBA(fr, fg, fb2, 255);
            }

            double z_data_min = p->z_axis.min;
            double z_data_range = z_range;

            for (int i = 0; i < nx - 1; i++) {
                for (int j = 0; j < ny - 1; j++) {
                    int i00 = i * ny + j;
                    int i10 = (i + 1) * ny + j;
                    int i11 = (i + 1) * ny + (j + 1);
                    int i01 = i * ny + (j + 1);
                    float qx[4] = { px[i00], px[i10], px[i11], px[i01] };
                    float qy[4] = { py[i00], py[i10], py[i11], py[i01] };
                    float qz[4] = { pz[i00], pz[i10], pz[i11], pz[i01] };

                    plm_color fill_c;
                    if (use_cmap) {
                        float z_avg = 0.25f * (ss->z_data[i00] + ss->z_data[i10] +
                                               ss->z_data[i11] + ss->z_data[i01]);
                        float t;
                        if (z_data_range > 0.0)
                            t = (float)((z_avg - z_data_min) / z_data_range);
                        else
                            t = 0.5f;
                        fill_c = plm_cmap_lookup(t, ss->style.cmap);
                    } else {
                        fill_c = solid_fill_c;
                    }
                    plm3d__fill_quad_z(fb, zbuf, qx, qy, qz, fill_c);
                }
            }
        }

        /* Draw horizontal lines (constant y, varying x) */
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx - 1; i++) {
                int idx0 = i * ny + j;
                int idx1 = (i + 1) * ny + j;
                plm3d__thick_line_z(fb, zbuf,
                                    px[idx0], py[idx0], pz[idx0],
                                    px[idx1], py[idx1], pz[idx1],
                                    ss->style.line_width,
                                    ss->style.line_color);
            }
        }

        /* Draw vertical lines (constant x, varying y) */
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny - 1; j++) {
                int idx0 = i * ny + j;
                int idx1 = i * ny + (j + 1);
                plm3d__thick_line_z(fb, zbuf,
                                    px[idx0], py[idx0], pz[idx0],
                                    px[idx1], py[idx1], pz[idx1],
                                    ss->style.line_width,
                                    ss->style.line_color);
            }
        }

        free(px); free(py); free(pz);
    }
}

/* ------------------------------------------------------------------ */
/*  main render                                                       */
/* ------------------------------------------------------------------ */

void plm3d_render(plm3d_plot *p, plm_fb *fb) {
    const int S = PLOTMINI_TEXT_SCALE;

    /* ---- 1. auto-range axes ---- */
    if (p->x_axis.min == p->x_axis.max)
        plm3d__axis_autorange(&p->x_axis, 0,
                              p->scatters, p->scatter_count,
                              p->lines,    p->line_count,
                              p->surfaces, p->surface_count);
    if (p->y_axis.min == p->y_axis.max)
        plm3d__axis_autorange(&p->y_axis, 1,
                              p->scatters, p->scatter_count,
                              p->lines,    p->line_count,
                              p->surfaces, p->surface_count);
    if (p->z_axis.min == p->z_axis.max)
        plm3d__axis_autorange(&p->z_axis, 2,
                              p->scatters, p->scatter_count,
                              p->lines,    p->line_count,
                              p->surfaces, p->surface_count);

    /* ---- 2. compute margins ---- */
    int ml = p->margin_left   >= 0 ? p->margin_left   : 40 * S;
    int mt = p->margin_top    >= 0 ? p->margin_top    : 30 * S;
    int mr = p->margin_right  >= 0 ? p->margin_right  : 20 * S;
    int mb = p->margin_bottom >= 0 ? p->margin_bottom : 40 * S;

    /* Title */
    int title_h = 0;
    if (p->title && p->title[0]) {
        int th = plm_text_height();
        title_h = th + 6 * S;
        mt = plm3d__maxi(mt, title_h);
    }

    /* ---- 3. compute plot_area ---- */
    p->plot_area.x0 = ml;
    p->plot_area.y0 = mt;
    p->plot_area.x1 = fb->width  - mr;
    p->plot_area.y1 = fb->height - mb;
    p->fb_w = fb->width;
    p->fb_h = fb->height;

    if (p->plot_area.x1 <= p->plot_area.x0) p->plot_area.x1 = p->plot_area.x0 + 1;
    if (p->plot_area.y1 <= p->plot_area.y0) p->plot_area.y1 = p->plot_area.y0 + 1;

    int pa_w = p->plot_area.x1 - p->plot_area.x0;
    int pa_h = p->plot_area.y1 - p->plot_area.y0;
    double plot_cx = (p->plot_area.x0 + p->plot_area.x1) * 0.5;
    double plot_cy = (p->plot_area.y0 + p->plot_area.y1) * 0.5;

    /* ---- 4. clear framebuffer ---- */
    plm_fb_clear(fb, PLM_BG_COLOR);

    /* ---- 5. allocate and clear z-buffer ---- */
    int npixels = fb->width * fb->height;
    float *zbuf = (float *)malloc((size_t)npixels * sizeof(float));
    if (!zbuf) return;
    for (int i = 0; i < npixels; i++) zbuf[i] = PLM3D_ZFAR;

    /* ---- 6. set up projection ---- */
    double az = p->view.azimuth   * 3.141592653589793 / 180.0;
    double el = p->view.elevation * 3.141592653589793 / 180.0;
    double ca = cos(az), sa = sin(az);
    double ce = cos(el), se = sin(el);

    double x_range = p->x_axis.max - p->x_axis.min;
    double y_range = p->y_axis.max - p->y_axis.min;
    double z_range = p->z_axis.max - p->z_axis.min;
    if (x_range <= 0.0) x_range = 1.0;
    if (y_range <= 0.0) y_range = 1.0;
    if (z_range <= 0.0) z_range = 1.0;

    /* Compute the 2D projected bounding box of the [-0.5, 0.5]^3 cube,
       then scale it to fit the plot area.                               */
    double rx_min = 1e30, rx_max = -1e30;
    double ry_min = 1e30, ry_max = -1e30;
    for (int corner = 0; corner < 8; corner++) {
        double cx = (corner & 1) ? 0.5 : -0.5;
        double cy = (corner & 2) ? 0.5 : -0.5;
        double cz = (corner & 4) ? 0.5 : -0.5;
        float rx, ry, rz;
        plm3d__project(cx, cy, cz, ca, sa, ce, se, &rx, &ry, &rz);
        if (rx < rx_min) rx_min = rx;
        if (rx > rx_max) rx_max = rx;
        if (ry < ry_min) ry_min = ry;
        if (ry > ry_max) ry_max = ry;
    }

    double proj_w = rx_max - rx_min;
    double proj_h = ry_max - ry_min;
    if (proj_w <= 0.0) proj_w = 1.0;
    if (proj_h <= 0.0) proj_h = 1.0;

    /* Scale uniformly to fit, leaving some padding */
    double scale_x = (pa_w * 0.85) / proj_w;
    double scale_y = (pa_h * 0.85) / proj_h;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    /* ---- 7. draw title ---- */
    if (p->title && p->title[0]) {
        int tw = plm_text_width(p->title);
        int tx = (int)(plot_cx - tw * 0.5);
        int ty = p->plot_area.y0 - plm_text_height() - 2 * S;
        if (ty < 0) ty = 2;
        plm_draw_text(fb, tx, ty, p->title, PLM_FG_COLOR);
    }

    /* ---- 8. draw axes ---- */
    plm3d__draw_axes(p, fb, zbuf, ca, sa, ce, se, plot_cx, plot_cy, scale);

    /* ---- 9. draw series (back-to-front for each type is unnecessary
               with z-buffer; any order works for opaque primitives)  ---- */
    plm3d__render_surfaces(p, fb, zbuf, ca, sa, ce, se,
                           plot_cx, plot_cy, scale,
                           x_range, y_range, z_range);
    plm3d__render_lines(p, fb, zbuf, ca, sa, ce, se,
                        plot_cx, plot_cy, scale,
                        x_range, y_range, z_range);
    plm3d__render_scatters(p, fb, zbuf, ca, sa, ce, se,
                           plot_cx, plot_cy, scale,
                           x_range, y_range, z_range);

    /* ---- 10. draw axis labels and tick labels (no depth test) ---- */
    {
        const int S = PLOTMINI_TEXT_SCALE;
        double x_range = p->x_axis.max - p->x_axis.min;
        double y_range = p->y_axis.max - p->y_axis.min;
        double z_range = p->z_axis.max - p->z_axis.min;
        if (x_range <= 0.0) x_range = 1.0;
        if (y_range <= 0.0) y_range = 1.0;
        if (z_range <= 0.0) z_range = 1.0;

        /* Recompute farthest corner for label origin (same logic as draw_axes) */
        double best_rz2 = 1e30;
        int bcx = 0, bcy = 0, bcz = 0;
        for (int corner = 0; corner < 8; corner++) {
            double tcx = (corner & 1) ? 0.5 : -0.5;
            double tcy = (corner & 2) ? 0.5 : -0.5;
            double tcz = (corner & 4) ? 0.5 : -0.5;
            float rx, ry, rz;
            plm3d__project(tcx, tcy, tcz, ca, sa, ce, se, &rx, &ry, &rz);
            if (rz < best_rz2) { best_rz2 = rz; bcx = corner & 1;
                                 bcy = (corner >> 1) & 1;
                                 bcz = (corner >> 2) & 1; }
        }
        double ox = bcx ? p->x_axis.max : p->x_axis.min;
        double oy = bcy ? p->y_axis.max : p->y_axis.min;
        double oz = bcz ? p->z_axis.max : p->z_axis.min;
        const char *axis_names[3] = {
            p->x_label ? p->x_label : "X",
            p->y_label ? p->y_label : "Y",
            p->z_label ? p->z_label : "Z"
        };

        double endpoints[3][3] = {
            {bcx ? p->x_axis.min : p->x_axis.max, oy, oz},
            {ox, bcy ? p->y_axis.min : p->y_axis.max, oz},
            {ox, oy, bcz ? p->z_axis.min : p->z_axis.max}
        };

        for (int axis = 0; axis < 3; axis++) {
            double ex = endpoints[axis][0];
            double ey = endpoints[axis][1];
            double ez = endpoints[axis][2];

            /* projected endpoint for axis-name placement */
            double enx = (ex - p->x_axis.min) / x_range - 0.5;
            double eny = (ey - p->y_axis.min) / y_range - 0.5;
            double enz = (ez - p->z_axis.min) / z_range - 0.5;
            float erx, ery_, erz;
            plm3d__project((float)enx, (float)eny, (float)enz, ca, sa, ce, se, &erx, &ery_, &erz);
            float epx = (float)(plot_cx + erx * scale);
            float epy = (float)(plot_cy - ery_ * scale);

            /* projected origin */
            double onx = (ox - p->x_axis.min) / x_range - 0.5;
            double ony = (oy - p->y_axis.min) / y_range - 0.5;
            double onz = (oz - p->z_axis.min) / z_range - 0.5;
            float orx, ory_, orz;
            plm3d__project((float)onx, (float)ony, (float)onz, ca, sa, ce, se, &orx, &ory_, &orz);
            float opx = (float)(plot_cx + orx * scale);
            float opy = (float)(plot_cy - ory_ * scale);

            /* axis direction in screen space */
            float adx = epx - opx, ady = epy - opy;
            float alen = sqrtf(adx * adx + ady * ady);
            if (alen < 1.0f) alen = 1.0f;
            float ax_nx = adx / alen, ax_ny = ady / alen;

            /* tick-mark direction in data space (matches plm3d__draw_axes):
               X,Y axes: tick in -Z;  Z axis: tick in -X                     */
            double range = (axis == 0) ? x_range :
                           (axis == 1) ? y_range : z_range;
            double lo    = (axis == 0) ? p->x_axis.min :
                           (axis == 1) ? p->y_axis.min : p->z_axis.min;

            double tick_len_data = range * 0.02;  /* matches draw_axes */

            double rough = range / 5.0;
            double exp_ = pow(10.0, floor(log10(rough)));
            double mant_ = rough / exp_;
            double nice;
            if      (mant_ <= 1.5) nice = 1.0;
            else if (mant_ <= 3.0) nice = 2.0;
            else if (mant_ <= 7.0) nice = 5.0;
            else                   nice = 10.0;
            double step = nice * exp_;
            if (step <= 0.0) step = 1.0;

            double tick_start = ceil(lo / step) * step;
            double label_pad  = 12.0 * S;  /* pixel offset from tick tip */

            for (double v = tick_start; v <= lo + range; v += step) {
                double tx = (axis == 0) ? v : ox;
                double ty = (axis == 1) ? v : oy;
                double tz = (axis == 2) ? v : oz;

                /* tick base and tip in data space */
                double tdx_d = (axis == 2) ? -tick_len_data : 0.0;
                double tdy_d = 0.0;
                double tdz_d = (axis == 2) ? 0.0 : -tick_len_data;

                /* project base */
                double tnx = (tx - p->x_axis.min) / x_range - 0.5;
                double tny = (ty - p->y_axis.min) / y_range - 0.5;
                double tnz = (tz - p->z_axis.min) / z_range - 0.5;
                float bpx, bpy, bpz;
                plm3d__project((float)tnx, (float)tny, (float)tnz,
                               ca, sa, ce, se, &bpx, &bpy, &bpz);

                /* project tip */
                double tnx2 = ((tx + tdx_d) - p->x_axis.min) / x_range - 0.5;
                double tny2 = ((ty + tdy_d) - p->y_axis.min) / y_range - 0.5;
                double tnz2 = ((tz + tdz_d) - p->z_axis.min) / z_range - 0.5;
                float tpx2, tpy2, tpz2;
                plm3d__project((float)tnx2, (float)tny2, (float)tnz2,
                               ca, sa, ce, se, &tpx2, &tpy2, &tpz2);

                /* screen-space tick base and tip */
                float sbx = (float)(plot_cx + bpx * scale);
                float sby = (float)(plot_cy - bpy * scale);
                float stx = (float)(plot_cx + tpx2 * scale);
                float sty = (float)(plot_cy - tpy2 * scale);

                /* tick direction in screen space */
                float tdx_s = stx - sbx, tdy_s = sty - sby;
                float tlen_s = sqrtf(tdx_s * tdx_s + tdy_s * tdy_s);
                if (tlen_s < 1.0f) { tdx_s = -ax_ny; tdy_s = ax_nx; tlen_s = 1.0f; }
                float tnx_s = tdx_s / tlen_s, tny_s = tdy_s / tlen_s;

                /* label at tick tip + padding outward */
                float llx = stx + tnx_s * label_pad;
                float lly = sty + tny_s * label_pad;

                char buf[32];
                snprintf(buf, sizeof(buf), "%g", v);
                int tw = plm_text_width(buf);
                int th = plm_text_height();
                int lx = (int)(llx - tw * 0.5f);
                int ly = (int)(lly + th * 0.5f);
                plm_draw_text(fb, lx, ly, buf, PLM_FG_COLOR);
            }

            /* ---- axis name label at endpoint (further out) ---- */
            {
                float nm_off = 28.0f * S;
                float nlx = epx + ax_nx * nm_off;
                float nly = epy + ax_ny * nm_off;

                const char *name = axis_names[axis];
                int tw = plm_text_width(name);
                int th = plm_text_height();
                plm_draw_text(fb, (int)(nlx - tw * 0.5f),
                              (int)(nly + th * 0.5f), name, PLM_FG_COLOR);
            }
        }
    }

    /* ---- 11. cleanup ---- */
    free(zbuf);
}

/* ------------------------------------------------------------------ */
/*  public API implementation                                         */
/* ------------------------------------------------------------------ */

void plm3d_plot_init(plm3d_plot *p) {
    memset(p, 0, sizeof(*p));
    /* Default view */
    p->view.azimuth   = -45.0;
    p->view.elevation = 25.0;
    /* Default margins: auto (-1) */
    p->margin_left   = -1;
    p->margin_top    = -1;
    p->margin_right  = -1;
    p->margin_bottom = -1;
}

void plm3d_plot_reset(plm3d_plot *p) {
    free(p->scatters); p->scatters = NULL;
    p->scatter_count = 0; p->scatter_cap = 0;
    free(p->lines); p->lines = NULL;
    p->line_count = 0; p->line_cap = 0;
    free(p->surfaces); p->surfaces = NULL;
    p->surface_count = 0; p->surface_cap = 0;
}

void plm3d_plot_add_scatter(plm3d_plot *p,
                            const float *x, const float *y, const float *z,
                            int count, plm3d_scatter_style style) {
    if (p->scatter_count >= p->scatter_cap) {
        int new_cap = p->scatter_cap ? p->scatter_cap * 2 : 8;
        p->scatters = (plm3d_scatter_series *)realloc(
            p->scatters, (size_t)new_cap * sizeof(plm3d_scatter_series));
        p->scatter_cap = new_cap;
    }
    plm3d_scatter_series *ss = &p->scatters[p->scatter_count++];
    ss->x_data = x; ss->y_data = y; ss->z_data = z;
    ss->count  = count;
    ss->style  = style;
}

void plm3d_plot_add_line(plm3d_plot *p,
                         const float *x, const float *y, const float *z,
                         int count, plm3d_line_style style) {
    if (p->line_count >= p->line_cap) {
        int new_cap = p->line_cap ? p->line_cap * 2 : 8;
        p->lines = (plm3d_line_series *)realloc(
            p->lines, (size_t)new_cap * sizeof(plm3d_line_series));
        p->line_cap = new_cap;
    }
    plm3d_line_series *ls = &p->lines[p->line_count++];
    ls->x_data = x; ls->y_data = y; ls->z_data = z;
    ls->count  = count;
    ls->style  = style;
}

void plm3d_plot_add_surface(plm3d_plot *p,
                            const float *x, const float *y, const float *z,
                            int nx, int ny, plm3d_surface_style style) {
    if (p->surface_count >= p->surface_cap) {
        int new_cap = p->surface_cap ? p->surface_cap * 2 : 4;
        p->surfaces = (plm3d_surface_series *)realloc(
            p->surfaces, (size_t)new_cap * sizeof(plm3d_surface_series));
        p->surface_cap = new_cap;
    }
    plm3d_surface_series *ss = &p->surfaces[p->surface_count++];
    ss->x_data = x; ss->y_data = y; ss->z_data = z;
    ss->nx = nx; ss->ny = ny;
    ss->style = style;
}

#endif /* PLOTMINI3D_IMPLEMENTATION */
