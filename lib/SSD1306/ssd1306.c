/*!
 * @file ssd1306.c
 *
 * STM32F1 libopencm3 port of Adafruit SSD1306 OLED display library
 * Original library: http://www.adafruit.com/category/63_98
 */

#include "ssd1306.h"
#define rccplug RCC_
//#include <libopencm3/stm32/common/i2c_common_v1.h>
//#include <libopencm3/stm32/f1/gpio.h>
//#include <libopencm3/cm3/systick.h>

// Delay function (requires systick to be configured)
static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < 8000; j++); // Approximate 1ms at 72MHz
    }
}

// Swap macro
#define ssd1306_swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

// Low-level communication functions
static void ssd1306_i2c_write_command(ssd1306_t *display, uint8_t cmd) {
    i2c_send_start(display->config.i2c.i2c);
    while (!((I2C_SR1(display->config.i2c.i2c) & I2C_SR1_SB) 
             & (I2C_SR2(display->config.i2c.i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));
    
    i2c_send_7bit_address(display->config.i2c.i2c, display->config.i2c.address, I2C_WRITE);
    while (!(I2C_SR1(display->config.i2c.i2c) & I2C_SR1_ADDR));
    (void)I2C_SR2(display->config.i2c.i2c); // Clear ADDR flag
    
    i2c_send_data(display->config.i2c.i2c, 0x00); // Command mode
    while (!(I2C_SR1(display->config.i2c.i2c) & I2C_SR1_BTF));
    
    i2c_send_data(display->config.i2c.i2c, cmd);
    while (!(I2C_SR1(display->config.i2c.i2c) & I2C_SR1_BTF));
    
    i2c_send_stop(display->config.i2c.i2c);
}

static void ssd1306_i2c_write_data(ssd1306_t *display, uint8_t *data, uint16_t len) {
    i2c_send_start(display->config.i2c.i2c);
    while (!((I2C_SR1(display->config.i2c.i2c) & I2C_SR1_SB) 
             & (I2C_SR2(display->config.i2c.i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));
    
    i2c_send_7bit_address(display->config.i2c.i2c, display->config.i2c.address, I2C_WRITE);
    while (!(I2C_SR1(display->config.i2c.i2c) & I2C_SR1_ADDR));
    (void)I2C_SR2(display->config.i2c.i2c); // Clear ADDR flag
    
    i2c_send_data(display->config.i2c.i2c, 0x40); // Data mode
    while (!(I2C_SR1(display->config.i2c.i2c) & I2C_SR1_BTF));
    
    for (uint16_t i = 0; i < len; i++) {
        i2c_send_data(display->config.i2c.i2c, data[i]);
        while (!(I2C_SR1(display->config.i2c.i2c) & I2C_SR1_BTF));
    }
    
    i2c_send_stop(display->config.i2c.i2c);
}

static void ssd1306_spi_write_byte(ssd1306_t *display, uint8_t data) {
    if (display->interface == SSD1306_SPI_HW) {
        spi_send(display->config.spi.spi, data);
        while (!(SPI_SR(display->config.spi.spi) & SPI_SR_TXE));
        while (SPI_SR(display->config.spi.spi) & SPI_SR_BSY);
    } else {
        // Software SPI
        for (uint8_t bit = 0x80; bit; bit >>= 1) {
            if (data & bit) {
                gpio_set(display->config.spi.port, display->config.spi.mosi_pin);
            } else {
                gpio_clear(display->config.spi.port, display->config.spi.mosi_pin);
            }
            gpio_set(display->config.spi.port, display->config.spi.sclk_pin);
            gpio_clear(display->config.spi.port, display->config.spi.sclk_pin);
        }
    }
}

static void ssd1306_spi_write_command(ssd1306_t *display, uint8_t cmd) {
    gpio_clear(display->config.spi.port, display->config.spi.cs_pin);
    gpio_clear(display->config.spi.port, display->config.spi.dc_pin); // Command mode
    ssd1306_spi_write_byte(display, cmd);
    gpio_set(display->config.spi.port, display->config.spi.cs_pin);
}

static void ssd1306_spi_write_data(ssd1306_t *display, uint8_t *data, uint16_t len) {
    gpio_clear(display->config.spi.port, display->config.spi.cs_pin);
    gpio_set(display->config.spi.port, display->config.spi.dc_pin); // Data mode
    for (uint16_t i = 0; i < len; i++) {
        ssd1306_spi_write_byte(display, data[i]);
    }
    gpio_set(display->config.spi.port, display->config.spi.cs_pin);
}

// Command wrapper function
void ssd1306_command(ssd1306_t *display, uint8_t cmd) {
    if (display->interface == SSD1306_I2C) {
        ssd1306_i2c_write_command(display, cmd);
    } else {
        ssd1306_spi_write_command(display, cmd);
    }
}

// Interface setup functions
void ssd1306_set_i2c_interface(ssd1306_t *display, uint32_t i2c, uint32_t port, 
                               uint16_t scl_pin, uint16_t sda_pin, uint8_t address) {
    display->interface = SSD1306_I2C;
    display->config.i2c.i2c = i2c;
    display->config.i2c.port = port;
    display->config.i2c.scl_pin = scl_pin;
    display->config.i2c.sda_pin = sda_pin;
    display->config.i2c.address = address;
}

void ssd1306_set_spi_hw_interface(ssd1306_t *display, uint32_t spi, uint32_t port,
                                  uint16_t mosi_pin, uint16_t sclk_pin, 
                                  uint16_t cs_pin, uint16_t dc_pin) {
    display->interface = SSD1306_SPI_HW;
    display->config.spi.spi = spi;
    display->config.spi.port = port;
    display->config.spi.mosi_pin = mosi_pin;
    display->config.spi.sclk_pin = sclk_pin;
    display->config.spi.cs_pin = cs_pin;
    display->config.spi.dc_pin = dc_pin;
}

void ssd1306_set_spi_sw_interface(ssd1306_t *display, uint32_t port,
                                  uint16_t mosi_pin, uint16_t sclk_pin, 
                                  uint16_t cs_pin, uint16_t dc_pin) {
    display->interface = SSD1306_SPI_SW;
    display->config.spi.spi = 0; // Not used for software SPI
    display->config.spi.port = port;
    display->config.spi.mosi_pin = mosi_pin;
    display->config.spi.sclk_pin = sclk_pin;
    display->config.spi.cs_pin = cs_pin;
    display->config.spi.dc_pin = dc_pin;
}

void ssd1306_set_reset_pin(ssd1306_t *display, uint32_t port, uint16_t pin) {
    display->rst_port = port;
    display->rst_pin = pin;
    display->rst_enabled = true;
}

// Initialize display
bool ssd1306_init(ssd1306_t *display, uint8_t width, uint8_t height, uint8_t vccstate) {
    display->width = width;
    display->height = height;
    display->vccstate = vccstate;
    display->rotation = 0;
    display->rst_enabled = false;
    
    // Allocate display buffer
    uint16_t buffer_size = width * ((height + 7) / 8);
    display->buffer = malloc(buffer_size);
    if (!display->buffer) {
        return false;
    }
    
    // Clear buffer
    memset(display->buffer, 0, buffer_size);
    
    // Configure GPIO and peripherals based on interface
    if (display->interface == SSD1306_I2C) {
        // Configure I2C pins
        rcc_periph_clock_enable(RCC_GPIOA);
        gpio_mode_setup(display->config.i2c.port, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, 
            display->config.i2c.scl_pin | display->config.i2c.sda_pin);
        rcc_periph_clock_enable(RCC_I2C1);
        i2c_set_speed(I2C1, i2c_speed_sm_100k, rcc_apb1_frequency / 1000000);
        i2c_peripheral_enable(display->config.i2c.i2c);
        /*
        gpio_set_mode(display->config.i2c.port, GPIO_MODE_OUTPUT_50_MHZ, 
                     GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, 
                     display->config.i2c.scl_pin | display->config.i2c.sda_pin);
        
        // Configure I2C peripheral
        i2c_peripheral_disable(display->config.i2c.i2c);
        i2c_set_standard_mode(display->config.i2c.i2c);
        // Set I2C clock to 36kHz
        i2c_set_clock_frequency(display->config.i2c.i2c, 36);
        i2c_set_trise(display->config.i2c.i2c, 0x25);
        i2c_set_dutycycle(display->config.i2c.i2c, I2C_CCR_DUTY_DIV2);
        i2c_set_ccr(display->config.i2c.i2c, 0xB4);
        i2c_peripheral_enable(display->config.i2c.i2c);
        */
        
    } 
    /*
    else if (display->interface == SSD1306_SPI_HW) {
        // Configure SPI pins
        gpio_set_mode(display->config.spi.port, GPIO_MODE_OUTPUT_50_MHZ,
                     GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, 
                     display->config.spi.mosi_pin | display->config.spi.sclk_pin);
        gpio_set_mode(display->config.spi.port, GPIO_MODE_OUTPUT_50_MHZ,
                     GPIO_CNF_OUTPUT_PUSHPULL,
                     display->config.spi.cs_pin | display->config.spi.dc_pin);
        
        // Configure SPI peripheral
        spi_disable(display->config.spi.spi);
        spi_init_master(display->config.spi.spi, SPI_CR1_BAUDRATE_FPCLK_DIV_8,
                       SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_1,
                       SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
        spi_enable_software_slave_management(display->config.spi.spi);
        spi_set_nss_high(display->config.spi.spi);
        spi_enable(display->config.spi.spi);
        
        // Set CS and DC high initially
        gpio_set(display->config.spi.port, display->config.spi.cs_pin);
        gpio_clear(display->config.spi.port, display->config.spi.dc_pin);
    }

    
        
    else { // Software SPI
        // Configure all pins as outputs
        gpio_set_mode(display->config.spi.port, GPIO_MODE_OUTPUT_50_MHZ,
                     GPIO_CNF_OUTPUT_PUSHPULL,
                     display->config.spi.mosi_pin | display->config.spi.sclk_pin |
                     display->config.spi.cs_pin | display->config.spi.dc_pin);
        
        // Set initial pin states
        gpio_clear(display->config.spi.port, display->config.spi.sclk_pin);
        gpio_set(display->config.spi.port, display->config.spi.cs_pin);
        gpio_clear(display->config.spi.port, display->config.spi.dc_pin);
    }
    
    // Configure reset pin if enabled
    if (display->rst_enabled) {
        gpio_set_mode(display->rst_port, GPIO_MODE_OUTPUT_50_MHZ,
                     GPIO_CNF_OUTPUT_PUSHPULL, display->rst_pin);
    }
    */
    // Perform reset sequence
    if (display->rst_enabled) {
        gpio_set(display->rst_port, display->rst_pin);
        delay_ms(1);
        gpio_clear(display->rst_port, display->rst_pin);
        delay_ms(10);
        gpio_set(display->rst_port, display->rst_pin);
    }
    
    // Initialize display with command sequence
    static const uint8_t init1[] = {
        SSD1306_DISPLAYOFF,         // 0xAE
        SSD1306_SETDISPLAYCLOCKDIV, // 0xD5
        0x80,                       // the suggested ratio 0x80
        SSD1306_SETMULTIPLEX        // 0xA8
    };
    
    for (uint8_t i = 0; i < sizeof(init1); i++) {
        ssd1306_command(display, init1[i]);
    }
    ssd1306_command(display, height - 1);
    
    static const uint8_t init2[] = {
        SSD1306_SETDISPLAYOFFSET,   // 0xD3
        0x0,                        // no offset
        SSD1306_SETSTARTLINE | 0x0, // line #0
        SSD1306_CHARGEPUMP          // 0x8D
    };
    
    for (uint8_t i = 0; i < sizeof(init2); i++) {
        ssd1306_command(display, init2[i]);
    }
    
    ssd1306_command(display, (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0x14);
    
    static const uint8_t init3[] = {
        SSD1306_MEMORYMODE,     // 0x20
        0x00,                   // 0x0 act like ks0108
        SSD1306_SEGREMAP | 0x1,
        SSD1306_COMSCANDEC
    };
    
    for (uint8_t i = 0; i < sizeof(init3); i++) {
        ssd1306_command(display, init3[i]);
    }
    
    uint8_t comPins = 0x02;
    display->contrast = 0x8F;
    
    if ((width == 128) && (height == 32)) {
        comPins = 0x02;
        display->contrast = 0x8F;
    } else if ((width == 128) && (height == 64)) {
        comPins = 0x12;
        display->contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x9F : 0xCF;
    } else if ((width == 96) && (height == 16)) {
        comPins = 0x2;
        display->contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0xAF;
    }
    
    ssd1306_command(display, SSD1306_SETCOMPINS);
    ssd1306_command(display, comPins);
    ssd1306_command(display, SSD1306_SETCONTRAST);
    ssd1306_command(display, display->contrast);
    
    ssd1306_command(display, SSD1306_SETPRECHARGE);
    ssd1306_command(display, (vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1);
    
    static const uint8_t init5[] = {
        SSD1306_SETVCOMDETECT,       // 0xDB
        0x40,
        SSD1306_DISPLAYALLON_RESUME, // 0xA4
        SSD1306_NORMALDISPLAY,       // 0xA6
        SSD1306_DEACTIVATE_SCROLL,
        SSD1306_DISPLAYON            // Main screen turn on
    };
    
    for (uint8_t i = 0; i < sizeof(init5); i++) {
        ssd1306_command(display, init5[i]);
    }
    
    return true;
}

void ssd1306_deinit(ssd1306_t *display) {
    if (display->buffer) {
        free(display->buffer);
        display->buffer = NULL;
    }
}

// Display control functions
void ssd1306_display(ssd1306_t *display) {
    static const uint8_t dlist1[] = {
        SSD1306_PAGEADDR,
        0,              // Page start address
        0xFF,           // Page end (not really, but works here)
        SSD1306_COLUMNADDR
    };
    
    for (uint8_t i = 0; i < sizeof(dlist1); i++) {
        ssd1306_command(display, dlist1[i]);
    }
    
    ssd1306_command(display, 0);                    // Column start
    ssd1306_command(display, display->width - 1);   // Column end
    
    uint16_t count = display->width * ((display->height + 7) / 8);
    
    if (display->interface == SSD1306_I2C) {
        ssd1306_i2c_write_data(display, display->buffer, count);
    } else {
        ssd1306_spi_write_data(display, display->buffer, count);
    }
}

void ssd1306_clear_display(ssd1306_t *display) {
    memset(display->buffer, 0, display->width * ((display->height + 7) / 8));
}

void ssd1306_invert_display(ssd1306_t *display, bool invert) {
    ssd1306_command(display, invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

void ssd1306_dim(ssd1306_t *display, bool dim) {
    ssd1306_command(display, SSD1306_SETCONTRAST);
    ssd1306_command(display, dim ? 0 : display->contrast);
}

// Drawing functions
void ssd1306_draw_pixel(ssd1306_t *display, int16_t x, int16_t y, uint16_t color) {
    if ((x >= 0) && (x < display->width) && (y >= 0) && (y < display->height)) {
        // Handle rotation
        switch (display->rotation) {
        case 1:
            ssd1306_swap(x, y);
            x = display->width - x - 1;
            break;
        case 2:
            x = display->width - x - 1;
            y = display->height - y - 1;
            break;
        case 3:
            ssd1306_swap(x, y);
            y = display->height - y - 1;
            break;
        }
        
        switch (color) {
        case SSD1306_WHITE:
            display->buffer[x + (y / 8) * display->width] |= (1 << (y & 7));
            break;
        case SSD1306_BLACK:
            display->buffer[x + (y / 8) * display->width] &= ~(1 << (y & 7));
            break;
        case SSD1306_INVERSE:
            display->buffer[x + (y / 8) * display->width] ^= (1 << (y & 7));
            break;
        }
    }
}

bool ssd1306_get_pixel(ssd1306_t *display, int16_t x, int16_t y) {
    if ((x >= 0) && (x < display->width) && (y >= 0) && (y < display->height)) {
        // Handle rotation
        switch (display->rotation) {
        case 1:
            ssd1306_swap(x, y);
            x = display->width - x - 1;
            break;
        case 2:
            x = display->width - x - 1;
            y = display->height - y - 1;
            break;
        case 3:
            ssd1306_swap(x, y);
            y = display->height - y - 1;
            break;
        }
        return (display->buffer[x + (y / 8) * display->width] & (1 << (y & 7)));
    }
    return false;
}

static void ssd1306_draw_fast_hline_internal(ssd1306_t *display, int16_t x, int16_t y, int16_t w, uint16_t color) {
    if ((y >= 0) && (y < display->height)) {
        if (x < 0) {
            w += x;
            x = 0;
        }
        if ((x + w) > display->width) {
            w = (display->width - x);
        }
        if (w > 0) {
            uint8_t *pBuf = &display->buffer[(y / 8) * display->width + x];
            uint8_t mask = 1 << (y & 7);
            switch (color) {
            case SSD1306_WHITE:
                while (w--) {
                    *pBuf++ |= mask;
                }
                break;
            case SSD1306_BLACK:
                mask = ~mask;
                while (w--) {
                    *pBuf++ &= mask;
                }
                break;
            case SSD1306_INVERSE:
                while (w--) {
                    *pBuf++ ^= mask;
                }
                break;
            }
        }
    }
}

static void ssd1306_draw_fast_vline_internal(ssd1306_t *display, int16_t x, int16_t __y, int16_t __h, uint16_t color) {
    if ((x >= 0) && (x < display->width)) {
        if (__y < 0) {
            __h += __y;
            __y = 0;
        }
        if ((__y + __h) > display->height) {
            __h = (display->height - __y);
        }
        if (__h > 0) {
            uint8_t y = __y, h = __h;
            uint8_t *pBuf = &display->buffer[(y / 8) * display->width + x];

            uint8_t mod = (y & 7);
            if (mod) {
                mod = 8 - mod;
                static const uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
                uint8_t mask = premask[mod];
                if (h < mod)
                    mask &= (0XFF >> (mod - h));

                switch (color) {
                case SSD1306_WHITE:
                    *pBuf |= mask;
                    break;
                case SSD1306_BLACK:
                    *pBuf &= ~mask;
                    break;
                case SSD1306_INVERSE:
                    *pBuf ^= mask;
                    break;
                }
                pBuf += display->width;
            }

            if (h >= mod) {
                h -= mod;
                if (h >= 8) {
                    if (color == SSD1306_INVERSE) {
                        do {
                            *pBuf ^= 0xFF;
                            pBuf += display->width;
                            h -= 8;
                        } while (h >= 8);
                    } else {
                        uint8_t val = (color != SSD1306_BLACK) ? 255 : 0;
                        do {
                            *pBuf = val;
                            pBuf += display->width;
                            h -= 8;
                        } while (h >= 8);
                    }
                }

                if (h) {
                    mod = h & 7;
                    static const uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
                    uint8_t mask = postmask[mod];
                    switch (color) {
                    case SSD1306_WHITE:
                        *pBuf |= mask;
                        break;
                    case SSD1306_BLACK:
                        *pBuf &= ~mask;
                        break;
                    case SSD1306_INVERSE:
                        *pBuf ^= mask;
                        break;
                    }
                }
            }
        }
    }
}

void ssd1306_draw_fast_hline(ssd1306_t *display, int16_t x, int16_t y, int16_t w, uint16_t color) {
    bool bSwap = false;
    switch (display->rotation) {
    case 1:
        bSwap = true;
        ssd1306_swap(x, y);
        x = display->width - x - 1;
        break;
    case 2:
        x = display->width - x - 1;
        y = display->height - y - 1;
        x -= (w - 1);
        break;
    case 3:
        bSwap = true;
        ssd1306_swap(x, y);
        y = display->height - y - 1;
        y -= (w - 1);
        break;
    }

    if (bSwap)
        ssd1306_draw_fast_vline_internal(display, x, y, w, color);
    else
        ssd1306_draw_fast_hline_internal(display, x, y, w, color);
}

void ssd1306_draw_fast_vline(ssd1306_t *display, int16_t x, int16_t y, int16_t h, uint16_t color) {
    bool bSwap = false;
    switch (display->rotation) {
    case 1:
        bSwap = true;
        ssd1306_swap(x, y);
        x = display->width - x - 1;
        x -= (h - 1);
        break;
    case 2:
        x = display->width - x - 1;
        y = display->height - y - 1;
        y -= (h - 1);
        break;
    case 3:
        bSwap = true;
        ssd1306_swap(x, y);
        y = display->height - y - 1;
        break;
    }

    if (bSwap)
        ssd1306_draw_fast_hline_internal(display, x, y, h, color);
    else
        ssd1306_draw_fast_vline_internal(display, x, y, h, color);
}

// Scrolling functions
void ssd1306_start_scroll_right(ssd1306_t *display, uint8_t start, uint8_t stop) {
    static const uint8_t scrollList1a[] = {SSD1306_RIGHT_HORIZONTAL_SCROLL, 0X00};
    for (uint8_t i = 0; i < sizeof(scrollList1a); i++) {
        ssd1306_command(display, scrollList1a[i]);
    }
    ssd1306_command(display, start);
    ssd1306_command(display, 0X00);
    ssd1306_command(display, stop);
    static const uint8_t scrollList1b[] = {0X00, 0XFF, SSD1306_ACTIVATE_SCROLL};
    for (uint8_t i = 0; i < sizeof(scrollList1b); i++) {
        ssd1306_command(display, scrollList1b[i]);
    }
}

void ssd1306_start_scroll_left(ssd1306_t *display, uint8_t start, uint8_t stop) {
    static const uint8_t scrollList2a[] = {SSD1306_LEFT_HORIZONTAL_SCROLL, 0X00};
    for (uint8_t i = 0; i < sizeof(scrollList2a); i++) {
        ssd1306_command(display, scrollList2a[i]);
    }
    ssd1306_command(display, start);
    ssd1306_command(display, 0X00);
    ssd1306_command(display, stop);
    static const uint8_t scrollList2b[] = {0X00, 0XFF, SSD1306_ACTIVATE_SCROLL};
    for (uint8_t i = 0; i < sizeof(scrollList2b); i++) {
        ssd1306_command(display, scrollList2b[i]);
    }
}

void ssd1306_start_scroll_diag_right(ssd1306_t *display, uint8_t start, uint8_t stop) {
    static const uint8_t scrollList3a[] = {SSD1306_SET_VERTICAL_SCROLL_AREA, 0X00};
    for (uint8_t i = 0; i < sizeof(scrollList3a); i++) {
        ssd1306_command(display, scrollList3a[i]);
    }
    ssd1306_command(display, display->height);
    static const uint8_t scrollList3b[] = {SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL, 0X00};
    for (uint8_t i = 0; i < sizeof(scrollList3b); i++) {
        ssd1306_command(display, scrollList3b[i]);
    }
    ssd1306_command(display, start);
    ssd1306_command(display, 0X00);
    ssd1306_command(display, stop);
    static const uint8_t scrollList3c[] = {0X01, SSD1306_ACTIVATE_SCROLL};
    for (uint8_t i = 0; i < sizeof(scrollList3c); i++) {
        ssd1306_command(display, scrollList3c[i]);
    }
}

void ssd1306_start_scroll_diag_left(ssd1306_t *display, uint8_t start, uint8_t stop) {
    static const uint8_t scrollList4a[] = {SSD1306_SET_VERTICAL_SCROLL_AREA, 0X00};
    for (uint8_t i = 0; i < sizeof(scrollList4a); i++) {
        ssd1306_command(display, scrollList4a[i]);
    }
    ssd1306_command(display, display->height);
    static const uint8_t scrollList4b[] = {SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL, 0X00};
    for (uint8_t i = 0; i < sizeof(scrollList4b); i++) {
        ssd1306_command(display, scrollList4b[i]);
    }
    ssd1306_command(display, start);
    ssd1306_command(display, 0X00);
    ssd1306_command(display, stop);
    static const uint8_t scrollList4c[] = {0X01, SSD1306_ACTIVATE_SCROLL};
    for (uint8_t i = 0; i < sizeof(scrollList4c); i++) {
        ssd1306_command(display, scrollList4c[i]);
    }
}

void ssd1306_stop_scroll(ssd1306_t *display) {
    ssd1306_command(display, SSD1306_DEACTIVATE_SCROLL);
}

// Utility functions
void ssd1306_set_rotation(ssd1306_t *display, uint8_t rotation) {
    display->rotation = rotation & 3;
}

uint8_t ssd1306_get_rotation(ssd1306_t *display) {
    return display->rotation;
}

uint8_t *ssd1306_get_buffer(ssd1306_t *display) {
    return display->buffer;
}
