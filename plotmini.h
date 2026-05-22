/*
  plotmini.h  --  v0.1  --  public domain, single-header plotting library

  Plot the data you have.  Software-rasterised, zero dependencies beyond libc.
  Output is always a raw pixel framebuffer -- attach any display backend you like.

  ======  USAGE  ======
  1. In *one* C file write:

        #define PLOTMINI_IMPLEMENTATION
        #include "plotmini.h"

  2. Everywhere else just:

        #include "plotmini.h"

  ======  CONFIGURATION MACROS  ======
  Define these *before* the include if you want to override defaults:

    PLOTMINI_MALLOC / PLOTMINI_FREE
        Custom allocator.  Defaults to <stdlib.h> malloc / free.

    PLOTMINI_ASSERT(cond)
        Custom assertion.  Defaults to <assert.h>.

    PLOTMINI_NO_FONT
        Strip the embedded bitmap font (~1.5 KB saved).  Text functions
        become no-ops.

    PLOTMINI_GRAYSCALE_ONLY
        Drop RGBA8 support.  Halves the implementation size.  The library
        only handles 1-byte-per-pixel luminance framebuffers.

  ======  LICENSE  ======
  This is free and unencumbered software released into the public domain.
  See the end of the file for the full Unlicense text.
*/

#ifndef PLOTMINI_H
#define PLOTMINI_H

#include <stddef.h>   /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  TYPES                                                             */
/* ------------------------------------------------------------------ */

/* ---- pixel format ------------------------------------------------- */
typedef enum plm_pixelfmt {
    PLM_GRAY8 = 1,   /* 1 byte per pixel, luminance  0..255 */
    PLM_RGBA8 = 4    /* 4 bytes per pixel, R G B A   0..255 */
} plm_pixelfmt;

/* ---- framebuffer -------------------------------------------------- */
typedef struct plm_fb {
    unsigned char *pixels;   /* raw pixel data, top-left origin        */
    int            width;
    int            height;
    int            stride;   /* bytes between rows (>= width * bpp)    */
    int            bpp;      /* bytes per pixel (1 = GRAY, 4 = RGBA)   */
} plm_fb;

/* ---- colour ------------------------------------------------------- */
typedef struct plm_color {
    unsigned char r, g, b, a;
} plm_color;

#define PLM_RGBA(rr,gg,bb,aa) ((plm_color){(rr),(gg),(bb),(aa)})

#define PLM_BLACK    PLM_RGBA(0x00,0x00,0x00,0xFF)
#define PLM_WHITE    PLM_RGBA(0xFF,0xFF,0xFF,0xFF)
#define PLM_RED      PLM_RGBA(0xFF,0x00,0x00,0xFF)
#define PLM_GREEN    PLM_RGBA(0x00,0xFF,0x00,0xFF)
#define PLM_BLUE     PLM_RGBA(0x00,0x00,0xFF,0xFF)
#define PLM_YELLOW   PLM_RGBA(0xFF,0xFF,0x00,0xFF)
#define PLM_CYAN     PLM_RGBA(0x00,0xFF,0xFF,0xFF)
#define PLM_MAGENTA  PLM_RGBA(0xFF,0x00,0xFF,0xFF)
#define PLM_GREY(x)  PLM_RGBA(x,x,x,0xFF)

/* ---- 2d point (data space) ---------------------------------------- */
typedef struct plm_vec2 {
    double x, y;
} plm_vec2;

/* ---- rectangle (pixel space, integer) ------------------------------ */
typedef struct plm_irect {
    int x0, y0, x1, y1;   /* (x0,y0) inclusive top-left,
                              (x1,y1) exclusive bottom-right            */
} plm_irect;

/* ---- axis scale ---------------------------------------------------- */
typedef enum plm_scale {
    PLM_LINEAR,
    PLM_LOG,
    PLM_CATEGORICAL
} plm_scale;

/* ---- axis description ---------------------------------------------- */
typedef struct plm_axis {
    plm_scale   scale;
    double      min, max;   /* data range; if min==max -> auto-range      */
    int         tick_hint;  /* desired number of major ticks (0 = auto)   */
    int         grid;       /* 0 = none, 1 = major grid lines,
                               2 = major+minor grid lines                  */
    const char *label;      /* axis label, may be NULL                    */
    int         label_font; /* reserved (font id)                         */
    /* categorical axis (PLM_CATEGORICAL) */
    const char **categories;  /* NULL-terminated array of labels, or NULL */
    int          num_categories; /* number of category labels              */
} plm_axis;

/* ---- line style ---------------------------------------------------- */
typedef struct plm_line_style {
    plm_color    color;
    float        width;      /* 0.0..N  pixel width  (0 = hairline)    */
    const char  *legend;     /* optional legend label                   */
} plm_line_style;

/* ---- line series --------------------------------------------------- */
typedef struct plm_line_series {
    const float *x_data;     /* user-owned arrays                       */
    const float *y_data;
    int          count;
    plm_line_style style;
} plm_line_series;

/* ---- bar / histogram style ----------------------------------------- */
typedef struct plm_bar_style {
    plm_color    fill_color;
    plm_color    stroke_color;
    float        stroke_width;  /* 0 = no stroke                       */
    const char  *legend;
} plm_bar_style;

/* ---- histogram series ---------------------------------------------- */
typedef struct plm_hist_series {
    const float   *data;        /* raw samples                          */
    int            count;
    int            bins;        /* number of bins  (0 = auto)           */
    int            density;     /* 0 = counts,  1 = probability density */
    plm_bar_style  style;
} plm_hist_series;

/* ---- bar series (categorical or numeric x) ------------------------- */
typedef struct plm_bar_series {
    const float   *x_data;     /* category indices or x positions        */
    const float   *y_data;     /* bar heights                            */
    int            count;
    float          width;      /* bar width in data units (0 = auto)     */
    plm_bar_style  style;
} plm_bar_series;

/* ---- scatter series ------------------------------------------------ */
typedef struct plm_scatter_style {
    plm_color    color;
    float        radius;       /* dot radius in pixels, default 2.0      */
    const char  *legend;
} plm_scatter_style;

typedef struct plm_scatter_series {
    const float        *x_data;
    const float        *y_data;
    int                 count;
    plm_scatter_style   style;
} plm_scatter_series;

/* ---- a complete plot description (hybrid retained model) ----------- */
typedef struct plm_plot {
    /* axes */
    plm_axis     x_axis;
    plm_axis     y_axis;

    /* layout (pixels) -- margins can be set or auto-computed (-1)     */
    int          margin_left;
    int          margin_top;
    int          margin_right;
    int          margin_bottom;

    /* labels */
    const char  *title;
    const char  *x_label;
    const char  *y_label;

    /* series -- these are filled by plm_plot_add_*()                  */
    plm_line_series *lines;
    int              line_count;
    int              line_cap;     /* internal: allocated capacity      */

    plm_hist_series *hists;
    int              hist_count;
    int              hist_cap;     /* internal                           */

    plm_scatter_series *scatters;
    int                 scatter_count;
    int                 scatter_cap;  /* internal                        */

    plm_bar_series *bars;
    int             bar_count;
    int             bar_cap;     /* internal                           */

    /* internal state (set during render, read-only for user)           */
    plm_irect    plot_area;   /* pixel rect of the data region          */
    int          fb_w;        /* framebuffer width at last render       */
    int          fb_h;        /* framebuffer height at last render      */
} plm_plot;

/* A grid of subplots.  Each cell holds an independent plm_plot.
   Create with plm_figure_init(), fill each cell via plm_figure_plot(),
   then render everything with plm_figure_render().                      */
typedef struct plm_figure {
    int          nrows;
    int          ncols;
    int          hgap;        /* horizontal gap between cells (pixels)   */
    int          vgap;        /* vertical   gap between cells (pixels)   */
    plm_plot    *plots;       /* array[nrows * ncols], owned by figure   */
} plm_figure;

/* ------------------------------------------------------------------ */
/*  PUBLIC API  (declarations)                                        */
/* ------------------------------------------------------------------ */

/* ---- framebuffer -------------------------------------------------- */

/* Create a framebuffer.  The memory is owned by the caller;
   pass `pixels` already allocated or NULL to let the library allocate. */
plm_fb plm_fb_create(unsigned char *pixels, int w, int h, plm_pixelfmt fmt);

/* Fill entire framebuffer with a solid colour. */
void   plm_fb_clear(plm_fb *fb, plm_color c);

/* Fill an integer rectangle.  Clipped to fb bounds. */
void   plm_fb_fill_rect(plm_fb *fb, plm_irect r, plm_color c);

/* Swap R<->B channels in-place (RGBA8 <-> BGRA8).
   Useful before feeding the framebuffer to a BGRA-native display backend. */
void   plm_fb_swizzle_rgba_bgra(plm_fb *fb);

/* ---- colormap ----------------------------------------------------- */

