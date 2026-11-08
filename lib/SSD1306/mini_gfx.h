/*!
 * mini_gfx.h
 *
 * Minimalist hardware-agnostic graphics library for monochrome displays
 */

#ifndef _MINI_GFX_H_
#define _MINI_GFX_H_

#include <stdint.h>
#include <stdbool.h>

// Color definitions (for monochrome displays)
#define GFX_BLACK   0
#define GFX_WHITE   1
#define GFX_INVERSE 2

// Hardware abstraction layer - function pointers to display driver functions
typedef struct {
    void (*draw_pixel)(int16_t x, int16_t y, uint16_t color);
    void (*draw_fast_hline)(int16_t x, int16_t y, int16_t w, uint16_t color);
    void (*draw_fast_vline)(int16_t x, int16_t y, int16_t h, uint16_t color);
    int16_t width;
    int16_t height;
} gfx_hal_t;

// Initialize graphics layer with hardware abstraction
void gfx_init(gfx_hal_t *hal);

// Basic drawing functions
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color);
void gfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void gfx_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint16_t color);
void gfx_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint16_t color);

// Rectangle functions
void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// Circle functions
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

// Triangle functions
void gfx_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                       int16_t x2, int16_t y2, uint16_t color);
void gfx_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                       int16_t x2, int16_t y2, uint16_t color);

// Polygon functions (up to 16 vertices for simplicity)
void gfx_draw_polygon(int16_t *points, uint8_t num_vertices, uint16_t color);
void gfx_fill_polygon(int16_t *points, uint8_t num_vertices, uint16_t color);

// Ellipse functions
void gfx_draw_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, uint16_t color);
void gfx_fill_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, uint16_t color);

// Rounded rectangle functions
void gfx_draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                         int16_t r, uint16_t color);
void gfx_fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                         int16_t r, uint16_t color);

// Utility functions
void gfx_clear_screen(uint16_t color);
void gfx_invert_rect(int16_t x, int16_t y, int16_t w, int16_t h);

// Clipping functions
void gfx_set_clip_rect(int16_t x, int16_t y, int16_t w, int16_t h);
void gfx_clear_clip_rect(void);

// Get display dimensions
int16_t gfx_get_width(void);
int16_t gfx_get_height(void);

#endif // _MINI_GFX_H_
