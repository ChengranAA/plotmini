# plotmini

Single-header ANSI C library for software-rendered plots. Zero dependencies beyond libc.

Output is a raw pixel framebuffer — attach any display backend (minifb, SDL, a file, …).

## Quick Start

```c
#define PLOTMINI_IMPLEMENTATION
#include "plotmini.h"

// 1. create a framebuffer
unsigned char pixels[800 * 600 * 4];
plm_fb fb = plm_fb_create(pixels, 800, 600, PLM_RGBA8);

// 2. build a plot
plm_plot plot;
plm_plot_init(&plot);
plot.title   = "Hello plotmini";
plot.x_label = "x";
plot.y_label = "sin(x)";

float x[100], y[100];
for (int i = 0; i < 100; i++) {
    x[i] = i * 0.1f;
    y[i] = sinf(x[i]);
}
plm_line_style s = { PLM_BLUE, 1.0f, "sin(x)" };
plm_plot_add_line(&plot, x, y, 100, s);

// 3. render
plm_render(&plot, &fb);
```

## Subplots

```c
plm_figure fig;
plm_figure_init(&fig, 2, 2);   // 2×2 grid

plm_plot *p0 = plm_figure_plot(&fig, 0, 0);
p0->title = "sin(x)";
plm_plot_add_line(p0, x, y, n, (plm_line_style){PLM_BLUE, 1.0f, NULL});

plm_plot *p1 = plm_figure_plot(&fig, 0, 1);
p1->title = "cos(x)";
plm_plot_add_line(p1, x2, y2, n2, (plm_line_style){PLM_RED, 1.0f, NULL});

plm_figure_render(&fig, &fb);
plm_figure_reset(&fig);
```

## Features

- Line plots with Wu anti-aliased rendering, variable thickness, NaN gaps
- Histograms (binned bars, filled + stroked)
- `plm_imshow` — colormapped 2D arrays (heat, viridis, gray)
- Automatic axis range with nice-number ticks
- Configurable margins, grid lines, labels, title
- Subplot grid layout with independent axes per cell
- Pixel formats: RGBA8 (4 bpp) or GRAY8 (1 bpp)

## Building the Examples

```sh
cd examples
make          # builds all examples (macOS, needs minifb)
./01_basic    # run
```

## Configuration Macros

Define before `#include "plotmini.h"`:

| Macro | Effect |
|---|---|
| `PLOTMINI_MALLOC` / `PLOTMINI_FREE` | Custom allocator |
| `PLOTMINI_NO_FONT` | Strip embedded 5×7 font (~1.5 KB) |
| `PLOTMINI_GRAYSCALE_ONLY` | Drop RGBA8, halve code size |

## Authorship

This library was written by agentic AI under human supervision and direction. All code is generated; the human reviewer guided the design, reviewed changes, and bears responsibility for the result.

## License

Public domain (Unlicense). Do what you want.