typedef enum plm_cmap {
    PLM_CMAP_HEAT,
    PLM_CMAP_VIRIDIS,
    PLM_CMAP_GRAY
} plm_cmap;

void   plm_imshow(plm_fb *fb, plm_irect rect,
                  const float *data, int data_w, int data_h,
                  double vmin, double vmax, plm_cmap cmap);

/* ---- plot lifecycle ------------------------------------------------ */

plm_irect plm_fit_into(plm_irect bounds, int inner_w, int inner_h);
/* Initialise a plot to defaults. */
void   plm_plot_init(plm_plot *p);

/* Free all internally-allocated series arrays.  plot can be reused. */
void   plm_plot_reset(plm_plot *p);

/* ---- adding data series -------------------------------------------- */

/* Append a line series.  Data is *copied* internally. */
void   plm_plot_add_line(plm_plot *p,
                          const float *x, const float *y, int count,
                          plm_line_style style);

/* Append a histogram series from raw samples. */
void   plm_plot_add_hist(plm_plot *p,
                          const float *samples, int count,
                          int bins, int density,
                          plm_bar_style style);

/* Append a scatter series.  Data is *copied* internally. */
void   plm_plot_add_scatter(plm_plot *p,
                             const float *x, const float *y, int count,
                             plm_scatter_style style);

/* Append a bar series.  Data is *copied* internally.
   For categorical axes, x_data should be category indices (0, 1, ...).
   width is in data units; 0 = auto (0.8 for categorical). */
void   plm_plot_add_bar(plm_plot *p,
                         const float *x, const float *y, int count,
                         float width, plm_bar_style style);

/* ---- rendering ----------------------------------------------------- */

/* Render the current plot description into a framebuffer.
   Layout (margins, tick labels) is recomputed every call, so you can
   call this repeatedly with different fb sizes or updated data. */
void   plm_render(const plm_plot *p, plm_fb *fb);

/* ---- subplot / figure ---------------------------------------------- */

/* Initialise a figure with `nrows` x `ncols` grid of subplots.
   All plots are pre-allocated; access them with plm_figure_plot().
   Default gaps: 20 px horizontal, 20 px vertical.                     */
void     plm_figure_init(plm_figure *fig, int nrows, int ncols);

/* Return a pointer to the plot at (row, col).  Both 0-based.
   Fill the returned plm_plot with series, labels, styles as usual.   */
plm_plot *plm_figure_plot(plm_figure *fig, int row, int col);

/* Render all subplots into one framebuffer.  Clears fb once, then
   renders each plot into its cell.  Margins & labels stay within
   each cell.                                                           */
void     plm_figure_render(const plm_figure *fig, plm_fb *fb);

/* Free all internal storage; figure can be re-init'd or discarded.    */
void     plm_figure_reset(plm_figure *fig);

/* ---- text helpers (optional, only if PLOTMINI_NO_FONT is not set) -- */

/* Returns pixel advance after the string.  Height is fixed per font. */
int    plm_text_width(const char *s);
int    plm_text_height(void);

/* Draw a null-terminated string at pixel (x, baseline_y). */
void   plm_draw_text(plm_fb *fb, int x, int y, const char *s, plm_color c);

#ifdef __cplusplus
}
#endif

#endif /* PLOTMINI_H */

/* ================================================================== */
/*  IMPLEMENTATION                                                    */
/* ================================================================== */
#ifdef PLOTMINI_IMPLEMENTATION

/* ---- standard library includes ------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

/* ---- allocator overrides ------------------------------------------- */
#ifndef PLOTMINI_MALLOC
#define PLOTMINI_MALLOC(sz)  malloc(sz)
#endif
#ifndef PLOTMINI_FREE
#define PLOTMINI_FREE(p)     free(p)
#endif
#ifndef PLOTMINI_ASSERT
#include <assert.h>
#define PLOTMINI_ASSERT(cond) assert(cond)
#endif

/* ---- internal math helpers ----------------------------------------- */

static int plm__isnan(float x) {
#ifdef _MSC_VER
    return _isnan(x);
#else
    return isnan(x);        /* C99; most compilers have it as a macro */
#endif
}

#define plm__clamp(x, lo, hi)  ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define plm__max(a, b)         ((a) > (b) ? (a) : (b))
#define plm__min(a, b)         ((a) < (b) ? (a) : (b))

/* ---- pixel ops ----------------------------------------------------- */

static void plm__set_pixel(plm_fb *fb, int x, int y, plm_color c) {
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
    if (fb->bpp == 1) {
        /* luminance: take perceptual average of R G B */
        fb->pixels[y * fb->stride + x] = 
            (unsigned char)((unsigned int)(c.r * 77 + c.g * 150 + c.b * 29) >> 8);
    } else {
        unsigned char *p = fb->pixels + y * fb->stride + x * 4;
        p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
    }
}

static void plm__blend_pixel(plm_fb *fb, int x, int y, plm_color c, unsigned char alpha) {
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
    if (alpha == 0) return;
    if (alpha == 255) { plm__set_pixel(fb, x, y, c); return; }

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
        /* leave dst[3] (alpha) unchanged -- premultiplied blending */
    }
}

/* Draw a filled, anti-aliased circle.  Used for scatter dots.
   Uses smoothstep falloff over 1.5 px for soft edges. */
static void plm__fill_circle(plm_fb *fb, int cx, int cy, float radius, plm_color color) {
    int r = (int)(radius + 1.5f);  /* extra pixels for the fade band */
    if (r < 1) r = 1;
    int y;
    float r2 = radius * radius;
    float fade = 1.5f;  /* width of the fade band */
    for (y = -r; y <= r; y++) {
        float wy = (float)(y * y);
        if (wy >= (radius + fade) * (radius + fade)) continue;
        float half_wf = sqrtf(r2 - wy);
        if (wy >= r2) half_wf = 0.0f;  /* purely in fade zone */
        int x0 = (int)((float)cx - half_wf - fade);
        int x1 = (int)((float)cx + half_wf + fade + 0.9999f);
        int x;
        for (x = x0; x <= x1; x++) {
            float dx = (float)(x - cx);
            float dy = (float)y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= radius - 0.5f) {
                plm__set_pixel(fb, x, cy + y, color);
            } else if (dist < radius + fade) {
                /* smoothstep: t = 1 - (dist - (r-0.5)) / (fade+0.5) */
                float t = (radius + fade - dist) / (fade + 0.5f);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                /* smoothstep: t*t*(3 - 2*t) */
                t = t * t * (3.0f - 2.0f * t);
                unsigned char alpha = (unsigned char)(t * 255.0f);
                plm__blend_pixel(fb, x, cy + y, color, alpha);
            }
        }
    }
}

/* ---- framebuffer ops ----------------------------------------------- */

plm_fb plm_fb_create(unsigned char *pixels, int w, int h, plm_pixelfmt fmt) {
    plm_fb fb;
    fb.width  = w;
    fb.height = h;
    fb.bpp    = (int)fmt;
    fb.stride = w * fb.bpp;
    if (pixels) {
        fb.pixels = pixels;
    } else {
        fb.pixels = (unsigned char *)PLOTMINI_MALLOC((size_t)(fb.stride * h));
    }
    return fb;
}

void plm_fb_clear(plm_fb *fb, plm_color c) {
    int y;
    if (fb->bpp == 4) {
        /* 4-byte fill -- use memset for speed */
        unsigned char row[4] = { c.r, c.g, c.b, c.a };
        for (y = 0; y < fb->height; y++) {
            unsigned char *dst = fb->pixels + y * fb->stride;
            int x;
            for (x = 0; x < fb->width; x++) {
                dst[0] = row[0]; dst[1] = row[1];
                dst[2] = row[2]; dst[3] = row[3];
                dst += 4;
            }
        }
    } else {
        unsigned char lum = (unsigned char)((unsigned int)(c.r * 77 + c.g * 150 + c.b * 29) >> 8);
        for (y = 0; y < fb->height; y++) {
            memset(fb->pixels + y * fb->stride, lum, (size_t)fb->width);
        }
    }
}

void plm_fb_fill_rect(plm_fb *fb, plm_irect r, plm_color c) {
    /* clip to framebuffer */
    if (r.x0 < 0) r.x0 = 0;
    if (r.y0 < 0) r.y0 = 0;
    if (r.x1 > fb->width)  r.x1 = fb->width;
    if (r.y1 > fb->height) r.y1 = fb->height;
    if (r.x0 >= r.x1 || r.y0 >= r.y1) return;

    if (fb->bpp == 4) {
        unsigned char row[4] = { c.r, c.g, c.b, c.a };
        int x, y;
        for (y = r.y0; y < r.y1; y++) {
            unsigned char *dst = fb->pixels + y * fb->stride + r.x0 * 4;
            for (x = r.x0; x < r.x1; x++) {
                dst[0] = row[0]; dst[1] = row[1];
                dst[2] = row[2]; dst[3] = row[3];
                dst += 4;
            }
        }
    } else {
        unsigned char lum = (unsigned char)((unsigned int)(c.r * 77 + c.g * 150 + c.b * 29) >> 8);
        int y;
        for (y = r.y0; y < r.y1; y++) {
            memset(fb->pixels + y * fb->stride + r.x0, lum,
                   (size_t)(r.x1 - r.x0));
        }
    }
}

