/*
 * mini_gfx.c
 *
 * Minimalist hardware-agnostic graphics library implementation
 */

#include "mini_gfx.h"
#include <stdlib.h>
#include <math.h>

// Global hardware abstraction layer
static gfx_hal_t *g_hal = NULL;

// Clipping rectangle
static struct {
    int16_t x, y, w, h;
    bool enabled;
} clip_rect = {0, 0, 0, 0, false};

// Utility macros
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SWAP(a, b) do { int16_t t = a; a = b; b = t; } while(0)

// Check if point is within clipping bounds
static bool is_clipped(int16_t x, int16_t y) {
    if (!clip_rect.enabled) return false;
    return (x < clip_rect.x || x >= clip_rect.x + clip_rect.w ||
            y < clip_rect.y || y >= clip_rect.y + clip_rect.h);
}

// Clip line coordinates to display bounds
static bool clip_line(int16_t *x0, int16_t *y0, int16_t *x1, int16_t *y1) {
    // Simple bounds checking - could be improved with Cohen-Sutherland
    int16_t width = g_hal->width;
    int16_t height = g_hal->height;
    
    if (clip_rect.enabled) {
        width = MIN(width, clip_rect.x + clip_rect.w);
        height = MIN(height, clip_rect.y + clip_rect.h);
        *x0 = MAX(*x0, clip_rect.x);
        *y0 = MAX(*y0, clip_rect.y);
        *x1 = MAX(*x1, clip_rect.x);
        *y1 = MAX(*y1, clip_rect.y);
    }
    
    if (*x0 < 0 && *x1 < 0) return false;
    if (*y0 < 0 && *y1 < 0) return false;
    if (*x0 >= width && *x1 >= width) return false;
    if (*y0 >= height && *y1 >= height) return false;
    
    return true;
}

// Initialize graphics layer
void gfx_init(gfx_hal_t *hal) {
    g_hal = hal;
    clip_rect.enabled = false;
}

// Basic drawing functions
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (!g_hal || is_clipped(x, y)) return;
    if (x >= 0 && x < g_hal->width && y >= 0 && y < g_hal->height) {
        g_hal->draw_pixel(x, y, color);
    }
}

void gfx_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (!g_hal || w <= 0) return;
    
    // Apply clipping
    if (clip_rect.enabled) {
        if (y < clip_rect.y || y >= clip_rect.y + clip_rect.h) return;
        int16_t x_end = x + w - 1;
        if (x_end < clip_rect.x || x >= clip_rect.x + clip_rect.w) return;
        x = MAX(x, clip_rect.x);
        x_end = MIN(x_end, clip_rect.x + clip_rect.w - 1);
        w = x_end - x + 1;
    }
    
    if (y >= 0 && y < g_hal->height && x < g_hal->width && x + w > 0) {
        if (x < 0) { w += x; x = 0; }
        if (x + w > g_hal->width) w = g_hal->width - x;
        if (w > 0) g_hal->draw_fast_hline(x, y, w, color);
    }
}

void gfx_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (!g_hal || h <= 0) return;
    
    // Apply clipping
    if (clip_rect.enabled) {
        if (x < clip_rect.x || x >= clip_rect.x + clip_rect.w) return;
        int16_t y_end = y + h - 1;
        if (y_end < clip_rect.y || y >= clip_rect.y + clip_rect.h) return;
        y = MAX(y, clip_rect.y);
        y_end = MIN(y_end, clip_rect.y + clip_rect.h - 1);
        h = y_end - y + 1;
    }
    
    if (x >= 0 && x < g_hal->width && y < g_hal->height && y + h > 0) {
        if (y < 0) { h += y; y = 0; }
        if (y + h > g_hal->height) h = g_hal->height - y;
        if (h > 0) g_hal->draw_fast_vline(x, y, h, color);
    }
}

