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

/* ---- theme colours (overridable via PLOTMINI_DARK_THEME) ---------- */
#ifdef PLOTMINI_DARK_THEME
  #define PLM_BG_COLOR    PLM_RGBA(0x1E,0x1E,0x1E,0xFF)  /* dark bg   */
  #define PLM_FG_COLOR    PLM_RGBA(0xCC,0xCC,0xCC,0xFF)  /* light fg  */
  #define PLM_GRID_COLOR  PLM_RGBA(0x3C,0x3C,0x3C,0xFF)  /* subtle grid */
#else
  #define PLM_BG_COLOR    PLM_WHITE
  #define PLM_FG_COLOR    PLM_BLACK
  #define PLM_GRID_COLOR  PLM_GREY(220)
#endif

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

/* ---- tick label formatter callback (forward) ----------------------- */
typedef void (*plm_tick_formatter)(char *buf, int buf_size, double value);

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
    /* custom tick label formatting */
    plm_tick_formatter tick_formatter; /* if non-NULL, called to format tick labels */
    int          font_scale;  /* 0 = use PLOTMINI_TEXT_SCALE; >0 = override */
} plm_axis;

/* ---- step mode (for line plots) ----------------------------------- */
typedef enum plm_step_mode {
    PLM_STEP_NONE = 0,  /* normal straight-line connection              */
    PLM_STEP_PRE,       /* step before: horizontal then vertical        */
    PLM_STEP_POST,      /* step after:  vertical then horizontal        */
    PLM_STEP_MID        /* step mid:    vertical-horizontal-vertical    */
} plm_step_mode;

/* ---- line style ---------------------------------------------------- */
typedef struct plm_line_style {
    plm_color      color;
    float          width;      /* 0.0..N  pixel width  (0 = hairline)  */
    const char    *legend;     /* optional legend label                 */
    plm_step_mode  step_mode;  /* PLM_STEP_NONE = normal line          */
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

/* ---- error bar style ----------------------------------------------- */
typedef struct plm_errorbar_style {
    plm_color    line_color;     /* color for error lines and connecting line */
    float        line_width;     /* 0 = hairline                              */
    float        cap_size;       /* cap width in pixels (0 = no caps)         */
    plm_color    marker_color;   /* marker fill color                         */
    float        marker_radius;  /* 0 = no markers                            */
    const char  *legend;
} plm_errorbar_style;

/* ---- error bar series ---------------------------------------------- */
typedef struct plm_errorbar_series {
    const float *x_data;
    const float *y_data;
    const float *x_err;       /* symmetric x error, or NULL = no x error      */
    const float *y_err;       /* symmetric y error, or NULL = no y error      */
    const float *x_err_low;   /* asymmetric lower x error (NULL → use x_err)  */
    const float *x_err_high;  /* asymmetric upper x error (NULL → use x_err)  */
    const float *y_err_low;   /* asymmetric lower y error (NULL → use y_err)  */
    const float *y_err_high;  /* asymmetric upper y error (NULL → use y_err)  */
    int          count;
    plm_errorbar_style style;
} plm_errorbar_series;

/* ---- band style ---------------------------------------------------- */
typedef struct plm_band_style {
    plm_color    fill_color;
    plm_color    stroke_color;
    float        stroke_width;  /* 0 = no stroke                             */
    const char  *legend;
} plm_band_style;

/* ---- band series (fill between two curves) ------------------------- */
typedef struct plm_band_series {
    const float *x_data;
    const float *y_lower;     /* lower bound                                 */
    const float *y_upper;     /* upper bound                                 */
    int          count;
    plm_band_style style;
} plm_band_series;

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

/* ---- stem style ---------------------------------------------------- */
typedef struct plm_stem_style {
    plm_color    line_color;     /* color for the vertical stem line          */
    float        line_width;     /* 0 = hairline                              */
    plm_color    marker_color;   /* marker fill color                         */
    float        marker_radius;  /* 0 = no markers                            */
    const char  *legend;
} plm_stem_style;

/* ---- stem series --------------------------------------------------- */
typedef struct plm_stem_series {
    const float     *x_data;
    const float     *y_data;
    int              count;
    double           baseline;    /* y-value of stem base (default = 0)       */
    plm_stem_style   style;
} plm_stem_series;

/* ---- legend position ----------------------------------------------- */
typedef enum plm_legend_pos {
    PLM_LEGEND_NONE = 0,      /* no legend                                  */
    PLM_LEGEND_TOP_RIGHT,     /* inside plot area, top-right corner          */
    PLM_LEGEND_TOP_LEFT,      /* inside plot area, top-left corner           */
    PLM_LEGEND_BOTTOM_RIGHT,  /* inside plot area, bottom-right corner       */
    PLM_LEGEND_BOTTOM_LEFT,   /* inside plot area, bottom-left corner        */
    PLM_LEGEND_OUTSIDE_RIGHT  /* outside plot area, to the right             */
} plm_legend_pos;

/* ---- a complete plot description (hybrid retained model) ----------- */
typedef struct plm_plot {
    /* axes */
    plm_axis     x_axis;
    plm_axis     y_axis;
    plm_axis     y_axis_right;  /* secondary (right-side) y-axis; min==max disables it */

    /* layout (pixels) -- margins can be set or auto-computed (-1)     */
    int          margin_left;
    int          margin_top;
    int          margin_right;
    int          margin_bottom;

    /* labels */
    const char  *title;
    const char  *x_label;
    const char  *y_label;
    const char  *y_label_right;  /* label for the right y-axis */

    /* legend */
    plm_legend_pos  legend_position;  /* PLM_LEGEND_NONE = no legend */

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

    plm_errorbar_series *errorbars;
    int                  errorbar_count;
    int                  errorbar_cap;  /* internal                    */

    plm_band_series *bands;
    int              band_count;
    int              band_cap;     /* internal                        */

    plm_stem_series *stems;
    int              stem_count;
    int              stem_cap;     /* internal                        */

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

/* Append an error bar series.  Data is *copied* internally.
   x_err and y_err may be NULL (no error in that direction).
   If cap_size > 0, caps are drawn at the end of error bars.
   If marker_radius > 0, markers are drawn at each data point.         */
void   plm_plot_add_errorbar(plm_plot *p,
                              const float *x, const float *y,
                              const float *x_err, const float *y_err,
                              int count,
                              plm_errorbar_style style);

/* Append a band (filled region) between two curves.
   Data is *copied* internally.  y_lower and y_upper define the
   lower and upper boundaries of the filled region.                     */
void   plm_plot_add_band(plm_plot *p,
                          const float *x, const float *y_lower,
                          const float *y_upper,
                          int count,
                          plm_band_style style);

/* Append an error bar series with asymmetric errors.
   Data is *copied* internally.
   Pass NULL for any of x_err_low/x_err_high to skip x error bars;
   pass NULL for any of y_err_low/y_err_high to skip y error bars.
   To get symmetric errors in a direction, pass the same array for
   low and high.                                                        */
void   plm_plot_add_errorbar_asym(plm_plot *p,
                                   const float *x, const float *y,
                                   const float *x_err_low, const float *x_err_high,
                                   const float *y_err_low, const float *y_err_high,
                                   int count,
                                   plm_errorbar_style style);

/* Append a stem series.  Data is *copied* internally.
   Stems are vertical lines from a baseline (default 0) to each
   (x, y) data point, with an optional marker at the top.               */
void   plm_plot_add_stem(plm_plot *p,
                          const float *x, const float *y, int count,
                          double baseline, plm_stem_style style);

/* ---- rendering ----------------------------------------------------- */

/* Save framebuffer as 24-bit BMP.  Returns 0 on success, non-zero on error. */
int    plm_fb_save_bmp(const plm_fb *fb, const char *path);

/* Render the current plot description into a framebuffer.
   Layout (margins, tick labels) is recomputed every call, so you can
   call this repeatedly with different fb sizes or updated data. */
void   plm_render(const plm_plot *p, plm_fb *fb);

/* ---- built-in tick formatters -------------------------------------- */

/* Default: "%.4g" */
void   plm_tick_fmt_default(char *buf, int sz, double v);
/* Scientific: "%.2e" */
void   plm_tick_fmt_sci(char *buf, int sz, double v);
/* Integer: "%.0f" */
void   plm_tick_fmt_int(char *buf, int sz, double v);
/* Percentage: "%.0f%%" (multiplies v by 100) */
void   plm_tick_fmt_pct(char *buf, int sz, double v);
/* Log10: "10^n" for exact powers of 10, else "%.4g" */
void   plm_tick_fmt_log10(char *buf, int sz, double v);
/* Currency: "$%.2f" */
void   plm_tick_fmt_usd(char *buf, int sz, double v);

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

int plm_fb_save_bmp(const plm_fb *fb, const char *path) {
    if (!fb || !fb->pixels || fb->width <= 0 || fb->height <= 0) return 1;
    FILE *f = fopen(path, "wb");
    if (!f) return 1;
    int w = fb->width, h = fb->height, row_size = (w * 3 + 3) & ~3;
    unsigned char hdr[54] = {0};
    hdr[0] = 'B'; hdr[1] = 'M';
    int fsize = 54 + row_size * h;
    hdr[2] = (unsigned char)fsize; hdr[3] = (unsigned char)(fsize>>8);
    hdr[4] = (unsigned char)(fsize>>16); hdr[5] = (unsigned char)(fsize>>24);
    hdr[10] = 54;
    hdr[14] = 40;
    hdr[18] = (unsigned char)w; hdr[19] = (unsigned char)(w>>8);
    hdr[22] = (unsigned char)h; hdr[23] = (unsigned char)(h>>8);
    hdr[26] = 1;
    hdr[28] = 24;
    fwrite(hdr, 54, 1, f);
    unsigned char *row = (unsigned char *)malloc((size_t)row_size);
    if (!row) { fclose(f); return 1; }
    int y;
    for (y = h - 1; y >= 0; y--) {
        const unsigned char *src = fb->pixels + y * fb->stride;
        int x;
        if (fb->bpp == 4) {
            for (x = 0; x < w; x++) {
                row[x*3+0] = src[x*4+2];
                row[x*3+1] = src[x*4+1];
                row[x*3+2] = src[x*4+0];
            }
        } else {
            for (x = 0; x < w; x++) {
                row[x*3+0] = row[x*3+1] = row[x*3+2] = src[x];
            }
        }
        fwrite(row, (size_t)row_size, 1, f);
    }
    free(row);
    fclose(f);
    return 0;
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

/* font metrics (always available, even with PLOTMINI_NO_FONT) */
#define PLM_FONT_W  5
#define PLM_FONT_H  7
#define PLM_FONT_ADVANCE 6   /* width + 1 px spacing */

/* override before #include to scale all text (default 1 = 5×7 px) */
#ifndef PLOTMINI_TEXT_SCALE
#define PLOTMINI_TEXT_SCALE 1
#endif

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

static int plm__text_width_s(const char *s, int scale) {
    int w = 0, S = scale > 0 ? scale : PLOTMINI_TEXT_SCALE;
    while (*s) {
        int ch = (unsigned char)*s;
        if (ch >= 32 && ch < 127) w += PLM_FONT_ADVANCE * S;
        s++;
    }
    return w;
}

static int plm__text_height_s(int scale) {
    int S = scale > 0 ? scale : PLOTMINI_TEXT_SCALE;
    return PLM_FONT_H * S;
}

int plm_text_width(const char *s) {
    return plm__text_width_s(s, PLOTMINI_TEXT_SCALE);
}

int plm_text_height(void) {
    return plm__text_height_s(PLOTMINI_TEXT_SCALE);
}

static void plm__draw_text_s(plm_fb *fb, int x, int y, const char *s,
                              plm_color c, int scale) {
    int S = scale > 0 ? scale : PLOTMINI_TEXT_SCALE;
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
                        int dx, dy;
                        int px = x + col * S;
                        int py = y - PLM_FONT_H * S + 1 + row * S;
                        for (dy = 0; dy < S; dy++) {
                            for (dx = 0; dx < S; dx++) {
                                plm__set_pixel(fb, px + dx, py + dy, c);
                            }
                        }
                    }
                }
            }
            x += PLM_FONT_ADVANCE * S;
        }
        s++;
    }
}