void plm_fb_swizzle_rgba_bgra(plm_fb *fb) {
    if (fb->bpp != 4) return;
    int i;
    for (i = 0; i < fb->width * fb->height; i++) {
        unsigned char *p = fb->pixels + i * 4;
        unsigned char r = p[0];
        p[0] = p[2];
        p[2] = r;
    }
}

/* ---- colormap helpers ---------------------------------------------- */

static plm_color plm__cmap_heat(float t) {
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
    unsigned char r, g, b;
    if (t < 0.25f) {
        float s = t / 0.25f;
        r = 0; g = 0; b = (unsigned char)(64 + 191.0f * s);
    } else if (t < 0.5f) {
        float s = (t - 0.25f) / 0.25f;
        r = 0; g = (unsigned char)(255.0f * s);
        b = (unsigned char)(255.0f * (1.0f - s));
    } else if (t < 0.75f) {
        float s = (t - 0.5f) / 0.25f;
        r = (unsigned char)(255.0f * s); g = 255; b = 0;
    } else {
        float s = (t - 0.75f) / 0.25f;
        r = 255; g = (unsigned char)(255.0f * (1.0f - s)); b = 0;
    }
    return PLM_RGBA(r, g, b, 0xFF);
}

static plm_color plm__cmap_viridis(float t) {
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
    unsigned char r, g, b;
    if (t < 0.25f) {
        float s = t / 0.25f;
        r = (unsigned char)(68.0f + 25.0f * s);
        g = (unsigned char)(1.0f + 84.0f * s);
        b = (unsigned char)(84.0f + 57.0f * s);
    } else if (t < 0.5f) {
        float s = (t - 0.25f) / 0.25f;
        r = (unsigned char)(93.0f - 72.0f * s);
        g = (unsigned char)(85.0f + 100.0f * s);
        b = (unsigned char)(141.0f - 57.0f * s);
    } else if (t < 0.75f) {
        float s = (t - 0.5f) / 0.25f;
        r = (unsigned char)(21.0f + 164.0f * s);
        g = (unsigned char)(185.0f + 46.0f * s);
        b = (unsigned char)(84.0f - 45.0f * s);
    } else {
        float s = (t - 0.75f) / 0.25f;
        r = (unsigned char)(185.0f + 68.0f * s);
        g = (unsigned char)(231.0f - 7.0f * s);
        b = (unsigned char)(39.0f - 2.0f * s);
    }
    return PLM_RGBA(r, g, b, 0xFF);
}

static plm_color plm__cmap_gray(float t) {
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
    unsigned char v = (unsigned char)(t * 255.0f);
    return PLM_RGBA(v, v, v, 0xFF);
}

void plm_imshow(plm_fb *fb, plm_irect rect,
                const float *data, int data_w, int data_h,
                double vmin, double vmax, plm_cmap cmap) {
    if (!data || data_w <= 0 || data_h <= 0) return;
    if (fb->bpp != 4 && fb->bpp != 1) return;
    if (rect.x0 < 0) rect.x0 = 0;
    if (rect.y0 < 0) rect.y0 = 0;
    if (rect.x1 > fb->width)  rect.x1 = fb->width;
    if (rect.y1 > fb->height) rect.y1 = fb->height;
    if (rect.x0 >= rect.x1 || rect.y0 >= rect.y1) return;
    int rw = rect.x1 - rect.x0;
    int rh = rect.y1 - rect.y0;
    if (vmin == vmax) {
        double lo =  DBL_MAX, hi = -DBL_MAX;
        int i;
        for (i = 0; i < data_w * data_h; i++) {
            double v = (double)data[i];
            if (!plm__isnan((float)v)) {
                if (v < lo) lo = v;
                if (v > hi) hi = v;
            }
        }
        if (lo >= hi) { lo = 0.0; hi = 1.0; }
        vmin = lo; vmax = hi;
    }
    double range = vmax - vmin;
    if (range <= 0.0) range = 1.0;
    plm_color (*cmap_fn)(float);
    switch (cmap) {
        default:
        case PLM_CMAP_HEAT:    cmap_fn = plm__cmap_heat;    break;
        case PLM_CMAP_VIRIDIS: cmap_fn = plm__cmap_viridis; break;
        case PLM_CMAP_GRAY:    cmap_fn = plm__cmap_gray;    break;
    }
    int dy;
    for (dy = 0; dy < rh; dy++) {
        int sy = dy * data_h / rh;
        unsigned char *row = fb->pixels + (rect.y0 + dy) * fb->stride;
        int dx;
        for (dx = 0; dx < rw; dx++) {
            int sx = dx * data_w / rw;
            double v = (double)data[sy * data_w + sx];
            float t = (float)((v - vmin) / range);
            plm_color c = cmap_fn(t);
            if (fb->bpp == 4) {
                row[(rect.x0 + dx) * 4 + 0] = c.r;
                row[(rect.x0 + dx) * 4 + 1] = c.g;
                row[(rect.x0 + dx) * 4 + 2] = c.b;
                row[(rect.x0 + dx) * 4 + 3] = c.a;
            } else {
                row[rect.x0 + dx] = (unsigned char)(t * 255.0f);
            }
        }
    }
}
plm_irect plm_fit_into(plm_irect bounds, int inner_w, int inner_h) {
    int bw = bounds.x1 - bounds.x0;
    int bh = bounds.y1 - bounds.y0;
    float scale = (float)bw / (float)inner_w;
    if ((float)inner_h * scale > (float)bh)
        scale = (float)bh / (float)inner_h;
    int dw = (int)((float)inner_w * scale);
    int dh = (int)((float)inner_h * scale);
    plm_irect r;
    r.x0 = bounds.x0 + (bw - dw) / 2;
    r.y0 = bounds.y0 + (bh - dh) / 2;
    r.x1 = r.x0 + dw;
    r.y1 = r.y0 + dh;
    return r;
}

/* ---- Cohen-Sutherland line clipping (float, sub-pixel) ------------- */

#define CLIP_INSIDE  0
#define CLIP_LEFT    1
#define CLIP_RIGHT   2
#define CLIP_BOTTOM  4
#define CLIP_TOP     8

static int plm__clip_outcode_f(float x, float y,
                                float xmin, float ymin,
                                float xmax, float ymax) {
    int code = CLIP_INSIDE;
    if (x < xmin) code |= CLIP_LEFT;
    if (x > xmax) code |= CLIP_RIGHT;
    if (y < ymin) code |= CLIP_TOP;
    if (y > ymax) code |= CLIP_BOTTOM;
    return code;
}

/* Clip a line segment to a rectangle; returns 1 if visible, 0 if rejected.
   Operates on float coords to preserve sub-pixel precision. */
static int plm__clip_line_f(float *x0, float *y0, float *x1, float *y1,
                             float xmin, float ymin, float xmax, float ymax) {
    int code0 = plm__clip_outcode_f(*x0, *y0, xmin, ymin, xmax, ymax);
    int code1 = plm__clip_outcode_f(*x1, *y1, xmin, ymin, xmax, ymax);
    while (1) {
        if (!(code0 | code1)) return 1;   /* both inside */
        if (code0 & code1)    return 0;   /* both outside same edge */
        int code = code0 ? code0 : code1;
        float x = 0.0f, y = 0.0f;
        float dx = *x1 - *x0;
        float dy = *y1 - *y0;
        if (code & CLIP_BOTTOM) {
            if (dy == 0.0f) return 0;
            x = *x0 + dx * (ymax - *y0) / dy;
            y = ymax;
        } else if (code & CLIP_TOP) {
            if (dy == 0.0f) return 0;
            x = *x0 + dx * (ymin - *y0) / dy;
            y = ymin;
        } else if (code & CLIP_RIGHT) {
            if (dx == 0.0f) return 0;
            y = *y0 + dy * (xmax - *x0) / dx;
            x = xmax;
        } else {  /* CLIP_LEFT */
            if (dx == 0.0f) return 0;
            y = *y0 + dy * (xmin - *x0) / dx;
            x = xmin;
        }
        if (code == code0) {
            *x0 = x; *y0 = y;
            code0 = plm__clip_outcode_f(*x0, *y0, xmin, ymin, xmax, ymax);
        } else {
            *x1 = x; *y1 = y;
            code1 = plm__clip_outcode_f(*x1, *y1, xmin, ymin, xmax, ymax);
        }
    }
}

/* ---- Wu anti-aliased line (float, sub-pixel endpoints) ------------- */

