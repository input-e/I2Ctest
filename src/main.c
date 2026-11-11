#include "main.h"


int main(void){
    RCCconf();
    ssdset();
    
    while(true){
        ssd1306_draw_pixel(&Display, 5, 5, 1);
        ssd1306_display(&Display);
        for (int i = 0; i < 48000000;i++) {asm("__nop__");}
        ssd1306_draw_pixel(&Display, 5, 5, 1);
        ssd1306_display(&Display);
        for (int i = 0; i < 48000000;i++) {asm("__nop__");}
    }
    return 0;
}
