#define PLOTMINI_IMPLEMENTATION
#include "../plotmini.h"
#include "test_runner.h"

/* ---- RGBA8 tests ---- */

TEST(fb_create_rgba)
{
    unsigned char buf[16 * 16 * 4];
    plm_fb fb = plm_fb_create(buf, 16, 16, PLM_RGBA8);

    ASSERT_EQ(fb.width,   16);
    ASSERT_EQ(fb.height,  16);
    ASSERT_EQ(fb.bpp,     4);
    ASSERT_EQ(fb.stride,  16 * 4);
    ASSERT_TRUE(fb.pixels == buf);
    return 0;
}

TEST(fb_create_gray)
{
    unsigned char buf[8 * 8];
    plm_fb fb = plm_fb_create(buf, 8, 8, PLM_GRAY8);

    ASSERT_EQ(fb.width,   8);
    ASSERT_EQ(fb.height,  8);
    ASSERT_EQ(fb.bpp,     1);
    ASSERT_EQ(fb.stride,  8);
    ASSERT_TRUE(fb.pixels == buf);
    return 0;
}

TEST(fb_create_null_pixels)
{
    /* When pixels==NULL, the library allocates internally */
    plm_fb fb = plm_fb_create(NULL, 10, 10, PLM_RGBA8);
    ASSERT_EQ(fb.width,  10);
    ASSERT_EQ(fb.height, 10);
    ASSERT_EQ(fb.bpp,    4);
    ASSERT_EQ(fb.stride, 40);
    ASSERT_TRUE(fb.pixels != NULL);  /* allocated by library */
    PLOTMINI_FREE(fb.pixels);
    return 0;
}

/* ---- clear tests ---- */

TEST(fb_clear_rgba_white)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0xAB, sizeof(buf));  /* fill with garbage */

    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_WHITE);

    /* every pixel should be white */
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char *p = buf + y * fb.stride + x * 4;
            ASSERT_EQ(p[0], 255);  /* R */
            ASSERT_EQ(p[1], 255);  /* G */
            ASSERT_EQ(p[2], 255);  /* B */
            ASSERT_EQ(p[3], 255);  /* A */
        }
    }
    return 0;
}

TEST(fb_clear_rgba_black)
{
    unsigned char buf[2 * 2 * 4];
    memset(buf, 0xFF, sizeof(buf));

    plm_fb fb = plm_fb_create(buf, 2, 2, PLM_RGBA8);
    plm_fb_clear(&fb, PLM_BLACK);

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char *p = buf + y * fb.stride + x * 4;
            ASSERT_EQ(p[0], 0);
            ASSERT_EQ(p[1], 0);
            ASSERT_EQ(p[2], 0);
            ASSERT_EQ(p[3], 255);
        }
    }
    return 0;
}

TEST(fb_clear_gray_white)
{
    unsigned char buf[3 * 3];
    memset(buf, 0x00, sizeof(buf));

    plm_fb fb = plm_fb_create(buf, 3, 3, PLM_GRAY8);
    plm_fb_clear(&fb, PLM_WHITE);

    for (int i = 0; i < 3 * 3; i++) {
        ASSERT_EQ(buf[i], 255);
    }
    return 0;
}

/* ---- fill_rect tests ---- */

TEST(fill_rect_full)
{
    unsigned char buf[4 * 4 * 4];
    memset(buf, 0x00, sizeof(buf));

    plm_fb fb = plm_fb_create(buf, 4, 4, PLM_RGBA8);
    plm_irect r = { 0, 0, 4, 4 };
    plm_fb_fill_rect(&fb, r, PLM_RED);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char *p = buf + y * fb.stride + x * 4;
            ASSERT_EQ(p[0], 255);
            ASSERT_EQ(p[1], 0);
            ASSERT_EQ(p[2], 0);
            ASSERT_EQ(p[3], 255);
        }
    }
    return 0;
}

TEST(fill_rect_partial)
{
    unsigned char buf[6 * 6 * 4];
    memset(buf, 0x00, sizeof(buf));

    plm_fb fb = plm_fb_create(buf, 6, 6, PLM_RGBA8);
    plm_irect r = { 2, 1, 5, 4 };  /* cols 2-4, rows 1-3 */
    plm_fb_fill_rect(&fb, r, PLM_BLUE);

    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 6; x++) {
            unsigned char *p = buf + y * fb.stride + x * 4;
            if (x >= 2 && x < 5 && y >= 1 && y < 4) {
                ASSERT_EQ(p[2], 255); /* blue */
                ASSERT_EQ(p[0], 0);
            } else {
                /* still zero */
                ASSERT_EQ(p[0], 0);
                ASSERT_EQ(p[2], 0);
            }
        }
    }
    return 0;
}

