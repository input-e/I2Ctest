#include "main.h"


int main(void){
    ssd1306_basic_init(SSD1306_INTERFACE_IIC, 0x3C);
    while(1){
        ssd1306_basic_clear();
        for (int i = 0; i < 8000000;i++) {__asm__("nop");}
        ssd1306_basic_string(0, 0, "Hello", 5, 1, SSD1306_FONT_16);
        for (int i = 0; i < 8000000;i++) {__asm__("nop");}
    }
    return 0;
}