static void plm__wu_line(plm_fb *fb,
                          float x0, float y0, float x1, float y1,
                          plm_color c) {
    int steep = fabsf(y1 - y0) > fabsf(x1 - x0);
    if (steep) {
        { float t = x0; x0 = y0; y0 = t; }
        { float t = x1; x1 = y1; y1 = t; }
    }
    if (x0 > x1) {
        { float t = x0; x0 = x1; x1 = t; }
        { float t = y0; y0 = y1; y1 = t; }
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float gradient;
    if (dx < 1e-6f) gradient = 1.0f;
    else             gradient = dy / dx;

    /* ---- handle first endpoint ---- */
    int   xpxl1 = (int)x0;
    float yend  = y0 + gradient * ((float)xpxl1 + 1.0f - x0);
    float xgap  = 1.0f - (x0 - (float)xpxl1);  /* fraction of pixel covered */
    {
        int   ipart = (int)yend;
        float fpart = yend - (float)ipart;
        unsigned char a1 = (unsigned char)((1.0f - fpart) * xgap * 255.0f);
        unsigned char a2 = (unsigned char)(fpart * xgap * 255.0f);
        if (steep) {
            plm__blend_pixel(fb, ipart,     xpxl1, c, a1);
            plm__blend_pixel(fb, ipart + 1, xpxl1, c, a2);
        } else {
            plm__blend_pixel(fb, xpxl1, ipart,     c, a1);
            plm__blend_pixel(fb, xpxl1, ipart + 1, c, a2);
        }
    }

    /* ---- handle second endpoint ---- */
    int   xpxl2 = (int)x1;
    float yend2 = y1 + gradient * ((float)xpxl2 - x1);  /* y at exact x1 */
    float xgap2 = x1 - (float)xpxl2;  /* fraction of pixel covered */
    if (xgap2 > 0.0f) {
        int   ipart = (int)yend2;
        float fpart = yend2 - (float)ipart;
        unsigned char a1 = (unsigned char)((1.0f - fpart) * xgap2 * 255.0f);
        unsigned char a2 = (unsigned char)(fpart * xgap2 * 255.0f);
        if (steep) {
            plm__blend_pixel(fb, ipart,     xpxl2, c, a1);
            plm__blend_pixel(fb, ipart + 1, xpxl2, c, a2);
        } else {
            plm__blend_pixel(fb, xpxl2, ipart,     c, a1);
            plm__blend_pixel(fb, xpxl2, ipart + 1, c, a2);
        }
    }

    /* ---- main loop ---- */
    {
        float intery = yend + gradient;
        int   x;
        for (x = xpxl1 + 1; x < xpxl2; x++) {
            int   ipart = (int)intery;
            float fpart = intery - (float)ipart;
            unsigned char a1 = (unsigned char)((1.0f - fpart) * 255.0f);
            unsigned char a2 = (unsigned char)(fpart * 255.0f);
            if (steep) {
                plm__blend_pixel(fb, ipart,     x, c, a1);
                plm__blend_pixel(fb, ipart + 1, x, c, a2);
            } else {
                plm__blend_pixel(fb, x, ipart,     c, a1);
                plm__blend_pixel(fb, x, ipart + 1, c, a2);
            }
            intery += gradient;
        }
    }
}

static void plm__wu_line_thick(plm_fb *fb,
                                float x0, float y0, float x1, float y1,
                                plm_color c, float width) {
    if (width <= 1.0f) {
        plm__wu_line(fb, x0, y0, x1, y1, c);
        return;
    }
    int n = (int)(width + 0.5f);
    float half = (width - 1.0f) * 0.5f;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) len = 1.0f;
    float px = -dy / len;
    float py =  dx / len;
    int i;
    for (i = 0; i < n; i++) {
        float off = (float)i - half;
        plm__wu_line(fb,
            x0 + px * off, y0 + py * off,
            x1 + px * off, y1 + py * off, c);
    }
}
/* ---- embedded bitmap font ------------------------------------------ */
#ifndef PLOTMINI_NO_FONT

/*
   5x7 pixel font, ASCII 32-126, stored column-major within each glyph.
   Each glyph is 6 bytes (5 columns x 1 byte each) + implicit 1 px spacing.
   Glyph height = 7 px.  Baseline = 6 (top 0..6).
   Data auto-generated; not hand-tuned.  Replace with your own if desired.
*/

static const unsigned char plm__font_data[95 * 5] = {
    /* space ' ' */ 0x00,0x00,0x00,0x00,0x00,
    /* ! */ 0x00,0x00,0x5F,0x00,0x00,
    /* " */ 0x00,0x07,0x00,0x07,0x00,
    /* # */ 0x14,0x7F,0x14,0x7F,0x14,
    /* $ */ 0x24,0x2A,0x7F,0x2A,0x12,
    /* % */ 0x23,0x13,0x08,0x64,0x62,
    /* & */ 0x36,0x49,0x55,0x22,0x50,
    /* ' */ 0x00,0x05,0x03,0x00,0x00,
    /* ( */ 0x00,0x1C,0x22,0x41,0x00,
    /* ) */ 0x00,0x41,0x22,0x1C,0x00,
    /* * */ 0x08,0x2A,0x1C,0x2A,0x08,
    /* + */ 0x08,0x08,0x3E,0x08,0x08,
    /* , */ 0x00,0x50,0x30,0x00,0x00,
    /* - */ 0x08,0x08,0x08,0x08,0x08,
    /* . */ 0x00,0x60,0x60,0x00,0x00,
    /* / */ 0x20,0x10,0x08,0x04,0x02,
    /* 0 */ 0x3E,0x51,0x49,0x45,0x3E,
    /* 1 */ 0x00,0x42,0x7F,0x40,0x00,
    /* 2 */ 0x42,0x61,0x51,0x49,0x46,
    /* 3 */ 0x21,0x41,0x45,0x4B,0x31,
    /* 4 */ 0x18,0x14,0x12,0x7F,0x10,
    /* 5 */ 0x27,0x45,0x45,0x45,0x39,
    /* 6 */ 0x3C,0x4A,0x49,0x49,0x30,
    /* 7 */ 0x01,0x71,0x09,0x05,0x03,
    /* 8 */ 0x36,0x49,0x49,0x49,0x36,
    /* 9 */ 0x06,0x49,0x49,0x29,0x1E,
    /* : */ 0x00,0x36,0x36,0x00,0x00,
    /* ; */ 0x00,0x56,0x36,0x00,0x00,
    /* < */ 0x00,0x08,0x14,0x22,0x41,
    /* = */ 0x14,0x14,0x14,0x14,0x14,
    /* > */ 0x41,0x22,0x14,0x08,0x00,
    /* ? */ 0x02,0x01,0x51,0x09,0x06,
    /* @ */ 0x32,0x49,0x79,0x41,0x3E,
    /* A */ 0x7E,0x11,0x11,0x11,0x7E,
    /* B */ 0x7F,0x49,0x49,0x49,0x36,
    /* C */ 0x3E,0x41,0x41,0x41,0x22,
    /* D */ 0x7F,0x41,0x41,0x22,0x1C,
    /* E */ 0x7F,0x49,0x49,0x49,0x41,
    /* F */ 0x7F,0x09,0x09,0x01,0x01,
    /* G */ 0x3E,0x41,0x41,0x51,0x32,
    /* H */ 0x7F,0x08,0x08,0x08,0x7F,
    /* I */ 0x00,0x41,0x7F,0x41,0x00,
    /* J */ 0x20,0x40,0x41,0x3F,0x01,
    /* K */ 0x7F,0x08,0x14,0x22,0x41,
    /* L */ 0x7F,0x40,0x40,0x40,0x40,
    /* M */ 0x7F,0x02,0x04,0x02,0x7F,
    /* N */ 0x7F,0x04,0x08,0x10,0x7F,
    /* O */ 0x3E,0x41,0x41,0x41,0x3E,
    /* P */ 0x7F,0x09,0x09,0x09,0x06,
    /* Q */ 0x3E,0x41,0x51,0x21,0x5E,
    /* R */ 0x7F,0x09,0x19,0x29,0x46,
    /* S */ 0x46,0x49,0x49,0x49,0x31,
    /* T */ 0x01,0x01,0x7F,0x01,0x01,
    /* U */ 0x3F,0x40,0x40,0x40,0x3F,
    /* V */ 0x1F,0x20,0x40,0x20,0x1F,
    /* W */ 0x7F,0x20,0x18,0x20,0x7F,
    /* X */ 0x63,0x14,0x08,0x14,0x63,
    /* Y */ 0x03,0x04,0x78,0x04,0x03,
    /* Z */ 0x61,0x51,0x49,0x45,0x43,
    /* [ */ 0x00,0x00,0x7F,0x41,0x41,
    /* \ */ 0x02,0x04,0x08,0x10,0x20,
    /* ] */ 0x41,0x41,0x7F,0x00,0x00,
    /* ^ */ 0x04,0x02,0x01,0x02,0x04,
    /* _ */ 0x40,0x40,0x40,0x40,0x40,
    /* ` */ 0x00,0x01,0x02,0x04,0x00,
    /* a */ 0x20,0x54,0x54,0x54,0x78,
    /* b */ 0x7F,0x48,0x44,0x44,0x38,
    /* c */ 0x38,0x44,0x44,0x44,0x20,
    /* d */ 0x38,0x44,0x44,0x48,0x7F,
    /* e */ 0x38,0x54,0x54,0x54,0x18,
    /* f */ 0x08,0x7E,0x09,0x01,0x02,
    /* g */ 0x08,0x14,0x54,0x54,0x3C,
    /* h */ 0x7F,0x08,0x04,0x04,0x78,
    /* i */ 0x00,0x44,0x7D,0x40,0x00,
    /* j */ 0x20,0x40,0x44,0x3D,0x00,
    /* k */ 0x00,0x7F,0x10,0x28,0x44,
    /* l */ 0x00,0x41,0x7F,0x40,0x00,
    /* m */ 0x7C,0x04,0x18,0x04,0x78,
    /* n */ 0x7C,0x08,0x04,0x04,0x78,
    /* o */ 0x38,0x44,0x44,0x44,0x38,
    /* p */ 0x7C,0x14,0x14,0x14,0x08,
    /* q */ 0x08,0x14,0x14,0x18,0x7C,
    /* r */ 0x7C,0x08,0x04,0x04,0x08,
    /* s */ 0x48,0x54,0x54,0x54,0x20,
    /* t */ 0x04,0x3F,0x44,0x40,0x20,
    /* u */ 0x3C,0x40,0x40,0x20,0x7C,
    /* v */ 0x1C,0x20,0x40,0x20,0x1C,
    /* w */ 0x3C,0x40,0x30,0x40,0x3C,
    /* x */ 0x44,0x28,0x10,0x28,0x44,
    /* y */ 0x0C,0x50,0x50,0x50,0x3C,
    /* z */ 0x44,0x64,0x54,0x4C,0x44,
    /* { */ 0x00,0x08,0x36,0x41,0x00,
    /* | */ 0x00,0x00,0x7F,0x00,0x00,
    /* } */ 0x00,0x41,0x36,0x08,0x00,
    /* ~ */ 0x08,0x04,0x08,0x10,0x08,
};