// Bresenham's line algorithm optimized for efficiency
void gfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!g_hal) return;
    
    if (!clip_line(&x0, &y0, &x1, &y1)) return;
    
    // Use fast functions for horizontal and vertical lines
    if (x0 == x1) {
        if (y0 > y1) SWAP(y0, y1);
        gfx_draw_fast_vline(x0, y0, y1 - y0 + 1, color);
        return;
    }
    if (y0 == y1) {
        if (x0 > x1) SWAP(x0, x1);
        gfx_draw_fast_hline(x0, y0, x1 - x0 + 1, color);
        return;
    }
    
    // Bresenham's algorithm
    int16_t dx = ABS(x1 - x0);
    int16_t dy = -ABS(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    
    while (true) {
        gfx_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Rectangle functions
void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    gfx_draw_fast_hline(x, y, w, color);
    gfx_draw_fast_hline(x, y + h - 1, w, color);
    gfx_draw_fast_vline(x, y, h, color);
    gfx_draw_fast_vline(x + w - 1, y, h, color);
}

void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    for (int16_t i = 0; i < h; i++) {
        gfx_draw_fast_hline(x, y + i, w, color);
    }
}

// Circle functions using Bresenham's circle algorithm
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r <= 0) return;
    
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        gfx_draw_pixel(x0 + x, y0 + y, color);
        gfx_draw_pixel(x0 + y, y0 + x, color);
        gfx_draw_pixel(x0 - y, y0 + x, color);
        gfx_draw_pixel(x0 - x, y0 + y, color);
        gfx_draw_pixel(x0 - x, y0 - y, color);
        gfx_draw_pixel(x0 - y, y0 - x, color);
        gfx_draw_pixel(x0 + y, y0 - x, color);
        gfx_draw_pixel(x0 + x, y0 - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r <= 0) return;
    
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        gfx_draw_fast_hline(x0 - x, y0 + y, 2*x + 1, color);
        gfx_draw_fast_hline(x0 - x, y0 - y, 2*x + 1, color);
        gfx_draw_fast_hline(x0 - y, y0 + x, 2*y + 1, color);
        gfx_draw_fast_hline(x0 - y, y0 - x, 2*y + 1, color);
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

// Triangle functions
void gfx_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                       int16_t x2, int16_t y2, uint16_t color) {
    gfx_draw_line(x0, y0, x1, y1, color);
    gfx_draw_line(x1, y1, x2, y2, color);
    gfx_draw_line(x2, y2, x0, y0, color);
}

// Simple scanline triangle fill
void gfx_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                       int16_t x2, int16_t y2, uint16_t color) {
    // Sort vertices by y-coordinate
    if (y0 > y1) { SWAP(x0, x1); SWAP(y0, y1); }
    if (y1 > y2) { SWAP(x1, x2); SWAP(y1, y2); }
    if (y0 > y1) { SWAP(x0, x1); SWAP(y0, y1); }
    
    if (y0 == y2) return; // Degenerate triangle
    
    // Fill triangle using horizontal lines
    for (int16_t y = y0; y <= y2; y++) {
        int16_t xa, xb;
        
        if (y <= y1) {
            // Upper part
            xa = y0 == y1 ? x0 : x0 + ((x1 - x0) * (y - y0)) / (y1 - y0);
        } else {
            // Lower part
            xa = x1 + ((x2 - x1) * (y - y1)) / (y2 - y1);
        }
        
        xb = y0 == y2 ? x0 : x0 + ((x2 - x0) * (y - y0)) / (y2 - y0);
        
        if (xa > xb) SWAP(xa, xb);
        gfx_draw_fast_hline(xa, y, xb - xa + 1, color);
    }
}

// Polygon functions (simple implementation)
void gfx_draw_polygon(int16_t *points, uint8_t num_vertices, uint16_t color) {
    if (num_vertices < 3) return;
    
    for (uint8_t i = 0; i < num_vertices; i++) {
        uint8_t next = (i + 1) % num_vertices;
        gfx_draw_line(points[i*2], points[i*2+1], 
                     points[next*2], points[next*2+1], color);
    }
}

