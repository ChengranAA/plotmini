#define PLOTMINI_TEXT_SCALE 2
#define PLOTMINI_DARK_THEME
#define PLOTMINI_IMPLEMENTATION
#include "../../plotmini.h"
#include "dep/MiniFB.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- custom tick formatters ---- */

/* ---- helper: normal-distribution PDF ---- */
static float normal_pdf(float x, float mu, float sigma) {
    float z = (x - mu) / sigma;
    return expf(-0.5f * z * z) / (sigma * sqrtf(2.0f * (float)M_PI));
}

int main(void) {
    const int win_w = 1200;
    const int win_h = 860;

    struct mfb_window *win = mfb_open(
        "plotmini — full-feature showcase", win_w, win_h);
    if (!win) { fprintf(stderr, "failed to open window\n"); return 1; }

    unsigned char *pixels = (unsigned char *)malloc((size_t)win_w * win_h * 4);
    plm_fb fb = plm_fb_create(pixels, win_w, win_h, PLM_RGBA8);

    plm_figure fig;
    plm_figure_init(&fig, 3, 2);
    fig.hgap = 24;
    fig.vgap = 24;

    /* ================================================================ */
    /* (0,0) — SIGNAL ANALYSIS : confidence band + step overlay         */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 0);
        p->title   = "Signal with 95% confidence band";
        p->x_label = "time (ms)";
        p->y_label = "amplitude";
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        const int n = 200;
        float *tx   = (float *)malloc((size_t)n * sizeof(float));
        float *sig  = (float *)malloc((size_t)n * sizeof(float));
        float *lo   = (float *)malloc((size_t)n * sizeof(float));
        float *hi   = (float *)malloc((size_t)n * sizeof(float));
        float *step = (float *)malloc((size_t)n * sizeof(float));
        int i;
        for (i = 0; i < n; i++) {
            tx[i]    = (float)i * 0.05f;
            float s  = sinf(tx[i] * 3.0f) * expf(-tx[i] * 0.15f);
            float nz = 0.12f * sinf(tx[i] * 17.0f);
            sig[i]   = s + nz;
            float env = 0.22f + fabsf(s) * 0.35f;
            lo[i]    = sig[i] - env;
            hi[i]    = sig[i] + env;
            /* step-line version (use only every 8th point for steps) */
            step[i]  = sig[i];
        }

        /* confidence band */
        plm_band_style bs = {
            PLM_RGBA(100, 140, 220, 80),
            PLM_RGBA(100, 140, 220, 180), 0.6f,
            "95% confidence"
        };
        plm_plot_add_band(p, tx, lo, hi, n, bs);

        /* step-plot overlay (decimated) */
        {
            const int sn = 25;
            float sx[25], sy[25];
            for (i = 0; i < sn; i++) {
                int idx = i * n / sn;
                sx[i] = tx[idx];
                sy[i] = sig[idx];
            }
            plm_line_style ls = { PLM_YELLOW, 2.5f, "step-post envelope", PLM_STEP_POST , 0};
            plm_plot_add_line(p, sx, sy, sn, ls);
        }

        /* fine signal line */
        {
            plm_line_style ls = { PLM_WHITE, 1.0f, "raw signal", 0 , 0};
            plm_plot_add_line(p, tx, sig, n, ls);
        }

        free(tx); free(sig); free(lo); free(hi); free(step);
    }

    /* ================================================================ */
    /* (0,1) — EXPERIMENTAL DATA : asymmetric error bars + fit          */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 0, 1);
        p->title   = "Asymmetric measurement errors";
        p->x_label = "concentration (mM)";
        p->y_label = "response (a.u.)";
        p->legend_position = PLM_LEGEND_TOP_LEFT;

        const int n = 9;
        float x[n], y[n], ylo[n], yhi[n];
        int i;
        for (i = 0; i < n; i++) {
            x[i]    = 0.5f + (float)i * 0.8f;
            float ideal = 1.2f + 0.55f * (float)i;
            y[i]    = ideal + 0.25f * sinf((float)i * 1.2f);
            ylo[i]  = 0.10f + 0.04f * (float)i;   /* smaller downward */
            yhi[i]  = 0.35f + 0.08f * (float)i;   /* larger upward   */
        }

        /* asymmetric error bars */
        plm_errorbar_style es = {
            PLM_CYAN, 1.5f, 4.0f, PLM_WHITE, 3.5f,
            "measured"
        };
        plm_plot_add_errorbar_asym(p, x, y,
            NULL, NULL, ylo, yhi, n, es);

        /* scatter of raw points */
        plm_scatter_style ss = { PLM_RGBA(255,200,100,200), 2.5f, "raw replicates" };
        /* add jittered replicates around each measurement */
        {
            float sx[27], sy[27];
            int ri = 0;
            for (i = 0; i < n; i++) {
                int rep;
                for (rep = 0; rep < 3; rep++) {
                    sx[ri] = x[i] + 0.08f * ((float)rand()/(float)RAND_MAX - 0.5f);
                    sy[ri] = y[i] + 0.20f * ((float)rand()/(float)RAND_MAX - 0.5f);
                    ri++;
                }
            }
            plm_plot_add_scatter(p, sx, sy, ri, ss);
        }

        /* linear fit line */
        {
            float fx[] = { x[0] - 0.3f, x[n-1] + 0.3f };
            float fy[] = {
                1.2f + 0.55f * 0.0f - 0.3f * 0.55f,
                1.2f + 0.55f * (float)(n-1) + 0.3f * 0.55f
            };
            plm_line_style ls = { PLM_RED, 1.8f, "linear fit", 0 , 0};
            plm_plot_add_line(p, fx, fy, 2, ls);
        }
    }

    /* ================================================================ */
    /* (1,0) — DISTRIBUTION : histogram + density curve                 */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 0);
        p->title   = "Sample distribution (n=2000)";
        p->x_label = "value";
        p->y_label = "count / density";
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        /* generate bimodal samples */
        const int nsamp = 2000;
        float *samples = (float *)malloc((size_t)nsamp * sizeof(float));
        int i;
        for (i = 0; i < nsamp; i++) {
            /* mixture: 60% N(3, 0.7) + 40% N(6.5, 1.1) */
            float u = (float)rand() / (float)RAND_MAX;
            float r1 = sqrtf(-2.0f * logf(fmaxf(0.0001f, (float)rand() / (float)RAND_MAX)))
                       * cosf(2.0f * (float)M_PI * (float)rand() / (float)RAND_MAX);
            if (u < 0.6f)
                samples[i] = 3.0f + 0.7f * r1;
            else
                samples[i] = 6.5f + 1.1f * r1;
        }

        /* histogram */
        plm_bar_style hs = {
            PLM_RGBA(70, 130, 210, 180),
            PLM_RGBA(70, 130, 210, 255), 0.5f,
            "histogram"
        };
        plm_plot_add_hist(p, samples, nsamp, 35, 0, hs);

        /* density curve overlay */
        {
            const int dn = 150;
            float dx[150], dy[150];
            for (i = 0; i < dn; i++) {
                dx[i] = 0.0f + (float)i * 9.0f / (float)(dn - 1);
                dy[i] = 0.6f * normal_pdf(dx[i], 3.0f, 0.7f)
                      + 0.4f * normal_pdf(dx[i], 6.5f, 1.1f);
                dy[i] *= (float)nsamp * (9.0f / 35.0f); /* scale to match histogram */
            }
            plm_line_style ls = { PLM_YELLOW, 2.0f, "true density", 0 , 0};
            plm_plot_add_line(p, dx, dy, dn, ls);
        }

        free(samples);
    }

    /* ================================================================ */
    /* (1,1) — CATEGORICAL : grouped bar chart with error bars          */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 1, 1);
        p->title   = "Survey: feature ratings by group";
        p->x_label = "Feature";
        p->y_label = "Score (0–10)";

        p->x_axis.scale = PLM_CATEGORICAL;
        p->x_axis.num_categories = 5;
        static const char *cats[] = {"Speed", "UI/UX", "Reliability", "Docs", "Support"};
        p->x_axis.categories = cats;
        p->x_axis.grid = 0;

        /* two groups as clustered bars */
        float xA[] = { -0.2f, 0.8f, 1.8f, 2.8f, 3.8f };
        float yA[] = { 7.2f, 6.5f, 8.1f, 4.3f, 5.8f };
        float xB[] = {  0.2f, 1.2f, 2.2f, 3.2f, 4.2f };
        float yB[] = { 6.8f, 8.2f, 7.0f, 5.9f, 6.4f };

        plm_bar_style bsA = { PLM_RGBA(60, 140, 220, 220), PLM_BLACK, 0.8f, "Group A" };
        plm_bar_style bsB = { PLM_RGBA(240, 140, 50, 220),  PLM_BLACK, 0.8f, "Group B" };

        plm_plot_add_bar(p, xA, yA, 5, 0.35f, bsA);
        plm_plot_add_bar(p, xB, yB, 5, 0.35f, bsB);

        /* error bars on bars */
        float errA[] = { 0.5f, 0.7f, 0.4f, 1.0f, 0.8f };
        float errB[] = { 0.6f, 0.5f, 0.7f, 0.9f, 0.6f };
        plm_errorbar_style esA = { PLM_WHITE, 1.0f, 3.0f, {0}, 0.0f, NULL };
        plm_errorbar_style esB = { PLM_WHITE, 1.0f, 3.0f, {0}, 0.0f, NULL };
        plm_plot_add_errorbar(p, xA, yA, NULL, errA, 5, esA);
        plm_plot_add_errorbar(p, xB, yB, NULL, errB, 5, esB);

        p->legend_position = PLM_LEGEND_TOP_LEFT;
    }

    /* ================================================================ */
    /* (2,0) — SPECTRUM : stem plot on log-log with formatter           */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 2, 0);
        p->title   = "Frequency spectrum (log-log)";
        p->x_label = "frequency (Hz)";
        p->y_label = "magnitude";
        p->x_axis.scale = PLM_LOG;
        p->y_axis.scale = PLM_LOG;
        p->x_axis.tick_formatter = plm_tick_fmt_log10;
        p->y_axis.tick_formatter = plm_tick_fmt_log10;
        p->legend_position = PLM_LEGEND_TOP_RIGHT;

        const int n = 50;
        float fx[n], fm[n];
        int i;
        for (i = 0; i < n; i++) {
            fx[i] = 0.8f * powf(10.0f, 0.04f * (float)i);  /* 0.8 → ~70 Hz */
            /* synthetic spectrum: 1/f noise + resonant peaks */
            fm[i] = 8.0f / sqrtf(fx[i])
                  + 5.0f * expf(-powf((fx[i] - 10.0f) / 3.0f, 2.0f))
                  + 3.5f * expf(-powf((fx[i] - 35.0f) / 4.0f, 2.0f));
            fm[i] *= 0.5f + 0.5f * ((float)rand() / (float)RAND_MAX);
        }

        /* all stems in one color */
        plm_stem_style ss_base = { PLM_CYAN, 1.2f, {0}, 0.0f, "spectrum" };
        plm_plot_add_stem(p, fx, fm, n, 0.1, ss_base);

        /* highlight the two resonant peaks with brighter markers */
        {
            float px[2] = { 10.0f, 35.0f };
            float py[2] = { 8.0f, 3.5f };
            plm_stem_style ss_peak = { PLM_YELLOW, 2.0f, PLM_RED, 4.5f, "resonances" };
            plm_plot_add_stem(p, px, py, 2, 0.1, ss_peak);
        }
    }

    /* ================================================================ */
    /* (2,1) — CLIMATE : dual Y-axis, band fill, bar-like rainfall     */
    /* ================================================================ */
    {
        plm_plot *p = plm_figure_plot(&fig, 2, 1);
        p->title   = "Monthly climate summary";
        p->x_label = "Month";
        p->y_label = "Temperature (°C)";
        p->y_label_right = "Rainfall (mm)";
        p->legend_position = PLM_LEGEND_BOTTOM_RIGHT;

        p->x_axis.scale = PLM_CATEGORICAL;
        p->x_axis.num_categories = 12;
        static const char *months[] = {
            "J","F","M","A","M","J","J","A","S","O","N","D"
        };
        p->x_axis.categories = months;
        p->x_axis.grid = 0;

        /* configure right y-axis for rainfall */
        p->y_axis_right.min  = 0.0;
        p->y_axis_right.max  = 180.0;
        p->y_axis_right.tick_hint = 4;

        /* temperature: min/max band + average line (left axis) */
        float t_min[] = { -2, 0, 3, 7,12,16,18,17,14, 9, 4, 0 };
        float t_max[] = {  6, 9,14,18,23,27,29,28,24,18,11, 5 };
        float t_avg[] = {  2, 4, 8,12,17,21,23,22,19,13, 7, 3 };
        float mon[]   = {  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        /* temperature band (min–max range) */
        plm_band_style tbs = {
            PLM_RGBA(220, 80, 70, 90),
            PLM_RGBA(200, 60, 50, 180), 0.8f,
            "T min–max"
        };
        plm_plot_add_band(p, mon, t_min, t_max, 12, tbs);

        /* average temperature line */
        plm_line_style tls = { PLM_RED, 2.5f, "T average", 0 , 0};
        plm_plot_add_line(p, mon, t_avg, 12, tls);

        /* rainfall as stems (maps to right axis) — simulate via bar */
        float rain[] = { 55, 42, 48, 52, 65, 78, 72, 68, 62, 58, 60, 56 };
        /* We can't easily map bars to right axis, so use stems */
        plm_stem_style rss = { PLM_BLUE, 2.0f, {0}, 0.0f, "rainfall" };
        plm_plot_add_stem(p, mon, rain, 12, 0.0, rss);
    }

    /* ---- render ---- */
    plm_figure_render(&fig, &fb);

    /* save screenshot for README */
    plm_fb_save_bmp(&fb, "showcase.bmp");

    plm_fb_swizzle_rgba_bgra(&fb);
    while (mfb_update(win, fb.pixels) >= 0) { /* idle */ }

    plm_figure_reset(&fig);
    free(pixels);
    return 0;
}