#define PLM_FONT_W  5
#define PLM_FONT_H  7
#define PLM_FONT_ADVANCE 6   /* width + 1 px spacing */

int plm_text_width(const char *s) {
    int w = 0;
    while (*s) {
        int ch = (unsigned char)*s;
        if (ch >= 32 && ch < 127) w += PLM_FONT_ADVANCE;
        s++;
    }
    return w;
}

int plm_text_height(void) {
    return PLM_FONT_H;
}

void plm_draw_text(plm_fb *fb, int x, int y, const char *s, plm_color c) {
    while (*s) {
        int ch = (unsigned char)*s;
        if (ch >= 32 && ch < 127) {
            const unsigned char *glyph = plm__font_data + (ch - 32) * PLM_FONT_W;
            int col;
            for (col = 0; col < PLM_FONT_W; col++) {
                unsigned char bits = glyph[col];
                int row;
                for (row = 0; row < PLM_FONT_H; row++) {
                    if (bits & (1u << row)) {
                        plm__set_pixel(fb, x + col, y - PLM_FONT_H + 1 + row, c);
                    }
                }
            }
            x += PLM_FONT_ADVANCE;
        }
        s++;
        /* simple wrap?  Not for now -- caller controls placement. */
    }
}

#else /* PLOTMINI_NO_FONT */

int  plm_text_width(const char *s)  { (void)s; return 0; }
int  plm_text_height(void)          { return 0; }
void plm_draw_text(plm_fb *fb, int x, int y, const char *s, plm_color c) {
    (void)fb; (void)x; (void)y; (void)s; (void)c;
}

#endif /* PLOTMINI_NO_FONT */

/* ---- plot lifecycle ------------------------------------------------ */

void plm_plot_init(plm_plot *p) {
    memset(p, 0, sizeof(*p));

    /* sensible defaults */
    p->x_axis.scale     = PLM_LINEAR;
    p->y_axis.scale     = PLM_LINEAR;
    p->x_axis.tick_hint = 5;
    p->y_axis.tick_hint = 5;
    p->x_axis.grid      = 1;  /* major grid lines on by default */
    p->y_axis.grid      = 1;

    p->margin_left      = 60;
    p->margin_top       = 30;
    p->margin_right     = 20;
    p->margin_bottom    = 40;

    p->title   = NULL;
    p->x_label = NULL;
    p->y_label = NULL;
}

void plm_plot_reset(plm_plot *p) {
    if (p->lines) PLOTMINI_FREE(p->lines);
    if (p->hists) PLOTMINI_FREE(p->hists);
    if (p->scatters) PLOTMINI_FREE(p->scatters);
    if (p->bars) PLOTMINI_FREE(p->bars);
    plm_plot_init(p);
}

/* ---- adding series ------------------------------------------------- */

void plm_plot_add_line(plm_plot *p,
                        const float *x, const float *y, int count,
                        plm_line_style style) {
    if (count <= 0) return;
    /* grow capacity if needed */
    if (p->line_count + 1 > p->line_cap) {
        int new_cap = p->line_cap ? p->line_cap * 2 : 4;
        p->lines = (plm_line_series *)realloc(p->lines,
                     (size_t)new_cap * sizeof(plm_line_series));
        p->line_cap = new_cap;
    }
    plm_line_series *ls = &p->lines[p->line_count++];
    ls->x_data  = (const float *)malloc((size_t)count * sizeof(float));
    ls->y_data  = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)ls->x_data, x, (size_t)count * sizeof(float));
    memcpy((void*)ls->y_data, y, (size_t)count * sizeof(float));
    ls->count   = count;
    ls->style   = style;
}

void plm_plot_add_hist(plm_plot *p,
                        const float *samples, int count,
                        int bins, int density,
                        plm_bar_style style) {
    if (count <= 0) return;
    if (p->hist_count + 1 > p->hist_cap) {
        int new_cap = p->hist_cap ? p->hist_cap * 2 : 4;
        p->hists = (plm_hist_series *)realloc(p->hists,
                     (size_t)new_cap * sizeof(plm_hist_series));
        p->hist_cap = new_cap;
    }
    plm_hist_series *hs = &p->hists[p->hist_count++];
    hs->data    = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)hs->data, samples, (size_t)count * sizeof(float));
    hs->count   = count;
    hs->bins    = bins;
    hs->density = density;
    hs->style   = style;
}

void plm_plot_add_scatter(plm_plot *p,
                           const float *x, const float *y, int count,
                           plm_scatter_style style) {
    if (count <= 0) return;
    if (p->scatter_count + 1 > p->scatter_cap) {
        int new_cap = p->scatter_cap ? p->scatter_cap * 2 : 4;
        p->scatters = (plm_scatter_series *)realloc(p->scatters,
                          (size_t)new_cap * sizeof(plm_scatter_series));
        p->scatter_cap = new_cap;
    }
    plm_scatter_series *ss = &p->scatters[p->scatter_count++];
    ss->x_data = (const float *)malloc((size_t)count * sizeof(float));
    ss->y_data = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)ss->x_data, x, (size_t)count * sizeof(float));
    memcpy((void*)ss->y_data, y, (size_t)count * sizeof(float));
    ss->count  = count;
    ss->style  = style;
    if (ss->style.radius <= 0.0f) ss->style.radius = 2.0f;
}

void plm_plot_add_bar(plm_plot *p,
                       const float *x, const float *y, int count,
                       float width, plm_bar_style style) {
    if (count <= 0) return;
    if (p->bar_count + 1 > p->bar_cap) {
        int new_cap = p->bar_cap ? p->bar_cap * 2 : 4;
        p->bars = (plm_bar_series *)realloc(p->bars,
                       (size_t)new_cap * sizeof(plm_bar_series));
        p->bar_cap = new_cap;
    }
    plm_bar_series *bs = &p->bars[p->bar_count++];
    bs->x_data = (const float *)malloc((size_t)count * sizeof(float));
    bs->y_data = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)bs->x_data, x, (size_t)count * sizeof(float));
    memcpy((void*)bs->y_data, y, (size_t)count * sizeof(float));
    bs->count  = count;
    bs->width  = width;
    bs->style  = style;
}

/* ---- auto-range ---------------------------------------------------- */

