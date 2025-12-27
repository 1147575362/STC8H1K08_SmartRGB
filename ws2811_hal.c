#include "config.h"
#include <intrins.h>

uint8_t xdata PixelBuffer[LED_COUNT][3]; 

void WS2811_Reset(void) {
    LED_PIN = 0;
    // 300us 复位信号
    {
        uint8_t i = 4, j = 200;
        do { while (--j); } while (--i);
    }
}

void WS2811_SendByte(uint8_t dat) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        // 24MHz 下的时序微调
        if (dat & 0x80) { 
            // 1码: 高电平 ~800ns
            LED_PIN = 1;
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            LED_PIN = 0;
        } else { 
            // 0码: 高电平 ~300ns
            LED_PIN = 1;
            _nop_(); _nop_(); 
            LED_PIN = 0;
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); 
        }
        dat <<= 1;
    }
}

void WS2811_Show(void) {
    uint8_t i;
    EA = 0; 
    WS2811_Reset();
    for(i = 0; i < LED_COUNT; i++) {
        WS2811_SendByte(PixelBuffer[i][1]); // G
        WS2811_SendByte(PixelBuffer[i][0]); // R
        WS2811_SendByte(PixelBuffer[i][2]); // B
    }
    EA = 1;
}