void plm_draw_text(plm_fb *fb, int x, int y, const char *s, plm_color c) {
    plm__draw_text_s(fb, x, y, s, c, PLOTMINI_TEXT_SCALE);
}

/* Draw a single character rotated 90° CW at (x,y) which is the
   top-left corner of the rotated glyph (PLM_FONT_H × PLM_FONT_W). */
static void plm__draw_char_rot90(plm_fb *fb, int x, int y, int ch,
                                  plm_color c, int scale) {
    if (ch < 32 || ch >= 127) return;
    const unsigned char *glyph = plm__font_data + (ch - 32) * PLM_FONT_W;
    int col, row;
    for (col = 0; col < PLM_FONT_W; col++) {
        unsigned char bits = glyph[col];
        for (row = 0; row < PLM_FONT_H; row++) {
            if (bits & (1u << row)) {
                /* 90° CW: (col,row) → (PLM_FONT_H-1-row, col) */
                int rx = (PLM_FONT_H - 1 - row) * scale;
                int ry = col * scale;
                int dx, dy;
                for (dy = 0; dy < scale; dy++) {
                    for (dx = 0; dx < scale; dx++) {
                        plm__set_pixel(fb, x + rx + dx, y + ry + dy, c);
                    }
                }
            }
        }
    }
}

#else /* PLOTMINI_NO_FONT */

static int plm__text_width_s(const char *s, int scale)  { (void)s; (void)scale; return 0; }
static int plm__text_height_s(int scale)          { (void)scale; return 0; }
int  plm_text_width(const char *s)  { (void)s; return 0; }
int  plm_text_height(void)          { return 0; }
void plm_draw_text(plm_fb *fb, int x, int y, const char *s, plm_color c) {
    (void)fb; (void)x; (void)y; (void)s; (void)c;
}
static void plm__draw_char_rot90(plm_fb *fb, int x, int y, int ch,
                                  plm_color c, int scale) {
    (void)fb; (void)x; (void)y; (void)ch; (void)c; (void)scale;
}

#endif /* PLOTMINI_NO_FONT */

/* ---- built-in tick formatters (implementations) -------------------- */

void plm_tick_fmt_default(char *buf, int sz, double v) {
    (void)sz; snprintf(buf, 32, "%.4g", v);
}
void plm_tick_fmt_sci(char *buf, int sz, double v) {
    (void)sz; snprintf(buf, 32, "%.2e", v);
}
void plm_tick_fmt_int(char *buf, int sz, double v) {
    (void)sz; snprintf(buf, 32, "%.0f", v);
}
void plm_tick_fmt_pct(char *buf, int sz, double v) {
    (void)sz; snprintf(buf, 32, "%.0f%%%%", v * 100.0);
}
void plm_tick_fmt_log10(char *buf, int sz, double v) {
    (void)sz;
    if (v >= 1.0 && v == pow(10.0, floor(log10(v))))
        snprintf(buf, 32, "10^%d", (int)log10(v));
    else
        snprintf(buf, 32, "%.4g", v);
}
void plm_tick_fmt_usd(char *buf, int sz, double v) {
    (void)sz; snprintf(buf, 32, "$%.2f", v);
}

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

    p->margin_left      = 70;
    p->margin_top       = 48;
    p->margin_right     = 30;
    p->margin_bottom    = 55;

    p->title   = NULL;
    p->x_label = NULL;
    p->y_label = NULL;
}

void plm_plot_reset(plm_plot *p) {
    int i;
    for (i = 0; i < p->line_count; i++) {
        PLOTMINI_FREE((void*)p->lines[i].x_data);
        PLOTMINI_FREE((void*)p->lines[i].y_data);
    }
    if (p->lines)     PLOTMINI_FREE(p->lines);

    for (i = 0; i < p->hist_count; i++) {
        PLOTMINI_FREE((void*)p->hists[i].data);
    }
    if (p->hists)     PLOTMINI_FREE(p->hists);

    for (i = 0; i < p->scatter_count; i++) {
        PLOTMINI_FREE((void*)p->scatters[i].x_data);
        PLOTMINI_FREE((void*)p->scatters[i].y_data);
    }
    if (p->scatters)  PLOTMINI_FREE(p->scatters);

    for (i = 0; i < p->bar_count; i++) {
        PLOTMINI_FREE((void*)p->bars[i].x_data);
        PLOTMINI_FREE((void*)p->bars[i].y_data);
    }
    if (p->bars)      PLOTMINI_FREE(p->bars);

    for (i = 0; i < p->errorbar_count; i++) {
        PLOTMINI_FREE((void*)p->errorbars[i].x_data);
        PLOTMINI_FREE((void*)p->errorbars[i].y_data);
        PLOTMINI_FREE((void*)p->errorbars[i].x_err);
        PLOTMINI_FREE((void*)p->errorbars[i].y_err);
        PLOTMINI_FREE((void*)p->errorbars[i].x_err_low);
        PLOTMINI_FREE((void*)p->errorbars[i].x_err_high);
        PLOTMINI_FREE((void*)p->errorbars[i].y_err_low);
        PLOTMINI_FREE((void*)p->errorbars[i].y_err_high);
    }
    if (p->errorbars) PLOTMINI_FREE(p->errorbars);

    for (i = 0; i < p->band_count; i++) {
        PLOTMINI_FREE((void*)p->bands[i].x_data);
        PLOTMINI_FREE((void*)p->bands[i].y_lower);
        PLOTMINI_FREE((void*)p->bands[i].y_upper);
    }
    if (p->bands)     PLOTMINI_FREE(p->bands);

    for (i = 0; i < p->stem_count; i++) {
        PLOTMINI_FREE((void*)p->stems[i].x_data);
        PLOTMINI_FREE((void*)p->stems[i].y_data);
    }
    if (p->stems)     PLOTMINI_FREE(p->stems);

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

void plm_plot_add_errorbar(plm_plot *p,
                            const float *x, const float *y,
                            const float *x_err, const float *y_err,
                            int count,
                            plm_errorbar_style style) {
    if (count <= 0) return;
    if (p->errorbar_count + 1 > p->errorbar_cap) {
        int new_cap = p->errorbar_cap ? p->errorbar_cap * 2 : 4;
        p->errorbars = (plm_errorbar_series *)realloc(p->errorbars,
                             (size_t)new_cap * sizeof(plm_errorbar_series));
        p->errorbar_cap = new_cap;
    }
    plm_errorbar_series *es = &p->errorbars[p->errorbar_count++];
    es->x_data = (const float *)malloc((size_t)count * sizeof(float));
    es->y_data = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)es->x_data, x, (size_t)count * sizeof(float));
    memcpy((void*)es->y_data, y, (size_t)count * sizeof(float));
    if (x_err) {
        es->x_err = (const float *)malloc((size_t)count * sizeof(float));
        memcpy((void*)es->x_err, x_err, (size_t)count * sizeof(float));
    } else {
        es->x_err = NULL;
    }
    if (y_err) {
        es->y_err = (const float *)malloc((size_t)count * sizeof(float));
        memcpy((void*)es->y_err, y_err, (size_t)count * sizeof(float));
    } else {
        es->y_err = NULL;
    }
    es->x_err_low  = NULL;
    es->x_err_high = NULL;
    es->y_err_low  = NULL;
    es->y_err_high = NULL;
    es->count = count;
    es->style = style;
}