TEST(fill_rect_clipped)
{
    unsigned char buf[3 * 3 * 4];
    memset(buf, 0x00, sizeof(buf));

    plm_fb fb = plm_fb_create(buf, 3, 3, PLM_RGBA8);
    plm_irect r = { -5, -5, 10, 10 };  /* way bigger than fb */
    plm_fb_fill_rect(&fb, r, PLM_GREEN);

    /* all pixels should be green */
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            unsigned char *p = buf + y * fb.stride + x * 4;
            ASSERT_EQ(p[1], 255);
            ASSERT_EQ(p[0], 0);
            ASSERT_EQ(p[2], 0);
        }
    }
    return 0;
}

/* ---- pixel blending ---- */

TEST(blend_pixel_opaque)
{
    unsigned char buf[4];
    memset(buf, 0, 4);

    plm_fb fb = plm_fb_create(buf, 1, 1, PLM_RGBA8);
    plm__blend_pixel(&fb, 0, 0, PLM_RED, 255);

    ASSERT_EQ(buf[0], 255); /* R */
    ASSERT_EQ(buf[1], 0);
    ASSERT_EQ(buf[2], 0);
    return 0;
}

TEST(blend_pixel_transparent)
{
    unsigned char buf[4];
    buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 255;

    plm_fb fb = plm_fb_create(buf, 1, 1, PLM_RGBA8);
    plm__blend_pixel(&fb, 0, 0, PLM_RED, 0);  /* alpha=0, no-op */

    /* unchanged */
    ASSERT_EQ(buf[0], 10);
    ASSERT_EQ(buf[1], 20);
    ASSERT_EQ(buf[2], 30);
    return 0;
}

TEST(blend_pixel_half)
{
    unsigned char buf[4];
    memset(buf, 0, 4);  /* black background */

    plm_fb fb = plm_fb_create(buf, 1, 1, PLM_RGBA8);
    plm__blend_pixel(&fb, 0, 0, PLM_WHITE, 128);  /* 50% white over black */

    /* result ~ 128 (127-128 due to integer rounding) */
    ASSERT_TRUE(buf[0] >= 127 && buf[0] <= 128);
    ASSERT_TRUE(buf[1] >= 127 && buf[1] <= 128);
    ASSERT_TRUE(buf[2] >= 127 && buf[2] <= 128);
    return 0;
}

/* ---- swizzle ---- */

TEST(swizzle_rgba_bgra)
{
    unsigned char buf[8] = {
        0xAA, 0xBB, 0xCC, 0xDD,   /* pixel 0 */
        0x11, 0x22, 0x33, 0x44    /* pixel 1 */
    };
    plm_fb fb = plm_fb_create(buf, 2, 1, PLM_RGBA8);
    plm_fb_swizzle_rgba_bgra(&fb);

    /* pixel 0: R<->B swapped */
    ASSERT_EQ(buf[0], 0xCC); /* was B */
    ASSERT_EQ(buf[1], 0xBB); /* G unchanged */
    ASSERT_EQ(buf[2], 0xAA); /* was R */
    ASSERT_EQ(buf[3], 0xDD); /* A unchanged */

    /* pixel 1 */
    ASSERT_EQ(buf[4], 0x33);
    ASSERT_EQ(buf[5], 0x22);
    ASSERT_EQ(buf[6], 0x11);
    ASSERT_EQ(buf[7], 0x44);
    return 0;
}

/* ---- PLM_RGBA macro ---- */

TEST(rgba_macro)
{
    plm_color c = PLM_RGBA(10, 20, 30, 40);
    ASSERT_EQ(c.r, 10);
    ASSERT_EQ(c.g, 20);
    ASSERT_EQ(c.b, 30);
    ASSERT_EQ(c.a, 40);
    return 0;
}

TEST_MAIN()
{
    RUN_TEST(fb_create_rgba);
    RUN_TEST(fb_create_gray);
    RUN_TEST(fb_create_null_pixels);
    RUN_TEST(fb_clear_rgba_white);
    RUN_TEST(fb_clear_rgba_black);
    RUN_TEST(fb_clear_gray_white);
    RUN_TEST(fill_rect_full);
    RUN_TEST(fill_rect_partial);
    RUN_TEST(fill_rect_clipped);
    RUN_TEST(blend_pixel_opaque);
    RUN_TEST(blend_pixel_transparent);
    RUN_TEST(blend_pixel_half);
    RUN_TEST(swizzle_rgba_bgra);
    RUN_TEST(rgba_macro);
}
TEST_MAIN_END()
