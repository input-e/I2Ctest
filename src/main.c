#include "main.h"


int main(void){
    RCCconf();
    ssdset();
    
    while(true){
        ssd1306_draw_pixel(&Display, 5, 5, 1);
        ssd1306_display(&Display);
        for (int i = 0; i < 8000000;i++) {__asm__("nop");}
        ssd1306_draw_pixel(&Display, 5, 5, 0);
        ssd1306_display(&Display);
        for (int i = 0; i < 8000000;i++) {__asm__("nop");}
    }
    return 0;
}
