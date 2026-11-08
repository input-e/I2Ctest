#ifndef _SSD1306_H_
#define _SSD1306_H_

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Display Color Definitions
#define SSD1306_BLACK 0
#define SSD1306_WHITE 1
#define SSD1306_INVERSE 2

// SSD1306 Command Definitions
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_SETCONTRAST 0x81         ///< See datasheet
#define SSD1306_CHARGEPUMP 0x8D          ///< See datasheet
#define SSD1306_SEGREMAP 0xA0            ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON 0xA5        ///< Not currently used
#define SSD1306_NORMALDISPLAY 0xA6       ///< See datasheet
#define SSD1306_INVERTDISPLAY 0xA7       ///< See datasheet
#define SSD1306_SETMULTIPLEX 0xA8        ///< See datasheet
#define SSD1306_DISPLAYOFF 0xAE          ///< See datasheet
#define SSD1306_DISPLAYON 0xAF           ///< See datasheet
#define SSD1306_COMSCANINC 0xC0          ///< Not currently used
#define SSD1306_COMSCANDEC 0xC8          ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET 0xD3    ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5  ///< See datasheet
#define SSD1306_SETPRECHARGE 0xD9        ///< See datasheet
#define SSD1306_SETCOMPINS 0xDA          ///< See datasheet
#define SSD1306_SETVCOMDETECT 0xDB       ///< See datasheet

#define SSD1306_SETLOWCOLUMN 0x00  ///< Not currently used
#define SSD1306_SETHIGHCOLUMN 0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE 0x40  ///< See datasheet

#define SSD1306_EXTERNALVCC 0x01  ///< External display voltage source
#define SSD1306_SWITCHCAPVCC 0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26              ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27               ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A  ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL 0x2E                    ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL 0x2F                      ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3             ///< Set scroll range

// Interface types
typedef enum {
    SSD1306_I2C,
    SSD1306_SPI_HW,
    SSD1306_SPI_SW
} ssd1306_interface_t;

// I2C configuration structure
typedef struct {
    uint32_t i2c;           // I2C peripheral (I2C1, I2C2)
    uint32_t port;          // GPIO port for I2C pins
    uint16_t scl_pin;       // SCL pin
    uint16_t sda_pin;       // SDA pin
    uint8_t address;        // I2C address (0x3C or 0x3D typically)
} ssd1306_i2c_config_t;

// SPI configuration structure
typedef struct {
    uint32_t spi;           // SPI peripheral (SPI1, SPI2)
    uint32_t port;          // GPIO port for SPI pins
    uint16_t mosi_pin;      // MOSI pin
    uint16_t sclk_pin;      // SCLK pin
    uint16_t cs_pin;        // Chip select pin
    uint16_t dc_pin;        // Data/Command pin
} ssd1306_spi_config_t;

// Main SSD1306 structure
typedef struct {
    // Display parameters
    uint8_t width;
    uint8_t height;
    uint8_t *buffer;
    
    // Interface configuration
    ssd1306_interface_t interface;
    union {
        ssd1306_i2c_config_t i2c;
        ssd1306_spi_config_t spi;
    } config;
    
    // Common pins
    uint32_t rst_port;
    uint16_t rst_pin;
    bool rst_enabled;
    
    // Display state
    uint8_t vccstate;
    uint8_t contrast;
    uint8_t rotation;
} ssd1306_t;

// Function prototypes
bool ssd1306_init(ssd1306_t *display, uint8_t width, uint8_t height, uint8_t vccstate);
void ssd1306_deinit(ssd1306_t *display);

// I2C interface setup
void ssd1306_set_i2c_interface(ssd1306_t *display, uint32_t i2c, uint32_t port, 
                               uint16_t scl_pin, uint16_t sda_pin, uint8_t address);

// Hardware SPI interface setup
void ssd1306_set_spi_hw_interface(ssd1306_t *display, uint32_t spi, uint32_t port,
                                  uint16_t mosi_pin, uint16_t sclk_pin, 
                                  uint16_t cs_pin, uint16_t dc_pin);

// Software SPI interface setup
void ssd1306_set_spi_sw_interface(ssd1306_t *display, uint32_t port,
                                  uint16_t mosi_pin, uint16_t sclk_pin, 
                                  uint16_t cs_pin, uint16_t dc_pin);

// Reset pin configuration
void ssd1306_set_reset_pin(ssd1306_t *display, uint32_t port, uint16_t pin);

// Display control functions
void ssd1306_display(ssd1306_t *display);
void ssd1306_clear_display(ssd1306_t *display);
void ssd1306_invert_display(ssd1306_t *display, bool invert);
void ssd1306_dim(ssd1306_t *display, bool dim);

// Drawing functions
void ssd1306_draw_pixel(ssd1306_t *display, int16_t x, int16_t y, uint16_t color);
bool ssd1306_get_pixel(ssd1306_t *display, int16_t x, int16_t y);
void ssd1306_draw_fast_hline(ssd1306_t *display, int16_t x, int16_t y, int16_t w, uint16_t color);
void ssd1306_draw_fast_vline(ssd1306_t *display, int16_t x, int16_t y, int16_t h, uint16_t color);

// Scrolling functions
void ssd1306_start_scroll_right(ssd1306_t *display, uint8_t start, uint8_t stop);
void ssd1306_start_scroll_left(ssd1306_t *display, uint8_t start, uint8_t stop);
void ssd1306_start_scroll_diag_right(ssd1306_t *display, uint8_t start, uint8_t stop);
void ssd1306_start_scroll_diag_left(ssd1306_t *display, uint8_t start, uint8_t stop);
void ssd1306_stop_scroll(ssd1306_t *display);

// Low-level command function
void ssd1306_command(ssd1306_t *display, uint8_t cmd);

// Buffer access
uint8_t *ssd1306_get_buffer(ssd1306_t *display);

// Utility functions
void ssd1306_set_rotation(ssd1306_t *display, uint8_t rotation);
uint8_t ssd1306_get_rotation(ssd1306_t *display);

#endif // _SSD1306_H_