// Simple polygon fill using scanline algorithm
void gfx_fill_polygon(int16_t *points, uint8_t num_vertices, uint16_t color) {
    if (num_vertices < 3) return;
    
    // Find bounding box
    int16_t min_y = points[1], max_y = points[1];
    for (uint8_t i = 1; i < num_vertices; i++) {
        if (points[i*2+1] < min_y) min_y = points[i*2+1];
        if (points[i*2+1] > max_y) max_y = points[i*2+1];
    }
    
    // Scanline fill
    for (int16_t y = min_y; y <= max_y; y++) {
        int16_t intersections[16]; // Max 16 intersections
        uint8_t count = 0;
        
        // Find intersections with polygon edges
        for (uint8_t i = 0; i < num_vertices && count < 16; i++) {
            uint8_t next = (i + 1) % num_vertices;
            int16_t y1 = points[i*2+1];
            int16_t y2 = points[next*2+1];
            
            if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                int16_t x1 = points[i*2];
                int16_t x2 = points[next*2];
                int16_t x = x1 + ((x2 - x1) * (y - y1)) / (y2 - y1);
                intersections[count++] = x;
            }
        }
        
        // Sort intersections
        for (uint8_t i = 0; i < count - 1; i++) {
            for (uint8_t j = i + 1; j < count; j++) {
                if (intersections[i] > intersections[j]) {
                    SWAP(intersections[i], intersections[j]);
                }
            }
        }
        
        // Fill between pairs of intersections
        for (uint8_t i = 0; i < count; i += 2) {
            if (i + 1 < count) {
                gfx_draw_fast_hline(intersections[i], y, 
                                   intersections[i+1] - intersections[i] + 1, color);
            }
        }
    }
}

// Ellipse functions using Bresenham's ellipse algorithm
void gfx_draw_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, uint16_t color) {
    if (a <= 0 || b <= 0) return;
    
    int16_t x = 0;
    int16_t y = b;
    int32_t a2 = a * a;
    int32_t b2 = b * b;
    int32_t err = b2 - a2 * b + a2 / 4;
    
    do {
        gfx_draw_pixel(x0 + x, y0 + y, color);
        gfx_draw_pixel(x0 - x, y0 + y, color);
        gfx_draw_pixel(x0 + x, y0 - y, color);
        gfx_draw_pixel(x0 - x, y0 - y, color);
        
        if (err < 0) {
            err += 2 * b2 * x + b2;
        } else {
            err += 2 * b2 * x - 2 * a2 * y + a2 + b2;
            y--;
        }
        x++;
    } while (x <= a);
    
    x = a;
    y = 0;
    err = a2 - b2 * a + b2 / 4;
    
    do {
        gfx_draw_pixel(x0 + x, y0 + y, color);
        gfx_draw_pixel(x0 - x, y0 + y, color);
        gfx_draw_pixel(x0 + x, y0 - y, color);
        gfx_draw_pixel(x0 - x, y0 - y, color);
        
        if (err < 0) {
            err += 2 * a2 * y + a2;
        } else {
            err += 2 * a2 * y - 2 * b2 * x + a2 + b2;
            x--;
        }
        y++;
    } while (y <= b);
}

void gfx_fill_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, uint16_t color) {
    if (a <= 0 || b <= 0) return;
    
    for (int16_t y = -b; y <= b; y++) {
        int16_t x = (int16_t)(a * sqrt(1.0 - (double)(y * y) / (b * b)));
        gfx_draw_fast_hline(x0 - x, y0 + y, 2*x + 1, color);
    }
}