void plm_plot_add_errorbar_asym(plm_plot *p,
                                 const float *x, const float *y,
                                 const float *x_err_low, const float *x_err_high,
                                 const float *y_err_low, const float *y_err_high,
                                 int count,
                                 plm_errorbar_style style) {
    if (count <= 0) return;
    if (p->errorbar_count + 1 > p->errorbar_cap) {
        int new_cap = p->errorbar_cap ? p->errorbar_cap * 2 : 4;
        p->errorbars = (plm_errorbar_series *)realloc(p->errorbars,
                             (size_t)new_cap * sizeof(plm_errorbar_series));
        p->errorbar_cap = new_cap;
    }
    plm_errorbar_series *es = &p->errorbars[p->errorbar_count++];
    es->x_data = (const float *)malloc((size_t)count * sizeof(float));
    es->y_data = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)es->x_data, x, (size_t)count * sizeof(float));
    memcpy((void*)es->y_data, y, (size_t)count * sizeof(float));
    es->x_err = NULL;
    es->y_err = NULL;
    if (x_err_low) {
        es->x_err_low = (const float *)malloc((size_t)count * sizeof(float));
        memcpy((void*)es->x_err_low, x_err_low, (size_t)count * sizeof(float));
    } else { es->x_err_low = NULL; }
    if (x_err_high) {
        es->x_err_high = (const float *)malloc((size_t)count * sizeof(float));
        memcpy((void*)es->x_err_high, x_err_high, (size_t)count * sizeof(float));
    } else { es->x_err_high = NULL; }
    if (y_err_low) {
        es->y_err_low = (const float *)malloc((size_t)count * sizeof(float));
        memcpy((void*)es->y_err_low, y_err_low, (size_t)count * sizeof(float));
    } else { es->y_err_low = NULL; }
    if (y_err_high) {
        es->y_err_high = (const float *)malloc((size_t)count * sizeof(float));
        memcpy((void*)es->y_err_high, y_err_high, (size_t)count * sizeof(float));
    } else { es->y_err_high = NULL; }
    es->count = count;
    es->style = style;
}

void plm_plot_add_stem(plm_plot *p,
                        const float *x, const float *y, int count,
                        double baseline, plm_stem_style style) {
    if (count <= 0) return;
    if (p->stem_count + 1 > p->stem_cap) {
        int new_cap = p->stem_cap ? p->stem_cap * 2 : 4;
        p->stems = (plm_stem_series *)realloc(p->stems,
                        (size_t)new_cap * sizeof(plm_stem_series));
        p->stem_cap = new_cap;
    }
    plm_stem_series *ss = &p->stems[p->stem_count++];
    ss->x_data = (const float *)malloc((size_t)count * sizeof(float));
    ss->y_data = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)ss->x_data, x, (size_t)count * sizeof(float));
    memcpy((void*)ss->y_data, y, (size_t)count * sizeof(float));
    ss->count    = count;
    ss->baseline = baseline;
    ss->style    = style;
}

void plm_plot_add_band(plm_plot *p,
                        const float *x, const float *y_lower,
                        const float *y_upper,
                        int count,
                        plm_band_style style) {
    if (count <= 0) return;
    if (p->band_count + 1 > p->band_cap) {
        int new_cap = p->band_cap ? p->band_cap * 2 : 4;
        p->bands = (plm_band_series *)realloc(p->bands,
                        (size_t)new_cap * sizeof(plm_band_series));
        p->band_cap = new_cap;
    }
    plm_band_series *bs = &p->bands[p->band_count++];
    bs->x_data   = (const float *)malloc((size_t)count * sizeof(float));
    bs->y_lower  = (const float *)malloc((size_t)count * sizeof(float));
    bs->y_upper  = (const float *)malloc((size_t)count * sizeof(float));
    memcpy((void*)bs->x_data,   x,        (size_t)count * sizeof(float));
    memcpy((void*)bs->y_lower,  y_lower,  (size_t)count * sizeof(float));
    memcpy((void*)bs->y_upper,  y_upper,  (size_t)count * sizeof(float));
    bs->count = count;
    bs->style = style;
}

/* ---- auto-range ---------------------------------------------------- */