static void plm__axis_autorange(plm_axis *ax,
                                 const plm_line_series *lines, int lc,
                                 const plm_hist_series *hists, int hc,
                                 const plm_scatter_series *scatters, int sc,
                                 const plm_bar_series *bars, int bc,
                                 int is_y) {
    /* categorical axis: use category count if available */
    if (ax->scale == PLM_CATEGORICAL && ax->num_categories > 0) {
        ax->min = -0.5;
        ax->max = (double)ax->num_categories - 0.5;
        return;
    }

    double lo =  DBL_MAX;
    double hi = -DBL_MAX;
    int found = 0;
    int i;

    for (i = 0; i < lc; i++) {
        const float *data = is_y ? lines[i].y_data : lines[i].x_data;
        int j;
        for (j = 0; j < lines[i].count; j++) {
            float v = data[j];
            if (!plm__isnan(v)) {
                if ((double)v < lo) lo = (double)v;
                if ((double)v > hi) hi = (double)v;
                found = 1;
            }
        }
    }

    for (i = 0; i < sc; i++) {
        const float *data = is_y ? scatters[i].y_data : scatters[i].x_data;
        int j;
        for (j = 0; j < scatters[i].count; j++) {
            float v = data[j];
            if (!plm__isnan(v)) {
                if ((double)v < lo) lo = (double)v;
                if ((double)v > hi) hi = (double)v;
                found = 1;
            }
        }
    }

    for (i = 0; i < bc; i++) {
        const float *data = is_y ? bars[i].y_data : bars[i].x_data;
        int j;
        for (j = 0; j < bars[i].count; j++) {
            float v = data[j];
            if (!plm__isnan(v)) {
                if ((double)v < lo) lo = (double)v;
                if ((double)v > hi) hi = (double)v;
                found = 1;
            }
        }
        /* for y-axis, also consider bar heights (y values) */
        if (is_y) {
            for (j = 0; j < bars[i].count; j++) {
                float v = bars[i].y_data[j];
                if (!plm__isnan(v)) {
                    if ((double)v > hi) hi = (double)v;
                    found = 1;
                }
            }
        }
    }

    if (is_y) {
        /* Y-axis with histograms: pre-compute bin max, range from 0 */
        if (lo > 0.0) lo = 0.0;
        if (hi < 0.0) hi = 0.0;
        for (i = 0; i < hc; i++) {
            if (hists[i].count <= 0) continue;
            int bins = hists[i].bins;
            if (bins <= 0) {
                double k = ceil(log2((double)hists[i].count) + 1.0);
                bins = (int)(k < 4 ? 4 : (k > 100 ? 100 : k));
            }
            double dmin =  DBL_MAX, dmax = -DBL_MAX;
            int j;
            for (j = 0; j < hists[i].count; j++) {
                double v = (double)hists[i].data[j];
                if (!plm__isnan((float)v)) {
                    if (v < dmin) dmin = v;
                    if (v > dmax) dmax = v;
                }
            }
            if (dmax <= dmin) { dmax = dmin + 1.0; }
            double bin_w = (dmax - dmin) / (double)bins;
            int *counts = (int *)calloc((size_t)bins, sizeof(int));
            for (j = 0; j < hists[i].count; j++) {
                double v = (double)hists[i].data[j];
                if (plm__isnan((float)v)) continue;
                int b = (int)((v - dmin) / bin_w);
                if (b < 0) b = 0;
                if (b >= bins) b = bins - 1;
                counts[b]++;
            }
            for (j = 0; j < bins; j++) {
                if (counts[j] > hi) hi = (double)counts[j];
            }
            found = 1;
            free(counts);
        }
    } else {
        for (i = 0; i < hc; i++) {
            int j;
            for (j = 0; j < hists[i].count; j++) {
                double v = (double)hists[i].data[j];
                if (!plm__isnan((float)v)) {
                    if (v < lo) lo = v;
                    if (v > hi) hi = v;
                    found = 1;
                }
            }
        }
    }

    if (found && fabs(hi - lo) < 1e-12) {
        hi = lo + 1.0;
        lo = lo - 1.0;
    }
    if (found) {
        double pad = (hi - lo) * 0.05;
        if (pad < 1e-12) pad = 1.0;
        ax->min = lo - pad;
        ax->max = hi + pad;
    } else {
        ax->min = 0.0;
        ax->max = 1.0;
    }
}

/* ---- tick generation ----------------------------------------------- */

/*
   "nice number" routine.
   Given range R = max-min, target number of intervals N,
   find a step S ∈ {1,2,5} × 10^k that yields roughly N ticks.
*/

static double plm__nice_step(double range, int target_intervals) {
    if (range <= 0.0) return 1.0;
    double rough = range / (double)target_intervals;
    double exp   = pow(10.0, floor(log10(rough)));
    double mant  = rough / exp;
    double nice;
    if      (mant < 1.5) nice = 1.0;
    else if (mant < 3.0) nice = 2.0;
    else if (mant < 7.0) nice = 5.0;
    else                 nice = 10.0;
    return nice * exp;
}

/* ---- coordinate mapping -------------------------------------------- */

/*
   Map a data-space value to pixel space.
   Returns a float pixel coordinate.
*/
static float plm__map_x(const plm_plot *p, double val) {
    double t = (val - p->x_axis.min) / (p->x_axis.max - p->x_axis.min);
    return (float)(p->plot_area.x0 + t * (p->plot_area.x1 - p->plot_area.x0));
}

static float plm__map_y(const plm_plot *p, double val) {
    double t = (val - p->y_axis.min) / (p->y_axis.max - p->y_axis.min);
    /* Y is flipped: data min -> pixel bottom */
    return (float)(p->plot_area.y1 - 1 - t * (p->plot_area.y1 - p->plot_area.y0));
}

/* ---- core render --------------------------------------------------- */

/* Render one plot into a rectangular cell of the framebuffer.
   cell_x0,cell_y0  = top-left of the cell in fb pixel coords.
   cell_x1,cell_y1  = bottom-right (exclusive).                         */