// Rounded rectangle functions
void gfx_draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                         int16_t r, uint16_t color) {
    if (w <= 0 || h <= 0 || r < 0) return;
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;
    
    // Draw straight edges
    gfx_draw_fast_hline(x + r, y, w - 2*r, color);         // Top
    gfx_draw_fast_hline(x + r, y + h - 1, w - 2*r, color); // Bottom
    gfx_draw_fast_vline(x, y + r, h - 2*r, color);         // Left
    gfx_draw_fast_vline(x + w - 1, y + r, h - 2*r, color); // Right
    
    // Draw corners
    int16_t cx, cy, dx, dy, err;
    cx = r; cy = 0; err = 0;
    
    while (cx >= cy) {
        gfx_draw_pixel(x + r - cx, y + r - cy, color);         // Top-left
        gfx_draw_pixel(x + w - r - 1 + cx, y + r - cy, color); // Top-right
        gfx_draw_pixel(x + r - cx, y + h - r - 1 + cy, color); // Bottom-left
        gfx_draw_pixel(x + w - r - 1 + cx, y + h - r - 1 + cy, color); // Bottom-right
        
        gfx_draw_pixel(x + r - cy, y + r - cx, color);         // Top-left
        gfx_draw_pixel(x + w - r - 1 + cy, y + r - cx, color); // Top-right
        gfx_draw_pixel(x + r - cy, y + h - r - 1 + cx, color); // Bottom-left
        gfx_draw_pixel(x + w - r - 1 + cy, y + h - r - 1 + cx, color); // Bottom-right
        
        if (err <= 0) {
            cy += 1;
            err += 2*cy + 1;
        }
        if (err > 0) {
            cx -= 1;
            err -= 2*cx + 1;
        }
    }
}

void gfx_fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                         int16_t r, uint16_t color) {
    if (w <= 0 || h <= 0 || r < 0) return;
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;
    
    // Fill center rectangle
    gfx_fill_rect(x + r, y, w - 2*r, h, color);
    gfx_fill_rect(x, y + r, r, h - 2*r, color);
    gfx_fill_rect(x + w - r, y + r, r, h - 2*r, color);
    
    // Fill corners with quarter circles
    int16_t cx, cy, err;
    cx = r; cy = 0; err = 0;
    
    while (cx >= cy) {
        gfx_draw_fast_hline(x + r - cx, y + r - cy, cx, color);     // Top-left
        gfx_draw_fast_hline(x + w - r, y + r - cy, cx, color);      // Top-right
        gfx_draw_fast_hline(x + r - cx, y + h - r - 1 + cy, cx, color); // Bottom-left
        gfx_draw_fast_hline(x + w - r, y + h - r - 1 + cy, cx, color);  // Bottom-right
        
        gfx_draw_fast_hline(x + r - cy, y + r - cx, cy, color);     // Top-left
        gfx_draw_fast_hline(x + w - r, y + r - cx, cy, color);      // Top-right
        gfx_draw_fast_hline(x + r - cy, y + h - r - 1 + cx, cy, color); // Bottom-left
        gfx_draw_fast_hline(x + w - r, y + h - r - 1 + cx, cy, color);  // Bottom-right
        
        if (err <= 0) {
            cy += 1;
            err += 2*cy + 1;
        }
        if (err > 0) {
            cx -= 1;
            err -= 2*cx + 1;
        }
    }
}

// Utility functions
void gfx_clear_screen(uint16_t color) {
    if (!g_hal) return;
    gfx_fill_rect(0, 0, g_hal->width, g_hal->height, color);
}

void gfx_invert_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t dy = 0; dy < h; dy++) {
        for (int16_t dx = 0; dx < w; dx++) {
            gfx_draw_pixel(x + dx, y + dy, GFX_INVERSE);
        }
    }
}

// Clipping functions
void gfx_set_clip_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
    clip_rect.x = x;
    clip_rect.y = y;
    clip_rect.w = w;
    clip_rect.h = h;
    clip_rect.enabled = true;
}

void gfx_clear_clip_rect(void) {
    clip_rect.enabled = false;
}

// Get display dimensions
int16_t gfx_get_width(void) {
    return g_hal ? g_hal->width : 0;
}

int16_t gfx_get_height(void) {
    return g_hal ? g_hal->height : 0;
}