static void plm__axis_autorange(plm_axis *ax,
                                 const plm_line_series *lines, int lc,
                                 const plm_hist_series *hists, int hc,
                                 const plm_scatter_series *scatters, int sc,
                                 const plm_bar_series *bars, int bc,
                                 const plm_errorbar_series *errorbars, int ebc,
                                 const plm_band_series *bands, int bdc,
                                 const plm_stem_series *stems, int stc,
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

    /* errorbar series (symmetric + asymmetric) */
    for (i = 0; i < ebc; i++) {
        const float *data = is_y ? errorbars[i].y_data : errorbars[i].x_data;
        int j;
        for (j = 0; j < errorbars[i].count; j++) {
            float v = data[j];
            if (!plm__isnan(v)) {
                if ((double)v < lo) lo = (double)v;
                if ((double)v > hi) hi = (double)v;
                found = 1;
            }
            if (is_y) {
                double err_lo = 0.0, err_hi = 0.0;
                if (errorbars[i].y_err_low && !plm__isnan(errorbars[i].y_err_low[j]))
                    err_lo = (double)errorbars[i].y_err_low[j];
                else if (errorbars[i].y_err && !plm__isnan(errorbars[i].y_err[j]))
                    err_lo = (double)errorbars[i].y_err[j];
                if (errorbars[i].y_err_high && !plm__isnan(errorbars[i].y_err_high[j]))
                    err_hi = (double)errorbars[i].y_err_high[j];
                else if (errorbars[i].y_err && !plm__isnan(errorbars[i].y_err[j]))
                    err_hi = (double)errorbars[i].y_err[j];
                double hi2 = (double)v + err_hi;
                double lo2 = (double)v - err_lo;
                if (hi2 > hi) hi = hi2;
                if (lo2 < lo) lo = lo2;
            } else {
                double err_lo = 0.0, err_hi = 0.0;
                if (errorbars[i].x_err_low && !plm__isnan(errorbars[i].x_err_low[j]))
                    err_lo = (double)errorbars[i].x_err_low[j];
                else if (errorbars[i].x_err && !plm__isnan(errorbars[i].x_err[j]))
                    err_lo = (double)errorbars[i].x_err[j];
                if (errorbars[i].x_err_high && !plm__isnan(errorbars[i].x_err_high[j]))
                    err_hi = (double)errorbars[i].x_err_high[j];
                else if (errorbars[i].x_err && !plm__isnan(errorbars[i].x_err[j]))
                    err_hi = (double)errorbars[i].x_err[j];
                double hi2 = (double)v + err_hi;
                double lo2 = (double)v - err_lo;
                if (hi2 > hi) hi = hi2;
                if (lo2 < lo) lo = lo2;
            }
        }
    }

    /* stem series */
    for (i = 0; i < stc; i++) {
        const float *data = is_y ? stems[i].y_data : stems[i].x_data;
        int j;
        for (j = 0; j < stems[i].count; j++) {
            float v = data[j];
            if (!plm__isnan(v)) {
                if ((double)v < lo) lo = (double)v;
                if ((double)v > hi) hi = (double)v;
                found = 1;
            }
        }
        if (is_y) {
            double bl = stems[i].baseline;
            if (bl < lo) lo = bl;
            if (bl > hi) hi = bl;
        }
    }

    /* band series */
    for (i = 0; i < bdc; i++) {
        int j;
        if (is_y) {
            for (j = 0; j < bands[i].count; j++) {
                float vl = bands[i].y_lower[j];
                float vu = bands[i].y_upper[j];
                if (!plm__isnan(vl)) {
                    if ((double)vl < lo) lo = (double)vl;
                    if ((double)vl > hi) hi = (double)vl;
                    found = 1;
                }
                if (!plm__isnan(vu)) {
                    if ((double)vu < lo) lo = (double)vu;
                    if ((double)vu > hi) hi = (double)vu;
                    found = 1;
                }
            }
        } else {
            for (j = 0; j < bands[i].count; j++) {
                float v = bands[i].x_data[j];
                if (!plm__isnan(v)) {
                    if ((double)v < lo) lo = (double)v;
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
        if (ax->scale == PLM_LOG) {
            /* log scale: ensure min > 0, use multiplicative padding */
            if (lo <= 0.0) lo = hi * 0.001;
            if (lo <= 0.0) lo = 1e-6;
            double log_lo = log10(lo);
            double log_hi = log10(hi);
            double log_pad = (log_hi - log_lo) * 0.05;
            if (log_pad < 0.02) log_pad = 0.02;
            ax->min = pow(10.0, log_lo - log_pad);
            ax->max = pow(10.0, log_hi + log_pad);
        } else {
            double pad = (hi - lo) * 0.05;
            if (pad < 1e-12) pad = 1.0;
            ax->min = lo - pad;
            ax->max = hi + pad;
        }
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
    double t;
    if (p->x_axis.scale == PLM_LOG) {
        double log_min = log10(p->x_axis.min);
        double log_max = log10(p->x_axis.max);
        double log_val = log10(val > 0.0 ? val : p->x_axis.min);
        t = (log_val - log_min) / (log_max - log_min);
    } else {
        t = (val - p->x_axis.min) / (p->x_axis.max - p->x_axis.min);
    }
    return (float)(p->plot_area.x0 + t * (p->plot_area.x1 - p->plot_area.x0));
}

static float plm__map_y(const plm_plot *p, double val) {
    double t;
    plm_axis ax = p->y_axis;
    if (ax.scale == PLM_LOG) {
        double log_min = log10(ax.min);
        double log_max = log10(ax.max);
        double log_val = log10(val > 0.0 ? val : ax.min);
        t = (log_val - log_min) / (log_max - log_min);
    } else {
        t = (val - ax.min) / (ax.max - ax.min);
    }
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
                             p->bars, p->bar_count,
                             p->errorbars, p->errorbar_count,
                             p->bands, p->band_count,
                             p->stems, p->stem_count, 0);
    }
    if (p->y_axis.min == p->y_axis.max) {
        plm__axis_autorange(&p->y_axis, p->lines, p->line_count,
                             p->hists, p->hist_count,
                             p->scatters, p->scatter_count,
                             p->bars, p->bar_count,
                             p->errorbars, p->errorbar_count,
                             p->bands, p->band_count,
                             p->stems, p->stem_count, 1);
    }
    /* autorange for right y-axis (if enabled) */
    if (p->y_axis_right.min == p->y_axis_right.max &&
        (p->y_axis_right.scale != PLM_LINEAR ||
         p->y_axis_right.tick_hint != 0 ||
         p->y_axis_right.label != NULL)) {
        plm__axis_autorange(&p->y_axis_right, p->lines, p->line_count,
                             p->hists, p->hist_count,
                             p->scatters, p->scatter_count,
                             p->bars, p->bar_count,
                             p->errorbars, p->errorbar_count,
                             p->bands, p->band_count,
                             p->stems, p->stem_count, 1);
    }

    /* 1b. pre-scan labels to expand margins if needed */
    {
        const int S = PLOTMINI_TEXT_SCALE;
        int y_fs   = p->y_axis.font_scale > 0 ? p->y_axis.font_scale : S;
        int ry_fs  = p->y_axis_right.font_scale > 0 ? p->y_axis_right.font_scale : S;
        int char_w = PLM_FONT_H * S;
        int tick_len = 4 * S;
        int gap      = 3 * S;

        int max_ytick_w = 0;
        {
            if (p->y_axis.scale == PLM_CATEGORICAL && p->y_axis.num_categories > 0) {
                int ci;
                for (ci = 0; ci < p->y_axis.num_categories; ci++) {
                    const char *label = p->y_axis.categories
                                        ? p->y_axis.categories[ci] : NULL;
                    char buf[32];
                    if (!label) { snprintf(buf, sizeof(buf), "%d", ci); label = buf; }
                    int w = plm__text_width_s(label, y_fs);
                    if (w > max_ytick_w) max_ytick_w = w;
                }
            } else if (p->y_axis.scale == PLM_LOG) {
                double log_min = log10(p->y_axis.min);
                double log_max = log10(p->y_axis.max);
                double v;
                for (v = floor(log_min); v <= log_max + 0.5; v += 1.0) {
                    double val = pow(10.0, v);
                    char buf[32];
                    if (p->y_axis.tick_formatter)
                        p->y_axis.tick_formatter(buf, sizeof(buf), val);
                    else snprintf(buf, sizeof(buf), "%.4g", val);
                    int w = plm__text_width_s(buf, y_fs);
                    if (w > max_ytick_w) max_ytick_w = w;
                }
            } else {
                double range = p->y_axis.max - p->y_axis.min;
                if (range > 0.0) {
                    double step = plm__nice_step(range, p->y_axis.tick_hint > 0 ?
                                                 p->y_axis.tick_hint : 5);
                    double lo = floor(p->y_axis.min / step) * step;
                    double v;
                    for (v = lo; v <= p->y_axis.max + step * 0.5; v += step) {
                        char buf[32];
                        if (p->y_axis.tick_formatter)
                            p->y_axis.tick_formatter(buf, sizeof(buf), v);
                        else snprintf(buf, sizeof(buf), "%.4g", v);
                        int w = plm__text_width_s(buf, y_fs);
                        if (w > max_ytick_w) max_ytick_w = w;
                    }
                }
            }
        }

        int max_rytick_w = 0;
        int has_right = (p->y_axis_right.min != p->y_axis_right.max ||
                         p->y_axis_right.label != NULL ||
                         p->y_label_right != NULL);
        if (has_right) {
            double range = p->y_axis_right.max - p->y_axis_right.min;
            if (range <= 0.0) range = 1.0;
            double step = plm__nice_step(range, p->y_axis_right.tick_hint > 0 ?
                                         p->y_axis_right.tick_hint : 5);
            double lo = floor(p->y_axis_right.min / step) * step;
            double v;
            for (v = lo; v <= p->y_axis_right.max + step * 0.5; v += step) {
                char buf[32];
                if (p->y_axis_right.tick_formatter)
                    p->y_axis_right.tick_formatter(buf, sizeof(buf), v);
                else snprintf(buf, sizeof(buf), "%.4g", v);
                int w = plm__text_width_s(buf, ry_fs);
                if (w > max_rytick_w) max_rytick_w = w;
            }
        }

        /* expand margins so labels fit */
        {
            int need_left = gap + char_w + gap + max_ytick_w + gap + tick_len;
            if (p->y_label && need_left > p->margin_left)
                p->margin_left = need_left;
            else if (max_ytick_w + gap + tick_len > p->margin_left)
                p->margin_left = max_ytick_w + gap + tick_len;
        }
        if (has_right) {
            int need_right = tick_len + gap + max_rytick_w + gap + char_w + gap;
            if (need_right > p->margin_right)
                p->margin_right = need_right;
        }
        /* expand bottom margin for x-axis tick labels + label */
        {
            int x_th = plm__text_height_s(p->x_axis.font_scale > 0 ? p->x_axis.font_scale : S);
            int label_h = PLM_FONT_H * S;
            int need_bottom = tick_len + gap + x_th + gap;
            if (p->x_label)
                need_bottom += label_h + gap;
            if (need_bottom > p->margin_bottom)
                p->margin_bottom = need_bottom;
        }
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
        plm_fb_fill_rect(fb, bg, PLM_BG_COLOR);
    }

    /* 4. draw grid lines (X axis - major) */
    if (p->x_axis.grid >= 1) {
        plm_color grid_c = PLM_GRID_COLOR;
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
        plm_color grid_c = PLM_GRID_COLOR;
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

    /* 6. draw axis spines (the L-shape, plus right if y_axis_right enabled) */
    {
        plm_color ax_c = PLM_FG_COLOR;
        /* bottom */
        { plm_irect r = { p->plot_area.x0, p->plot_area.y1 - 1,
                          p->plot_area.x1, p->plot_area.y1 };
          plm_fb_fill_rect(fb, r, ax_c); }
        /* left */
        { plm_irect r = { p->plot_area.x0, p->plot_area.y0,
                          p->plot_area.x0 + 1, p->plot_area.y1 };
          plm_fb_fill_rect(fb, r, ax_c); }
        /* right (secondary y-axis) */
        if (p->y_axis_right.min != p->y_axis_right.max ||
            p->y_axis_right.label != NULL) {
            { plm_irect r = { p->plot_area.x1 - 1, p->plot_area.y0,
                              p->plot_area.x1, p->plot_area.y1 };
              plm_fb_fill_rect(fb, r, ax_c); }
        }
    }

    /* 6b. draw band series (fill between two curves) */
    {
        int si;
        for (si = 0; si < p->band_count; si++) {
            const plm_band_series *bs = &p->bands[si];
            if (bs->count < 2) continue;
            int n = bs->count, i;
            float band_ymin = 1e9f, band_ymax = -1e9f;
            float *poly_x = (float *)malloc((size_t)(2*n) * sizeof(float));
            float *poly_y = (float *)malloc((size_t)(2*n) * sizeof(float));
            if (!poly_x || !poly_y) {
                if (poly_x) free(poly_x);
                if (poly_y) free(poly_y);
                continue;
            }
            int pcount = 0;
            /* upper curve left to right */
            for (i = 0; i < n; i++) {
                if (plm__isnan(bs->x_data[i]) ||
                    plm__isnan(bs->y_upper[i])) continue;
                float px = plm__map_x(p, bs->x_data[i]);
                float py = plm__map_y(p, bs->y_upper[i]);
                poly_x[pcount] = px; poly_y[pcount] = py;
                if (py < band_ymin) band_ymin = py;
                if (py > band_ymax) band_ymax = py;
                pcount++;
            }
            /* lower curve right to left */
            for (i = n - 1; i >= 0; i--) {
                if (plm__isnan(bs->x_data[i]) ||
                    plm__isnan(bs->y_lower[i])) continue;
                float px = plm__map_x(p, bs->x_data[i]);
                float py = plm__map_y(p, bs->y_lower[i]);
                poly_x[pcount] = px; poly_y[pcount] = py;
                if (py < band_ymin) band_ymin = py;
                if (py > band_ymax) band_ymax = py;
                pcount++;
            }
            if (pcount < 3) { free(poly_x); free(poly_y); continue; }
            /* clip to plot area */
            int scan_y0 = (int)band_ymin;
            int scan_y1 = (int)(band_ymax + 0.9999f);
            if (scan_y0 < p->plot_area.y0) scan_y0 = p->plot_area.y0;
            if (scan_y1 > p->plot_area.y1) scan_y1 = p->plot_area.y1;
            /* scanline fill (pre-allocate intersection buffer once) */
            {
                int y;
                float *ix_vals = (float *)malloc((size_t)pcount *
                                                 sizeof(float));
                if (!ix_vals) { free(poly_x); free(poly_y); continue; }
                for (y = scan_y0; y < scan_y1; y++) {
                    int ix_count = 0;
                    int j;
                    for (j = 0; j < pcount; j++) {
                        int j2 = (j + 1) % pcount;
                        float y0 = poly_y[j], y1 = poly_y[j2];
                        float x0 = poly_x[j], x1 = poly_x[j2];
                        float sy = (float)y + 0.5f;
                        if ((y0 <= sy && y1 > sy) ||
                            (y1 <= sy && y0 > sy)) {
                            float t = (sy - y0) / (y1 - y0);
                            ix_vals[ix_count++] = x0 + t * (x1 - x0);
                        }
                    }
                    /* sort (bubble, small n) */
                    { int a, b;
                      for (a = 0; a < ix_count-1; a++)
                        for (b = a+1; b < ix_count; b++)
                          if (ix_vals[a] > ix_vals[b]) {
                            float t = ix_vals[a];
                            ix_vals[a] = ix_vals[b];
                            ix_vals[b] = t;
                          }
                    }
                    /* fill pairs */
                    { int k;
                      for (k = 0; k + 1 < ix_count; k += 2) {
                        float xl = ix_vals[k], xr = ix_vals[k+1];
                        if (xl < (float)p->plot_area.x0)
                            xl = (float)p->plot_area.x0;
                        if (xr > (float)p->plot_area.x1)
                            xr = (float)p->plot_area.x1;
                        int xli = (int)xl, xri = (int)(xr+0.9999f);
                        int x;
                        for (x = xli; x < xri; x++)
                            plm__blend_pixel(fb, x, y,
                                bs->style.fill_color,
                                bs->style.fill_color.a);
                      }
                    }
                }
                free(ix_vals);
            }
            /* stroke boundaries */
            if (bs->style.stroke_width > 0.0f) {
                plm_color sc = bs->style.stroke_color;
                float sw = bs->style.stroke_width;
                /* upper */
                { int seg_start = -1;
                  for (i = 0; i < n; i++) {
                    if (plm__isnan(bs->x_data[i]) ||
                        plm__isnan(bs->y_upper[i]))
                        { seg_start = -1; continue; }
                    if (seg_start >= 0) {
                        float px0=plm__map_x(p,bs->x_data[i-1]);
                        float py0=plm__map_y(p,bs->y_upper[i-1]);
                        float px1=plm__map_x(p,bs->x_data[i]);
                        float py1=plm__map_y(p,bs->y_upper[i]);
                        plm__wu_line_thick(fb,px0,py0,px1,py1,sc,sw);
                    }
                    if (seg_start < 0) seg_start = i;
                  }
                }
                /* lower */
                { int seg_start = -1;
                  for (i = 0; i < n; i++) {
                    if (plm__isnan(bs->x_data[i]) ||
                        plm__isnan(bs->y_lower[i]))
                        { seg_start = -1; continue; }
                    if (seg_start >= 0) {
                        float px0=plm__map_x(p,bs->x_data[i-1]);
                        float py0=plm__map_y(p,bs->y_lower[i-1]);
                        float px1=plm__map_x(p,bs->x_data[i]);
                        float py1=plm__map_y(p,bs->y_lower[i]);
                        plm__wu_line_thick(fb,px0,py0,px1,py1,sc,sw);
                    }
                    if (seg_start < 0) seg_start = i;
                  }
                }
            }
            free(poly_x);
            free(poly_y);
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

    /* 7. draw line series */
    {
        int si;
        for (si = 0; si < p->line_count; si++) {
            const plm_line_series *ls = &p->lines[si];
            int i;
            int seg_start = -1;
            float prev_px = 0.0f, prev_py = 0.0f;
            for (i = 0; i < ls->count; i++) {
                /* NaN gap handling */
                if (plm__isnan(ls->x_data[i]) || plm__isnan(ls->y_data[i])) {
                    seg_start = -1;
                    continue;
                }
                float px = plm__map_x(p, ls->x_data[i]);
                float py = plm__map_y(p, ls->y_data[i]);
                if (seg_start >= 0) {
                    if (ls->style.step_mode == PLM_STEP_NONE) {
                        float px0 = prev_px, py0 = prev_py;
                        float px1 = px, py1 = py;
                        if (plm__clip_line_f(&px0, &py0, &px1, &py1,
                                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                                             (float)(p->plot_area.x1 - 1.5f),
                                             (float)(p->plot_area.y1 - 1.5f))) {
                            plm__wu_line_thick(fb, px0, py0, px1, py1,
                                               ls->style.color, ls->style.width);
                        }
                    } else if (ls->style.step_mode == PLM_STEP_PRE) {
                        /* horizontal then vertical */
                        float hx0 = prev_px, hy0 = prev_py;
                        float hx1 = px,      hy1 = prev_py;
                        if (plm__clip_line_f(&hx0, &hy0, &hx1, &hy1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, hx0, hy0, hx1, hy1,
                                               ls->style.color, ls->style.width);
                        float vx0 = px, vy0 = prev_py;
                        float vx1 = px, vy1 = py;
                        if (plm__clip_line_f(&vx0, &vy0, &vx1, &vy1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, vx0, vy0, vx1, vy1,
                                               ls->style.color, ls->style.width);
                    } else if (ls->style.step_mode == PLM_STEP_POST) {
                        /* vertical then horizontal */
                        float vx0 = prev_px, vy0 = prev_py;
                        float vx1 = prev_px, vy1 = py;
                        if (plm__clip_line_f(&vx0, &vy0, &vx1, &vy1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, vx0, vy0, vx1, vy1,
                                               ls->style.color, ls->style.width);
                        float hx0 = prev_px, hy0 = py;
                        float hx1 = px,      hy1 = py;
                        if (plm__clip_line_f(&hx0, &hy0, &hx1, &hy1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, hx0, hy0, hx1, hy1,
                                               ls->style.color, ls->style.width);
                    } else { /* PLM_STEP_MID */
                        float mid_x = (prev_px + px) * 0.5f;
                        float vx0 = prev_px, vy0 = prev_py;
                        float vx1 = mid_x,   vy1 = prev_py;
                        if (plm__clip_line_f(&vx0, &vy0, &vx1, &vy1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, vx0, vy0, vx1, vy1,
                                               ls->style.color, ls->style.width);
                        float hx0 = mid_x, hy0 = prev_py;
                        float hx1 = mid_x, hy1 = py;
                        if (plm__clip_line_f(&hx0, &hy0, &hx1, &hy1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, hx0, hy0, hx1, hy1,
                                               ls->style.color, ls->style.width);
                        float vx0b = mid_x, vy0b = py;
                        float vx1b = px,    vy1b = py;
                        if (plm__clip_line_f(&vx0b, &vy0b, &vx1b, &vy1b,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1 - 1.5f),
                             (float)(p->plot_area.y1 - 1.5f)))
                            plm__wu_line_thick(fb, vx0b, vy0b, vx1b, vy1b,
                                               ls->style.color, ls->style.width);
                    }
                }
                prev_px = px; prev_py = py;
                if (seg_start < 0) seg_start = i;
            }
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

    /* 9b2. draw stem series */
    {
        int si;
        for (si = 0; si < p->stem_count; si++) {
            const plm_stem_series *ss = &p->stems[si];
            if (ss->count <= 0) continue;
            plm_color lc = ss->style.line_color;
            float    lw = ss->style.line_width;
            float baseline_px = plm__map_y(p, ss->baseline);
            int i;
            for (i = 0; i < ss->count; i++) {
                if (plm__isnan(ss->x_data[i]) ||
                    plm__isnan(ss->y_data[i])) continue;
                float px = plm__map_x(p, ss->x_data[i]);
                float py = plm__map_y(p, ss->y_data[i]);
                /* vertical stem line */
                if (lw > 0.0f) {
                    float y0 = baseline_px, y1 = py;
                    if (y0 > y1) { float t = y0; y0 = y1; y1 = t; }
                    if (y0 < (float)p->plot_area.y0) y0 = (float)p->plot_area.y0;
                    if (y1 > (float)(p->plot_area.y1 - 1.5f)) y1 = (float)(p->plot_area.y1 - 1.5f);
                    if (y0 <= y1)
                        plm__wu_line_thick(fb, px, y0, px, y1, lc, lw);
                }
                /* marker at top */
                if (ss->style.marker_radius > 0.0f) {
                    int ix = (int)(px + 0.5f);
                    int iy = (int)(py + 0.5f);
                    if (ix >= p->plot_area.x0 && ix < p->plot_area.x1 &&
                        iy >= p->plot_area.y0 && iy < p->plot_area.y1)
                        plm__fill_circle(fb, ix, iy,
                            ss->style.marker_radius,
                            ss->style.marker_color);
                }
            }
        }
    }

    /* 9c. draw error bar series */
    {
        int si;
        for (si = 0; si < p->errorbar_count; si++) {
            const plm_errorbar_series *es = &p->errorbars[si];
            if (es->count <= 0) continue;
            plm_color lc = es->style.line_color;
            float    lw = es->style.line_width;
            float    cap = es->style.cap_size * 0.5f;
            /* connecting line */
            {
                int i, seg_start = -1;
                for (i = 0; i < es->count; i++) {
                    if (plm__isnan(es->x_data[i]) ||
                        plm__isnan(es->y_data[i]))
                        { seg_start = -1; continue; }
                    if (seg_start >= 0) {
                        float px0 = plm__map_x(p, es->x_data[i-1]);
                        float py0 = plm__map_y(p, es->y_data[i-1]);
                        float px1 = plm__map_x(p, es->x_data[i]);
                        float py1 = plm__map_y(p, es->y_data[i]);
                        if (plm__clip_line_f(&px0, &py0, &px1, &py1,
                             (float)p->plot_area.x0, (float)p->plot_area.y0,
                             (float)(p->plot_area.x1-1),
                             (float)(p->plot_area.y1-1)))
                            plm__wu_line_thick(fb, px0, py0, px1, py1, lc, lw);
                    }
                    if (seg_start < 0) seg_start = i;
                }
            }
            /* error bars per point */
            {
                int i;
                for (i = 0; i < es->count; i++) {
                    if (plm__isnan(es->x_data[i]) ||
                        plm__isnan(es->y_data[i])) continue;
                    float px = plm__map_x(p, es->x_data[i]);
                    float py = plm__map_y(p, es->y_data[i]);
                    /* y error bars (asymmetric, fall back to symmetric) */
                    {
                        float y_err_lo = 0.0f, y_err_hi = 0.0f;
                        int has_y = 0;
                        if (es->y_err_low && !plm__isnan(es->y_err_low[i]))
                            { y_err_lo = es->y_err_low[i]; has_y = 1; }
                        else if (es->y_err && !plm__isnan(es->y_err[i]))
                            { y_err_lo = es->y_err[i]; has_y = 1; }
                        if (es->y_err_high && !plm__isnan(es->y_err_high[i]))
                            { y_err_hi = es->y_err_high[i]; has_y = 1; }
                        else if (es->y_err && !plm__isnan(es->y_err[i]))
                            { y_err_hi = es->y_err[i]; has_y = 1; }
                        if (has_y && (y_err_lo > 0.0f || y_err_hi > 0.0f)) {
                            float ylo = plm__map_y(p,
                                (double)es->y_data[i] - (double)y_err_lo);
                            float yhi = plm__map_y(p,
                                (double)es->y_data[i] + (double)y_err_hi);
                            float ymin = (float)p->plot_area.y0;
                            float ymax = (float)(p->plot_area.y1 - 1.5f);
                            if (ylo < ymin) ylo = ymin;
                            if (ylo > ymax) ylo = ymax;
                            if (yhi < ymin) yhi = ymin;
                            if (yhi > ymax) yhi = ymax;
                            plm__wu_line_thick(fb, px, ylo, px, yhi, lc, lw);
                            if (cap > 0.0f) {
                                float x0 = px - cap, x1 = px + cap;
                                if (x0 < (float)p->plot_area.x0)
                                    x0 = (float)p->plot_area.x0;
                                if (x1 > (float)(p->plot_area.x1-1))
                                    x1 = (float)(p->plot_area.x1-1);
                                plm__wu_line_thick(fb, x0, ylo, x1, ylo, lc, lw);
                                plm__wu_line_thick(fb, x0, yhi, x1, yhi, lc, lw);
                            }
                        }
                    }
                    /* x error bars (asymmetric, fall back to symmetric) */
                    {
                        float x_err_lo = 0.0f, x_err_hi = 0.0f;
                        int has_x = 0;
                        if (es->x_err_low && !plm__isnan(es->x_err_low[i]))
                            { x_err_lo = es->x_err_low[i]; has_x = 1; }
                        else if (es->x_err && !plm__isnan(es->x_err[i]))
                            { x_err_lo = es->x_err[i]; has_x = 1; }
                        if (es->x_err_high && !plm__isnan(es->x_err_high[i]))
                            { x_err_hi = es->x_err_high[i]; has_x = 1; }
                        else if (es->x_err && !plm__isnan(es->x_err[i]))
                            { x_err_hi = es->x_err[i]; has_x = 1; }
                        if (has_x && (x_err_lo > 0.0f || x_err_hi > 0.0f)) {
                            float xlo = plm__map_x(p,
                                (double)es->x_data[i] - (double)x_err_lo);
                            float xhi = plm__map_x(p,
                                (double)es->x_data[i] + (double)x_err_hi);
                            float xmin = (float)p->plot_area.x0;
                            float xmax = (float)(p->plot_area.x1 - 1.5f);
                            if (xlo < xmin) xlo = xmin;
                            if (xlo > xmax) xlo = xmax;
                            if (xhi < xmin) xhi = xmin;
                            if (xhi > xmax) xhi = xmax;
                            plm__wu_line_thick(fb, xlo, py, xhi, py, lc, lw);
                            if (cap > 0.0f) {
                                float y0 = py - cap, y1 = py + cap;
                                if (y0 < (float)p->plot_area.y0)
                                    y0 = (float)p->plot_area.y0;
                                if (y1 > (float)(p->plot_area.y1-1))
                                    y1 = (float)(p->plot_area.y1-1);
                                plm__wu_line_thick(fb, xlo, y0, xlo, y1, lc, lw);
                                plm__wu_line_thick(fb, xhi, y0, xhi, y1, lc, lw);
                            }
                        }
                    }
                    /* marker */
                    if (es->style.marker_radius > 0.0f) {
                        int ix = (int)(px + 0.5f);
                        int iy = (int)(py + 0.5f);
                        if (ix >= p->plot_area.x0 && ix < p->plot_area.x1 &&
                            iy >= p->plot_area.y0 && iy < p->plot_area.y1)
                            plm__fill_circle(fb, ix, iy,
                                es->style.marker_radius,
                                es->style.marker_color);
                    }
                }
            }
        }
    }

    /* 10-12. draw titles, axis labels, and tick labels                */
    {
        const int S = PLOTMINI_TEXT_SCALE;
        int x_fs   = p->x_axis.font_scale > 0 ? p->x_axis.font_scale : S;
        int y_fs   = p->y_axis.font_scale > 0 ? p->y_axis.font_scale : S;
        int ry_fs  = p->y_axis_right.font_scale > 0 ? p->y_axis_right.font_scale : S;
        int x_th   = plm__text_height_s(x_fs);
        int y_th   = plm__text_height_s(y_fs);
        int ry_th  = plm__text_height_s(ry_fs);
        int char_w = PLM_FONT_H * S;
        int char_h = PLM_FONT_W * S;
        int advance= PLM_FONT_ADVANCE * S;
        int tick_len = 4 * S;
        int gap      = 3 * S;
        int tick_text_gap = 3 * S;

        /* ---- pre-scan: max width of y-axis tick labels (needed for positioning) ---- */
        int max_ytick_w = 0;
        {
            if (p->y_axis.scale == PLM_CATEGORICAL && p->y_axis.num_categories > 0) {
                int ci;
                for (ci = 0; ci < p->y_axis.num_categories; ci++) {
                    const char *label = p->y_axis.categories
                                        ? p->y_axis.categories[ci] : NULL;
                    char buf[32];
                    if (!label) { snprintf(buf, sizeof(buf), "%d", ci); label = buf; }
                    int w = plm__text_width_s(label, y_fs);
                    if (w > max_ytick_w) max_ytick_w = w;
                }
            } else if (p->y_axis.scale == PLM_LOG) {
                double log_min = log10(p->y_axis.min);
                double log_max = log10(p->y_axis.max);
                double v;
                for (v = floor(log_min); v <= log_max + 0.5; v += 1.0) {
                    double val = pow(10.0, v);
                    char buf[32];
                    if (p->y_axis.tick_formatter)
                        p->y_axis.tick_formatter(buf, sizeof(buf), val);
                    else snprintf(buf, sizeof(buf), "%.4g", val);
                    int w = plm__text_width_s(buf, y_fs);
                    if (w > max_ytick_w) max_ytick_w = w;
                }
            } else {
                double range = p->y_axis.max - p->y_axis.min;
                if (range > 0.0) {
                    double step = plm__nice_step(range, p->y_axis.tick_hint > 0 ?
                                                 p->y_axis.tick_hint : 5);
                    double lo = floor(p->y_axis.min / step) * step;
                    double v;
                    for (v = lo; v <= p->y_axis.max + step * 0.5; v += step) {
                        char buf[32];
                        if (p->y_axis.tick_formatter)
                            p->y_axis.tick_formatter(buf, sizeof(buf), v);
                        else snprintf(buf, sizeof(buf), "%.4g", v);
                        int w = plm__text_width_s(buf, y_fs);
                        if (w > max_ytick_w) max_ytick_w = w;
                    }
                }
            }
        }

        int max_rytick_w = 0;
        int has_right = (p->y_axis_right.min != p->y_axis_right.max ||
                         p->y_axis_right.label != NULL ||
                         p->y_label_right != NULL);
        if (has_right) {
            double range = p->y_axis_right.max - p->y_axis_right.min;
            if (range <= 0.0) range = 1.0;
            double step = plm__nice_step(range, p->y_axis_right.tick_hint > 0 ?
                                         p->y_axis_right.tick_hint : 5);
            double lo = floor(p->y_axis_right.min / step) * step;
            double v;
            for (v = lo; v <= p->y_axis_right.max + step * 0.5; v += step) {
                char buf[32];
                if (p->y_axis_right.tick_formatter)
                    p->y_axis_right.tick_formatter(buf, sizeof(buf), v);
                else snprintf(buf, sizeof(buf), "%.4g", v);
                int w = plm__text_width_s(buf, ry_fs);
                if (w > max_rytick_w) max_rytick_w = w;
            }
        }

        /* ==== 10. title ==== */
        if (p->title) {
            int tw = plm_text_width(p->title);
            int cx = cell_x0 + (cell_x1 - cell_x0) / 2;
            int th = plm_text_height();
            int avail = p->margin_top > 0 ? p->margin_top : 30;
            int ty = cell_y0 + avail / 2 + th / 2;
            plm_draw_text(fb, cx - tw / 2, ty, p->title, PLM_FG_COLOR);
        }

        /* ---- compute positions ---- */
        /* X-axis: spine at plot_area.y1-1, tick marks go down, then text below */
        int x_spine_y   = p->plot_area.y1 - 1;
        int x_tick_end  = x_spine_y + 1 + tick_len;  /* bottom of tick mark */
        int x_text_y0   = x_tick_end + tick_text_gap + x_th; /* bottom of x tick text */
        int x_label_y   = x_text_y0 + x_th + gap;

        /* Left Y-axis: tick marks go left from spine, then text left of that */
        int y_spine_x   = p->plot_area.x0;
        int y_tick_left = y_spine_x - tick_len;       /* left end of tick mark */
        int y_text_right= y_tick_left - tick_text_gap; /* right edge of y tick text */
        int ylabel_x;
        {
            int y_text_left = y_text_right - max_ytick_w;
            int space = y_text_left - gap - char_w - cell_x0;
            if (space < 0) space = 0;
            ylabel_x = cell_x0 + space / 2 + char_w / 2;
            if (ylabel_x + char_w / 2 > y_text_left - gap)
                ylabel_x = y_text_left - gap - char_w / 2;
            if (ylabel_x - char_w / 2 < cell_x0)
                ylabel_x = cell_x0 + char_w / 2;
        }

        /* Right Y-axis: tick marks go right from spine, then text right of that */
        int ry_spine_x  = p->plot_area.x1 - 1;
        int ry_tick_right= ry_spine_x + 1 + tick_len;
        int ry_text_x   = ry_tick_right + tick_text_gap;
        int rylabel_x;
        if (has_right) {
            int ry_text_right = ry_text_x + max_rytick_w;
            int space = cell_x1 - (ry_text_right + gap + char_w);
            if (space < 0) space = 0;
            rylabel_x = ry_text_right + gap + space / 2;
            if (rylabel_x + char_w / 2 > cell_x1)
                rylabel_x = cell_x1 - char_w / 2;
        } else {
            rylabel_x = cell_x1;
        }

        /* ==== tick marks on axis spines ==== */
        {
            plm_color tc = PLM_FG_COLOR;
            /* X-axis tick marks (on bottom spine, pointing down) */
            if (p->x_axis.scale == PLM_CATEGORICAL && p->x_axis.num_categories > 0) {
                int ci;
                for (ci = 0; ci < p->x_axis.num_categories; ci++) {
                    float px = plm__map_x(p, (double)ci);
                    int ix = (int)(px + 0.5f);
                    if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                        int y;
                        for (y = -2*S; y < tick_len; y++)
                            plm__set_pixel(fb, ix, x_spine_y + 1 + y, tc);
                    }
                }
            } else {
                double range = p->x_axis.max - p->x_axis.min;
                double step  = plm__nice_step(range, p->x_axis.tick_hint > 0 ?
                                             p->x_axis.tick_hint : 5);
                double lo;
                if (p->x_axis.scale == PLM_LOG) {
                    double log_min = log10(p->x_axis.min);
                    double log_max = log10(p->x_axis.max);
                    lo = floor(log_min); step = 1.0;
                    double v;
                    for (v = lo; v <= log_max + 0.5; v += step) {
                        double val = pow(10.0, v);
                        float px = plm__map_x(p, val);
                        int ix = (int)(px + 0.5f);
                        if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                            int y;
                            for (y = 0; y < tick_len; y++)
                                plm__set_pixel(fb, ix, x_spine_y + 1 + y, tc);
                        }
                    }
                } else {
                    lo = floor(p->x_axis.min / step) * step;
                    double v;
                    for (v = lo; v <= p->x_axis.max + step * 0.5; v += step) {
                        float px = plm__map_x(p, v);
                        int ix = (int)(px + 0.5f);
                        if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                            int y;
                            for (y = 0; y < tick_len; y++)
                                plm__set_pixel(fb, ix, x_spine_y + 1 + y, tc);
                        }
                    }
                }
            }
            /* Left Y-axis tick marks (pointing left) */
            if (p->y_axis.scale == PLM_CATEGORICAL && p->y_axis.num_categories > 0) {
                int ci;
                for (ci = 0; ci < p->y_axis.num_categories; ci++) {
                    float py = plm__map_y(p, (double)ci);
                    int iy = (int)(py + 0.5f);
                    if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                        int x;
                        for (x = -2*S; x < tick_len; x++)
                            plm__set_pixel(fb, y_spine_x - 1 - x, iy, tc);
                    }
                }
            } else {
                double range = p->y_axis.max - p->y_axis.min;
                double step  = plm__nice_step(range, p->y_axis.tick_hint > 0 ?
                                             p->y_axis.tick_hint : 5);
                double lo;
                if (p->y_axis.scale == PLM_LOG) {
                    double log_min = log10(p->y_axis.min);
                    double log_max = log10(p->y_axis.max);
                    lo = floor(log_min); step = 1.0;
                    double v;
                    for (v = lo; v <= log_max + 0.5; v += step) {
                        double val = pow(10.0, v);
                        float py = plm__map_y(p, val);
                        int iy = (int)(py + 0.5f);
                        if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                            int x;
                            for (x = 0; x < tick_len; x++)
                                plm__set_pixel(fb, y_spine_x - 1 - x, iy, tc);
                        }
                    }
                } else {
                    lo = floor(p->y_axis.min / step) * step;
                    double v;
                    for (v = lo; v <= p->y_axis.max + step * 0.5; v += step) {
                        float py = plm__map_y(p, v);
                        int iy = (int)(py + 0.5f);
                        if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                            int x;
                            for (x = 0; x < tick_len; x++)
                                plm__set_pixel(fb, y_spine_x - 1 - x, iy, tc);
                        }
                    }
                }
            }
            /* Right Y-axis tick marks (pointing right) */
            if (has_right) {
                double range = p->y_axis_right.max - p->y_axis_right.min;
                if (range <= 0.0) range = 1.0;
                double step  = plm__nice_step(range, p->y_axis_right.tick_hint > 0 ?
                                             p->y_axis_right.tick_hint : 5);
                double lo = floor(p->y_axis_right.min / step) * step;
                double v;
                for (v = lo; v <= p->y_axis_right.max + step * 0.5; v += step) {
                    double t = (v - p->y_axis_right.min) /
                               (p->y_axis_right.max - p->y_axis_right.min);
                    float py = (float)(p->plot_area.y1 - 1 -
                                       t * (p->plot_area.y1 - p->plot_area.y0));
                    int iy = (int)(py + 0.5f);
                    if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                        int x;
                        for (x = -2*S; x < tick_len; x++)
                            plm__set_pixel(fb, ry_spine_x + 1 + x, iy, tc);
                    }
                }
            }
        }

        /* ==== 11. axis labels ==== */
        if (p->x_label) {
            int tw = plm_text_width(p->x_label);
            int cx = p->plot_area.x0 + (p->plot_area.x1 - p->plot_area.x0) / 2;
            plm_draw_text(fb, cx - tw / 2, x_label_y, p->x_label, PLM_FG_COLOR);
        }
        if (p->y_label) {
            int len = (int)strlen(p->y_label);
            int total_h = len * advance;
            int mid_y = p->plot_area.y0 + (p->plot_area.y1 - p->plot_area.y0) / 2;
            int start_y = mid_y + total_h / 2;
            int i;
            for (i = 0; p->y_label[i]; i++) {
                int gx = ylabel_x - char_w / 2;
                int gy = start_y - (len - 1 - i) * advance - char_h / 2;
                plm__draw_char_rot90(fb, gx, gy,
                    (unsigned char)p->y_label[i], PLM_FG_COLOR, S);
            }
        }
        if (p->y_label_right && has_right) {
            int len = (int)strlen(p->y_label_right);
            int total_h = len * advance;
            int mid_y = p->plot_area.y0 + (p->plot_area.y1 - p->plot_area.y0) / 2;
            int start_y = mid_y + total_h / 2;
            int i;
            for (i = 0; p->y_label_right[i]; i++) {
                int gx = rylabel_x - char_w / 2;
                int gy = start_y - (len - 1 - i) * advance - char_h / 2;
                plm__draw_char_rot90(fb, gx, gy,
                    (unsigned char)p->y_label_right[i], PLM_FG_COLOR, S);
            }
        }

        /* ==== 12. tick labels ==== */
        /* X-axis tick labels (centred on tick, below tick mark) */
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
                        int tw = plm__text_width_s(buf, x_fs);
                        plm__draw_text_s(fb, ix - tw / 2, x_text_y0, buf, PLM_FG_COLOR, x_fs);
                    } else {
                        int tw = plm__text_width_s(label, x_fs);
                        plm__draw_text_s(fb, ix - tw / 2, x_text_y0, label, PLM_FG_COLOR, x_fs);
                    }
                }
            }
        } else {
            double range = p->x_axis.max - p->x_axis.min;
            double step  = plm__nice_step(range, p->x_axis.tick_hint > 0 ?
                                         p->x_axis.tick_hint : 5);
            double lo;
            if (p->x_axis.scale == PLM_LOG) {
                double log_min = log10(p->x_axis.min);
                double log_max = log10(p->x_axis.max);
                lo = floor(log_min); step = 1.0;
                double v;
                for (v = lo; v <= log_max + 0.5; v += step) {
                    double val = pow(10.0, v);
                    float px = plm__map_x(p, val);
                    int ix = (int)(px + 0.5f);
                    if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                        char buf[32];
                        if (p->x_axis.tick_formatter)
                            p->x_axis.tick_formatter(buf, sizeof(buf), val);
                        else
                            snprintf(buf, sizeof(buf), "%.4g", val);
                        int tw = plm__text_width_s(buf, x_fs);
                        plm__draw_text_s(fb, ix - tw / 2, x_text_y0, buf, PLM_FG_COLOR, x_fs);
                    }
                }
            } else {
                lo = floor(p->x_axis.min / step) * step;
                double v;
                for (v = lo; v <= p->x_axis.max + step * 0.5; v += step) {
                    float px = plm__map_x(p, v);
                    int ix = (int)(px + 0.5f);
                    if (ix >= p->plot_area.x0 && ix < p->plot_area.x1) {
                        char buf[32];
                        if (p->x_axis.tick_formatter)
                            p->x_axis.tick_formatter(buf, sizeof(buf), v);
                        else
                            snprintf(buf, sizeof(buf), "%.4g", v);
                        int tw = plm__text_width_s(buf, x_fs);
                        plm__draw_text_s(fb, ix - tw / 2, x_text_y0, buf, PLM_FG_COLOR, x_fs);
                    }
                }
            }
        }
        /* Y-axis tick labels (left side, right-aligned, vertically centred on tick) */
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
                        int tw = plm__text_width_s(buf, y_fs);
                        plm__draw_text_s(fb, y_text_right - tw, iy + y_th/2, buf, PLM_FG_COLOR, y_fs);
                    } else {
                        int tw = plm__text_width_s(label, y_fs);
                        plm__draw_text_s(fb, y_text_right - tw, iy + y_th/2, label, PLM_FG_COLOR, y_fs);
                    }
                }
            }
        } else {
            double range = p->y_axis.max - p->y_axis.min;
            double step  = plm__nice_step(range, p->y_axis.tick_hint > 0 ?
                                         p->y_axis.tick_hint : 5);
            double lo;
            if (p->y_axis.scale == PLM_LOG) {
                double log_min = log10(p->y_axis.min);
                double log_max = log10(p->y_axis.max);
                lo = floor(log_min); step = 1.0;
                double v;
                for (v = lo; v <= log_max + 0.5; v += step) {
                    double val = pow(10.0, v);
                    float py = plm__map_y(p, val);
                    int iy = (int)(py + 0.5f);
                    if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                        char buf[32];
                        if (p->y_axis.tick_formatter)
                            p->y_axis.tick_formatter(buf, sizeof(buf), val);
                        else
                            snprintf(buf, sizeof(buf), "%.4g", val);
                        int tw = plm__text_width_s(buf, y_fs);
                        plm__draw_text_s(fb, y_text_right - tw, iy + y_th/2, buf, PLM_FG_COLOR, y_fs);
                    }
                }
            } else {
                lo = floor(p->y_axis.min / step) * step;
                double v;
                for (v = lo; v <= p->y_axis.max + step * 0.5; v += step) {
                    float py = plm__map_y(p, v);
                    int iy = (int)(py + 0.5f);
                    if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                        char buf[32];
                        if (p->y_axis.tick_formatter)
                            p->y_axis.tick_formatter(buf, sizeof(buf), v);
                        else
                            snprintf(buf, sizeof(buf), "%.4g", v);
                        int tw = plm__text_width_s(buf, y_fs);
                        plm__draw_text_s(fb, y_text_right - tw, iy + y_th/2, buf, PLM_FG_COLOR, y_fs);
                    }
                }
            }
        }
        /* Right y-axis tick labels */
        if (has_right) {
            double range = p->y_axis_right.max - p->y_axis_right.min;
            if (range <= 0.0) range = 1.0;
            double step  = plm__nice_step(range, p->y_axis_right.tick_hint > 0 ?
                                         p->y_axis_right.tick_hint : 5);
            double lo = floor(p->y_axis_right.min / step) * step;
            double v;
            for (v = lo; v <= p->y_axis_right.max + step * 0.5; v += step) {
                double t = (v - p->y_axis_right.min) /
                           (p->y_axis_right.max - p->y_axis_right.min);
                float py = (float)(p->plot_area.y1 - 1 -
                                   t * (p->plot_area.y1 - p->plot_area.y0));
                int iy = (int)(py + 0.5f);
                if (iy >= p->plot_area.y0 && iy < p->plot_area.y1) {
                    char buf[32];
                    if (p->y_axis_right.tick_formatter)
                        p->y_axis_right.tick_formatter(buf, sizeof(buf), v);
                    else
                        snprintf(buf, sizeof(buf), "%.4g", v);
                    plm__draw_text_s(fb, ry_text_x, iy + ry_th/2, buf, PLM_FG_COLOR, ry_fs);
                }
            }
        }
    }  /* end of label block */

    /* 13. draw legend (if enabled) */
    if (p->legend_position != PLM_LEGEND_NONE) {
        struct { const char *label; plm_color color; int is_line; int is_marker; } entries[64];
        int entry_count = 0;
        int si;
        for (si = 0; si < p->line_count && entry_count < 64; si++) {
            if (p->lines[si].style.legend) {
                entries[entry_count].label = p->lines[si].style.legend;
                entries[entry_count].color = p->lines[si].style.color;
                entries[entry_count].is_line = 1;
                entries[entry_count].is_marker = 0;
                entry_count++;
            }
        }
        for (si = 0; si < p->scatter_count && entry_count < 64; si++) {
            if (p->scatters[si].style.legend) {
                entries[entry_count].label = p->scatters[si].style.legend;
                entries[entry_count].color = p->scatters[si].style.color;
                entries[entry_count].is_line = 0;
                entries[entry_count].is_marker = 1;
                entry_count++;
            }
        }
        for (si = 0; si < p->errorbar_count && entry_count < 64; si++) {
            if (p->errorbars[si].style.legend) {
                entries[entry_count].label = p->errorbars[si].style.legend;
                entries[entry_count].color = p->errorbars[si].style.line_color;
                entries[entry_count].is_line = 1;
                entries[entry_count].is_marker = (p->errorbars[si].style.marker_radius > 0.0f);
                entry_count++;
            }
        }
        for (si = 0; si < p->band_count && entry_count < 64; si++) {
            if (p->bands[si].style.legend) {
                entries[entry_count].label = p->bands[si].style.legend;
                entries[entry_count].color = p->bands[si].style.fill_color;
                entries[entry_count].is_line = 0;
                entries[entry_count].is_marker = 0;
                entry_count++;
            }
        }
        for (si = 0; si < p->bar_count && entry_count < 64; si++) {
            if (p->bars[si].style.legend) {
                entries[entry_count].label = p->bars[si].style.legend;
                entries[entry_count].color = p->bars[si].style.fill_color;
                entries[entry_count].is_line = 0;
                entries[entry_count].is_marker = 0;
                entry_count++;
            }
        }
        for (si = 0; si < p->stem_count && entry_count < 64; si++) {
            if (p->stems[si].style.legend) {
                entries[entry_count].label = p->stems[si].style.legend;
                entries[entry_count].color = p->stems[si].style.line_color;
                entries[entry_count].is_line = 0;
                entries[entry_count].is_marker = (p->stems[si].style.marker_radius > 0.0f);
                entry_count++;
            }
        }
        if (entry_count > 0) {
            const int S = PLOTMINI_TEXT_SCALE;
            int max_label_w = 0, i;
            for (i = 0; i < entry_count; i++) {
                int w = plm_text_width(entries[i].label);
                if (w > max_label_w) max_label_w = w;
            }
            int sample_w = 14 * S;
            int entry_h = (PLM_FONT_H + 2) * S;
            int pad = 6 * S;
            int box_w = pad + sample_w + 4 * S + max_label_w + pad;
            int box_h = pad + entry_count * entry_h + pad;
            int box_x, box_y;
            int pa_x0 = p->plot_area.x0, pa_y0 = p->plot_area.y0;
            int pa_x1 = p->plot_area.x1, pa_y1 = p->plot_area.y1;
            switch (p->legend_position) {
                case PLM_LEGEND_TOP_RIGHT:
                    box_x = pa_x1 - box_w - 4; box_y = pa_y0 + 4; break;
                case PLM_LEGEND_TOP_LEFT:
                    box_x = pa_x0 + 4; box_y = pa_y0 + 4; break;
                case PLM_LEGEND_BOTTOM_RIGHT:
                    box_x = pa_x1 - box_w - 4; box_y = pa_y1 - box_h - 4; break;
                case PLM_LEGEND_BOTTOM_LEFT:
                    box_x = pa_x0 + 4; box_y = pa_y1 - box_h - 4; break;
                case PLM_LEGEND_OUTSIDE_RIGHT:
                    box_x = pa_x1 + 4; box_y = pa_y0 + 4; break;
                default: box_x = pa_x1 - box_w - 4; box_y = pa_y0 + 4; break;
            }
            { plm_irect bg_r = { box_x, box_y, box_x + box_w, box_y + box_h };
              plm_fb_fill_rect(fb, bg_r, PLM_BG_COLOR);
              { int x; for (x = bg_r.x0; x < bg_r.x1; x++) {
                  plm__set_pixel(fb, x, bg_r.y0, PLM_FG_COLOR);
                  plm__set_pixel(fb, x, bg_r.y1 - 1, PLM_FG_COLOR); }
                int y; for (y = bg_r.y0; y < bg_r.y1; y++) {
                  plm__set_pixel(fb, bg_r.x0, y, PLM_FG_COLOR);
                  plm__set_pixel(fb, bg_r.x1 - 1, y, PLM_FG_COLOR); } } }
            for (i = 0; i < entry_count; i++) {
                int ey = box_y + pad + i * entry_h;
                int sx = box_x + pad;
                int sy = ey + entry_h / 2;
                if (entries[i].is_line) {
                    plm__wu_line_thick(fb, (float)sx, (float)sy,
                        (float)(sx + sample_w), (float)sy, entries[i].color, 2.0f);
                } else if (entries[i].is_marker) {
                    plm__fill_circle(fb, sx + sample_w / 2, sy, 3.0f, entries[i].color);
                } else {
                    plm_irect r = { sx, sy - 3*S, sx + sample_w, sy + 3*S };
                    plm_fb_fill_rect(fb, r, entries[i].color);
                }
                plm_draw_text(fb, sx + sample_w + 4 * S,
                    ey + PLM_FONT_H * S, entries[i].label, PLM_FG_COLOR);
            }
        }
    }

}

