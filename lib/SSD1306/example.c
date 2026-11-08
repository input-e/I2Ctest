/*
 * example.c
 * 
 * Example usage of SSD1306 OLED display driver.
 * This example uses a pretty bare graphic library to demonstrate functionality.
 * 
 * This code was tested on an STM32F103C8T6 (Blue Pill) board with a 128x64 SSD1306-based OLED.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/systick.h>
#include "ssd1306.h"
#include "mini_gfx.h"
    
static ssd1306_t display;

// Hardware abstraction layer functions for mini_gfx
static void hal_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    ssd1306_draw_pixel(&display, x, y, color);
}

static void hal_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint16_t color) {
    ssd1306_draw_fast_hline(&display, x, y, w, color);
}

static void hal_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint16_t color) {
    ssd1306_draw_fast_vline(&display, x, y, h, color);
}

// System clock setup for 72MHz
static void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

// GPIO setup
static void gpio_setup(void) {
    // Enable GPIO clocks
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
}

// I2C example
void i2c_example(void) {
    
    // Enable I2C1 clock
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_GPIOB);
    
    // Configure I2C interface (I2C1 on PB6=SCL, PB7=SDA)
    ssd1306_set_i2c_interface(&display, I2C1, GPIOB, GPIO6, GPIO7, 0x3C);
    
    // Optional: Configure reset pin (PC13)
    ssd1306_set_reset_pin(&display, GPIOC, GPIO13);
    
    // Initialize display (128x64, internal charge pump)
    if (ssd1306_init(&display, 128, 64, SSD1306_SWITCHCAPVCC)) {
        gfx_hal_t hal = {
            .draw_pixel = hal_draw_pixel,
            .draw_fast_hline = hal_draw_fast_hline,
            .draw_fast_vline = hal_draw_fast_vline,
            .width = 128,
            .height = 64
        };

        gfx_init(&hal);

        gfx_clear_screen(GFX_BLACK);

        // Draw rectangles
        gfx_draw_rect(10, 10, 30, 20, GFX_WHITE);
        gfx_fill_rect(50, 10, 20, 20, GFX_WHITE);

        // Draw circles
        gfx_draw_circle(20, 45, 8, GFX_WHITE);
        gfx_fill_circle(50, 45, 8, GFX_WHITE);

        // Draw lines
        gfx_draw_line(80, 10, 120, 30, GFX_WHITE);
        gfx_draw_line(80, 30, 120, 10, GFX_WHITE);

        // Update display
        ssd1306_display(&display);
    }
}

// Hardware SPI example
void spi_hw_example(void) {
    // Enable SPI1 clock
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure SPI interface (SPI1 on PA5=SCK, PA7=MOSI, PA4=CS, PA3=DC)
    ssd1306_set_spi_hw_interface(&display, SPI1, GPIOA, GPIO7, GPIO5, GPIO4, GPIO3);

    // Optional: Configure reset pin (PA2)
    ssd1306_set_reset_pin(&display, GPIOA, GPIO2);

    // Initialize display (128x32, internal charge pump)
    if (ssd1306_init(&display, 128, 32, SSD1306_SWITCHCAPVCC)) {
        // Clear display
        ssd1306_clear_display(&display);

        // Draw a rectangle pattern
        for (int i = 0; i < 128; i += 8) {
            ssd1306_draw_fast_vline(&display, i, 0, 32, SSD1306_WHITE);
        }

        // Update display
        ssd1306_display(&display);

        // Test invert display
        ssd1306_invert_display(&display, true);

        // Clean up when done
        // ssd1306_deinit(&display);
    }
}

// Software SPI example
void spi_sw_example(void) {
    ssd1306_t display;

    // Enable GPIO clock
    rcc_periph_clock_enable(RCC_GPIOB);

    // Configure software SPI interface (PB12=MOSI, PB13=SCK, PB14=CS, PB15=DC)
    ssd1306_set_spi_sw_interface(&display, GPIOB, GPIO12, GPIO13, GPIO14, GPIO15);

    // Optional: Configure reset pin (PB11)
    ssd1306_set_reset_pin(&display, GPIOB, GPIO11);

    // Initialize display (96x16, external VCC)
    if (ssd1306_init(&display, 96, 16, SSD1306_EXTERNALVCC)) {
        // Clear display
        ssd1306_clear_display(&display);

        // Draw a checkerboard pattern
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 96; x++) {
                if ((x / 4 + y / 4) % 2) {
                    ssd1306_draw_pixel(&display, x, y, SSD1306_WHITE);
                }
            }
        }

        // Update display
        ssd1306_display(&display);

        // Test dimming
        ssd1306_dim(&display, true);

        // Clean up when done
        // ssd1306_deinit(&display);
    }
}

// Simple delay function
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < 8000; j++);
    }
}

int main(void) {
    // Initialize system
    clock_setup();
    gpio_setup();

    // Choose one of the examples:

    // For I2C display:
    i2c_example();

    // For hardware SPI display:
    // spi_hw_example();

    // For software SPI display:
    // spi_sw_example();

    // Main loop
    while (1) {
        // Your application code here
        delay_ms(1000);
    }

    return 0;
}
