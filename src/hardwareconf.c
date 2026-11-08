#include "hardwareconf.h"

void ssdset(){
    ssd1306_set_i2c_interface(&Display, I2C1, GPIOA, GPIO9, GPIO10, 0x3C);
    ssd1306_init(&Display, 128, 64, SSD1306_SWITCHCAPVCC);
}

void RCCconf(){
    rcc_clock_setup_in_hse_8mhz_out_48mhz();
}