/* Public: render a single plot (fills the whole framebuffer). */
void plm_render(const plm_plot *p, plm_fb *fb) {
    plm_plot *pp = (plm_plot *)p;  /* cast away const for autorange writes */

    /* clear the whole framebuffer once */
    plm_fb_clear(fb, PLM_BG_COLOR);

    /* delegate to the cell-based renderer */
    plm__render_plot_into(pp, fb, 0, 0, fb->width, fb->height);
}

/* ---- built-in tick formatters -------------------------------------- */

/* Default: "%.4g" */
void   plm_tick_fmt_default(char *buf, int sz, double v);
/* Scientific: "%.2e" */
void   plm_tick_fmt_sci(char *buf, int sz, double v);
/* Integer: "%.0f" */
void   plm_tick_fmt_int(char *buf, int sz, double v);
/* Percentage: "%.0f%%" (multiplies v by 100) */
void   plm_tick_fmt_pct(char *buf, int sz, double v);
/* Log10: "10^n" for exact powers of 10, else "%.4g" */
void   plm_tick_fmt_log10(char *buf, int sz, double v);
/* Currency: "$%.2f" */
void   plm_tick_fmt_usd(char *buf, int sz, double v);

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
        p->margin_left   = 55;
        p->margin_top    = 30;
        p->margin_right  = 20;
        p->margin_bottom = 38;
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
    plm_fb_clear(fb, PLM_BG_COLOR);

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