static void plm__render_plot_into(plm_plot *p, plm_fb *fb,
                                   int cell_x0, int cell_y0,
                                   int cell_x1, int cell_y1) {
    /* 1. auto-range if requested */
    if (p->x_axis.min == p->x_axis.max) {
        plm__axis_autorange(&p->x_axis, p->lines, p->line_count,
                             p->hists, p->hist_count,
                             p->scatters, p->scatter_count,
                             p->bars, p->bar_count, 0);
    }
    if (p->y_axis.min == p->y_axis.max) {
        plm__axis_autorange(&p->y_axis, p->lines, p->line_count,
                             p->hists, p->hist_count,
                             p->scatters, p->scatter_count,
                             p->bars, p->bar_count, 1);
    }

    /* 2. compute plot area in pixels (inset from cell by margins) */
    p->plot_area.x0 = cell_x0 + p->margin_left;
    p->plot_area.y0 = cell_y0 + p->margin_top;
    p->plot_area.x1 = cell_x1 - p->margin_right;
    p->plot_area.y1 = cell_y1 - p->margin_bottom;

    p->fb_w = cell_x1 - cell_x0;
    p->fb_h = cell_y1 - cell_y0;

    /* 3. draw plot-area background */
    {
        plm_irect bg;
        bg.x0 = p->plot_area.x0; bg.y0 = p->plot_area.y0;
        bg.x1 = p->plot_area.x1; bg.y1 = p->plot_area.y1;
        plm_fb_fill_rect(fb, bg, PLM_WHITE);
    }

    /* 4. draw grid lines (X axis - major) */
    if (p->x_axis.grid >= 1) {
        plm_color grid_c = PLM_GREY(220);
        if (p->x_axis.scale == PLM_CATEGORICAL && p->x_axis.num_categories > 0) {
            /* grid at each category position */
            int ci;
            for (ci = 0; ci < p->x_axis.num_categories; ci++) {
                float px = plm__map_x(p, (double)ci);
                int ix = (int)(px + 0.5f);
                if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                    int y;
                    for (y = p->plot_area.y0; y < p->plot_area.y1; y++) {
                        plm__blend_pixel(fb, ix, y, grid_c, 128);
                    }
                }
            }
        } else {
            double range = p->x_axis.max - p->x_axis.min;
            double step  = plm__nice_step(range, p->x_axis.tick_hint > 0 ?
                                         p->x_axis.tick_hint : 5);
            double lo = floor(p->x_axis.min / step) * step;
            double v;
            for (v = lo; v <= p->x_axis.max + step * 0.5; v += step) {
                float px = plm__map_x(p, v);
                int ix = (int)(px + 0.5f);
                if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                    int y;
                    for (y = p->plot_area.y0; y < p->plot_area.y1; y++) {
                        plm__blend_pixel(fb, ix, y, grid_c, 128);
                    }
                }
            }
        }
    }

    /* 5. draw grid lines (Y axis - major) */
    if (p->y_axis.grid >= 1) {
        plm_color grid_c = PLM_GREY(220);
        if (p->y_axis.scale == PLM_CATEGORICAL && p->y_axis.num_categories > 0) {
            int ci;
            for (ci = 0; ci < p->y_axis.num_categories; ci++) {
                float py = plm__map_y(p, (double)ci);
                int iy = (int)(py + 0.5f);
                if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                    int x;
                    for (x = p->plot_area.x0; x < p->plot_area.x1; x++) {
                        plm__blend_pixel(fb, x, iy, grid_c, 128);
                    }
                }
            }
        } else {
            double range = p->y_axis.max - p->y_axis.min;
            double step  = plm__nice_step(range, p->y_axis.tick_hint > 0 ?
                                         p->y_axis.tick_hint : 5);
            double lo = floor(p->y_axis.min / step) * step;
            double v;
            for (v = lo; v <= p->y_axis.max + step * 0.5; v += step) {
                float py = plm__map_y(p, v);
                int iy = (int)(py + 0.5f);
                if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                    int x;
                    for (x = p->plot_area.x0; x < p->plot_area.x1; x++) {
                        plm__blend_pixel(fb, x, iy, grid_c, 128);
                    }
                }
            }
        }
    }

    /* 6. draw axis spines (the L-shape) */
    {
        plm_color ax_c = PLM_BLACK;
        /* bottom */
        { plm_irect r = { p->plot_area.x0, p->plot_area.y1 - 1,
                          p->plot_area.x1, p->plot_area.y1 };
          plm_fb_fill_rect(fb, r, ax_c); }
        /* left */
        { plm_irect r = { p->plot_area.x0, p->plot_area.y0,
                          p->plot_area.x0 + 1, p->plot_area.y1 };
          plm_fb_fill_rect(fb, r, ax_c); }
    }

    /* 7. draw line series */
    {
        int si;
        for (si = 0; si < p->line_count; si++) {
            const plm_line_series *ls = &p->lines[si];
            int i;
            int seg_start = -1;
            for (i = 0; i < ls->count; i++) {
                /* NaN gap handling */
                if (plm__isnan(ls->x_data[i]) || plm__isnan(ls->y_data[i])) {
                    seg_start = -1;
                    continue;
                }
                if (seg_start >= 0) {
                    float px0 = plm__map_x(p, ls->x_data[i-1]);
                    float py0 = plm__map_y(p, ls->y_data[i-1]);
                    float px1 = plm__map_x(p, ls->x_data[i]);
                    float py1 = plm__map_y(p, ls->y_data[i]);
                    if (plm__clip_line_f(&px0, &py0, &px1, &py1,
                                         (float)p->plot_area.x0, (float)p->plot_area.y0,
                                         (float)(p->plot_area.x1 - 1),
                                         (float)(p->plot_area.y1 - 1))) {
                        plm__wu_line_thick(fb, px0, py0, px1, py1,
                                           ls->style.color, ls->style.width);
                    }
                }
                if (seg_start < 0) seg_start = i;
            }
        }
    }

    /* 8. draw histogram series */
    {
        int si;
        for (si = 0; si < p->hist_count; si++) {
            const plm_hist_series *hs = &p->hists[si];
            if (hs->count <= 0) continue;

            int bins = hs->bins;
            if (bins <= 0) {
                double k = ceil(log2((double)hs->count) + 1.0);
                bins = (int)(k < 4 ? 4 : (k > 100 ? 100 : k));
            }

            double dmin =  DBL_MAX, dmax = -DBL_MAX;
            int j;
            for (j = 0; j < hs->count; j++) {
                double v = (double)hs->data[j];
                if (!plm__isnan((float)v)) {
                    if (v < dmin) dmin = v;
                    if (v > dmax) dmax = v;
                }
            }
            if (dmax <= dmin) continue;

            double bin_w = (dmax - dmin) / (double)bins;
            int *counts = (int *)calloc((size_t)bins, sizeof(int));
            if (!counts) continue;

            for (j = 0; j < hs->count; j++) {
                double v = (double)hs->data[j];
                if (plm__isnan((float)v)) continue;
                int b = (int)((v - dmin) / bin_w);
                if (b < 0) b = 0;
                if (b >= bins) b = bins - 1;
                counts[b]++;
            }

            int max_count = 0;
            for (j = 0; j < bins; j++) {
                if (counts[j] > max_count) max_count = counts[j];
            }
            if (max_count == 0) { free(counts); continue; }

            float bar_area_left  = plm__map_x(p, dmin);
            float bar_area_right = plm__map_x(p, dmax);
            float bar_w = (bar_area_right - bar_area_left) / (float)bins;
            if (bar_w < 1.0f) bar_w = 1.0f;

            for (j = 0; j < bins; j++) {
                double h = hs->density
                    ? (double)counts[j] / ((double)hs->count * bin_w)
                    : (double)counts[j];
                float px_left  = bar_area_left + (float)j * bar_w;
                float px_right = px_left + bar_w;
                float baseline = plm__map_y(p, 0.0);
                float bar_top  = plm__map_y(p, h);

                if (px_right < (float)p->plot_area.x0 ||
                    px_left  > (float)p->plot_area.x1) continue;

                if (px_left  < (float)p->plot_area.x0) px_left  = (float)p->plot_area.x0;
                if (px_right > (float)p->plot_area.x1) px_right = (float)p->plot_area.x1;

                plm_irect r;
                r.x0 = (int)px_left;
                r.y0 = (int)(bar_top < baseline ? bar_top : baseline);
                r.x1 = (int)(px_right + 0.5f);
                r.y1 = (int)(bar_top < baseline ? baseline : bar_top);
                if (r.x1 <= r.x0) r.x1 = r.x0 + 1;
                if (r.y1 <= r.y0) r.y1 = r.y0 + 1;

                plm_fb_fill_rect(fb, r, hs->style.fill_color);

                if (hs->style.stroke_width > 0.0f) {
                    plm_color sc = hs->style.stroke_color;
                    int x;
                    for (x = r.x0; x < r.x1; x++) {
                        plm__set_pixel(fb, x, r.y0, sc);
                        plm__set_pixel(fb, x, r.y1 - 1, sc);
                    }
                    int y;
                    for (y = r.y0; y < r.y1; y++) {
                        plm__set_pixel(fb, r.x0, y, sc);
                        plm__set_pixel(fb, r.x1 - 1, y, sc);
                    }
                }
            }
            free(counts);
        }
    }

    /* 9. draw scatter series */
    {
        int si;
        for (si = 0; si < p->scatter_count; si++) {
            const plm_scatter_series *ss = &p->scatters[si];
            int i;
            for (i = 0; i < ss->count; i++) {
                if (plm__isnan(ss->x_data[i]) || plm__isnan(ss->y_data[i]))
                    continue;
                float px = plm__map_x(p, ss->x_data[i]);
                float py = plm__map_y(p, ss->y_data[i]);
                int ix = (int)(px + 0.5f);
                int iy = (int)(py + 0.5f);
                /* only draw if centre is inside plot area */
                if (ix >= p->plot_area.x0 && ix < p->plot_area.x1 &&
                    iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                    plm__fill_circle(fb, ix, iy, ss->style.radius, ss->style.color);
                }
            }
        }
    }

    /* 9b. draw bar series */
    {
        int si;
        for (si = 0; si < p->bar_count; si++) {
            const plm_bar_series *bs = &p->bars[si];
            if (bs->count <= 0) continue;

            /* determine bar width */
            float bar_w = bs->width;
            if (bar_w <= 0.0f) {
                /* auto width: 0.8 for categorical, computed for numeric */
                if (p->x_axis.scale == PLM_CATEGORICAL && p->x_axis.num_categories > 1) {
                    bar_w = 0.8f;
                } else {
                    bar_w = (float)(p->x_axis.max - p->x_axis.min) / (float)bs->count * 0.8f;
                    if (bar_w <= 0.0f) bar_w = 0.8f;
                }
            }

            float half_w = bar_w * 0.5f;
            /* convert data-unit half-width to pixels */
            float x_scale = (float)(p->plot_area.x1 - p->plot_area.x0) /
                           (float)(p->x_axis.max - p->x_axis.min);
            float half_w_px = half_w * x_scale;
            float baseline = plm__map_y(p, 0.0);
            /* clamp baseline to plot area */
            if (baseline < (float)p->plot_area.y0) baseline = (float)p->plot_area.y0;
            if (baseline > (float)p->plot_area.y1) baseline = (float)p->plot_area.y1;

            int j;
            for (j = 0; j < bs->count; j++) {
                if (plm__isnan(bs->x_data[j]) || plm__isnan(bs->y_data[j]))
                    continue;

                float px_center = plm__map_x(p, bs->x_data[j]);
                float bar_top   = plm__map_y(p, bs->y_data[j]);

                float px_left  = px_center - half_w_px;
                float px_right = px_center + half_w_px;

                /* clip to plot area */
                if (px_right < (float)p->plot_area.x0 ||
                    px_left  > (float)p->plot_area.x1) continue;

                if (px_left  < (float)p->plot_area.x0) px_left  = (float)p->plot_area.x0;
                if (px_right > (float)p->plot_area.x1) px_right = (float)p->plot_area.x1;

                plm_irect r;
                r.x0 = (int)px_left;
                r.y0 = (int)(bar_top < baseline ? bar_top : baseline);
                r.x1 = (int)(px_right + 0.5f);
                r.y1 = (int)(bar_top < baseline ? baseline : bar_top);
                if (r.x1 <= r.x0) r.x1 = r.x0 + 1;
                if (r.y1 <= r.y0) r.y1 = r.y0 + 1;

                plm_fb_fill_rect(fb, r, bs->style.fill_color);

                if (bs->style.stroke_width > 0.0f) {
                    plm_color sc = bs->style.stroke_color;
                    int x;
                    for (x = r.x0; x < r.x1; x++) {
                        plm__set_pixel(fb, x, r.y0, sc);
                        plm__set_pixel(fb, x, r.y1 - 1, sc);
                    }
                    int y;
                    for (y = r.y0; y < r.y1; y++) {
                        plm__set_pixel(fb, r.x0, y, sc);
                        plm__set_pixel(fb, r.x1 - 1, y, sc);
                    }
                }
            }
        }
    }

    /* 10. draw title (centred within the cell) */
    if (p->title) {
        int tw = plm_text_width(p->title);
        int cx = cell_x0 + (cell_x1 - cell_x0) / 2;
        plm_draw_text(fb, cx - tw / 2, cell_y0 + 20, p->title, PLM_BLACK);
    }

    /* 11. draw axis labels */
    if (p->x_label) {
        int tw = plm_text_width(p->x_label);
        int cx = p->plot_area.x0 + (p->plot_area.x1 - p->plot_area.x0) / 2;
        plm_draw_text(fb, cx - tw / 2, cell_y1 - 5, p->x_label, PLM_BLACK);
    }
    if (p->y_label) {
        /* horizontal text near the left edge of the cell */
        int ty = p->plot_area.y0 + (p->plot_area.y1 - p->plot_area.y0) / 2;
        plm_draw_text(fb, cell_x0 + 2, ty, p->y_label, PLM_BLACK);
    }

    /* 12. draw tick labels */
    /* X-axis tick labels */
    if (p->x_axis.scale == PLM_CATEGORICAL && p->x_axis.num_categories > 0) {
        int ci;
        for (ci = 0; ci < p->x_axis.num_categories; ci++) {
            float px = plm__map_x(p, (double)ci);
            int ix = (int)(px + 0.5f);
            if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                const char *label = p->x_axis.categories
                                    ? p->x_axis.categories[ci] : NULL;
                if (!label) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", ci);
                    int tw = plm_text_width(buf);
                    plm_draw_text(fb, ix - tw / 2, p->plot_area.y1 + 15,
                                  buf, PLM_BLACK);
                } else {
                    int tw = plm_text_width(label);
                    plm_draw_text(fb, ix - tw / 2, p->plot_area.y1 + 15,
                                  label, PLM_BLACK);
                }
            }
        }
    } else {
        double range = p->x_axis.max - p->x_axis.min;
        double step  = plm__nice_step(range, p->x_axis.tick_hint > 0 ?
                                     p->x_axis.tick_hint : 5);
        double lo = floor(p->x_axis.min / step) * step;
        double v;
        for (v = lo; v <= p->x_axis.max + step * 0.5; v += step) {
            float px = plm__map_x(p, v);
            int ix = (int)(px + 0.5f);
            if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.4g", v);
                int tw = plm_text_width(buf);
                plm_draw_text(fb, ix - tw / 2, p->plot_area.y1 + 15,
                              buf, PLM_BLACK);
            }
        }
    }
    /* Y-axis tick labels */
    if (p->y_axis.scale == PLM_CATEGORICAL && p->y_axis.num_categories > 0) {
        int ci;
        for (ci = 0; ci < p->y_axis.num_categories; ci++) {
            float py = plm__map_y(p, (double)ci);
            int iy = (int)(py + 0.5f);
            if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                const char *label = p->y_axis.categories
                                    ? p->y_axis.categories[ci] : NULL;
                if (!label) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", ci);
                    plm_draw_text(fb, p->plot_area.x0 - plm_text_width(buf) - 4,
                                  iy + 3, buf, PLM_BLACK);
                } else {
                    plm_draw_text(fb, p->plot_area.x0 - plm_text_width(label) - 4,
                                  iy + 3, label, PLM_BLACK);
                }
            }
        }
    } else {
        double range = p->y_axis.max - p->y_axis.min;
        double step  = plm__nice_step(range, p->y_axis.tick_hint > 0 ?
                                     p->y_axis.tick_hint : 5);
        double lo = floor(p->y_axis.min / step) * step;
        double v;
        for (v = lo; v <= p->y_axis.max + step * 0.5; v += step) {
            float py = plm__map_y(p, v);
            int iy = (int)(py + 0.5f);
            if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.4g", v);
                plm_draw_text(fb, p->plot_area.x0 - plm_text_width(buf) - 4,
                              iy + 3, buf, PLM_BLACK);
            }
        }
    }
}

/* Public: render a single plot (fills the whole framebuffer). */
void plm_render(const plm_plot *p, plm_fb *fb) {
    plm_plot *pp = (plm_plot *)p;  /* cast away const for autorange writes */

    /* clear the whole framebuffer once */
    plm_fb_clear(fb, PLM_WHITE);

    /* delegate to the cell-based renderer */
    plm__render_plot_into(pp, fb, 0, 0, fb->width, fb->height);
}

/* ---- subplot / figure ---------------------------------------------- */

void plm_figure_init(plm_figure *fig, int nrows, int ncols) {
    int i, total;
    memset(fig, 0, sizeof(*fig));
    fig->nrows = nrows < 1 ? 1 : nrows;
    fig->ncols = ncols < 1 ? 1 : ncols;
    fig->hgap  = 20;
    fig->vgap  = 20;
    total = fig->nrows * fig->ncols;
    fig->plots = (plm_plot *)PLOTMINI_MALLOC((size_t)total * sizeof(plm_plot));
    for (i = 0; i < total; i++) {
        plm_plot *p = &fig->plots[i];
        plm_plot_init(p);
        /* Subplot cells have tighter space: reduce default margins. */
        p->margin_left   = 45;
        p->margin_top    = 22;
        p->margin_right  = 12;
        p->margin_bottom = 28;
    }
}

plm_plot *plm_figure_plot(plm_figure *fig, int row, int col) {
    if (!fig || !fig->plots) return NULL;
    if (row < 0 || row >= fig->nrows || col < 0 || col >= fig->ncols)
        return NULL;
    return &fig->plots[row * fig->ncols + col];
}

void plm_figure_render(const plm_figure *fig, plm_fb *fb) {
    int r, c;
    int cell_w, cell_h;
    int fig_w, fig_h;
    int fig_x0, fig_y0, fig_x1, fig_y1;

    if (!fig || !fig->plots || fig->nrows < 1 || fig->ncols < 1) return;

    /* 1. clear entire framebuffer once */
    plm_fb_clear(fb, PLM_WHITE);

    /* 2. compute figure content bounds (we use the full fb for simplicity) */
    fig_x0 = 0;
    fig_y0 = 0;
    fig_x1 = fb->width;
    fig_y1 = fb->height;
    fig_w  = fig_x1 - fig_x0;
    fig_h  = fig_y1 - fig_y0;

    /* 3. cell size: distribute remaining space after gaps */
    cell_w = (fig_w - (fig->ncols - 1) * fig->hgap) / fig->ncols;
    cell_h = (fig_h - (fig->nrows - 1) * fig->vgap) / fig->nrows;
    if (cell_w < 40) cell_w = 40;
    if (cell_h < 40) cell_h = 40;

    /* 4. render each cell */
    for (r = 0; r < fig->nrows; r++) {
        for (c = 0; c < fig->ncols; c++) {
            int cell_x0 = fig_x0 + c * (cell_w + fig->hgap);
            int cell_y0 = fig_y0 + r * (cell_h + fig->vgap);
            int cell_x1 = cell_x0 + cell_w;
            int cell_y1 = cell_y0 + cell_h;

            plm_plot *p = &fig->plots[r * fig->ncols + c];
            plm__render_plot_into(p, fb, cell_x0, cell_y0, cell_x1, cell_y1);
        }
    }
}

void plm_figure_reset(plm_figure *fig) {
    int i, total;
    if (!fig || !fig->plots) return;
    total = fig->nrows * fig->ncols;
    for (i = 0; i < total; i++) {
        plm_plot_reset(&fig->plots[i]);
    }
    PLOTMINI_FREE(fig->plots);
    memset(fig, 0, sizeof(*fig));
}

#endif /* PLOTMINI_IMPLEMENTATION */

/*
  ======================================================================
  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org/>
*/
