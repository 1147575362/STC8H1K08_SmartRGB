#include "config.h"
#include <intrins.h>

// 显存定义：直接映射灯带的颜色顺序 (通常是 GRB)
// STC8H1K08 RAM较小，xdata可能会不够，这里使用 idata 或默认 (small/large model需注意)
// 建议在Keil Target选项中 Memory Model 选 "Large: variables in XDATA" 
// STC8H1K08 XDATA 有 1024 字节，足够放下。
uint8_t xdata PixelBuffer[LED_COUNT][3]; 

/**
 * @brief 发送复位信号 (>280us low)
 */
void WS2811_Reset(void) {
    LED_PIN = 0;
    // 延时约 300us
    // STC8 @ 24MHz, 1ms ~ 24000 cycles. 
    // 简单的软件延时
    {
        uint8_t i, j;
        i = 4;
        j = 200;
        do {
            while (--j);
        } while (--i);
    }
}

/**
 * @brief 发送单个字节 (8 bits)
 * 时序说明 (24MHz, 1 clock = 41.6ns):
 * T0H: 250ns ~ 300ns -> 6~7 clocks
 * T0L: 900ns -> 21 clocks
 * T1H: 800ns -> 19 clocks
 * T1L: 450ns -> 10 clocks
 * 注意：C语言调用和循环有开销，以下nop经过估算。
 */
void WS2811_SendByte(uint8_t dat) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (dat & 0x80) {
            // 发送 '1' 码: 高电平长，低电平短
            LED_PIN = 1;
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); // ~14 clocks high
            
            LED_PIN = 0;
            // Loop overhead handles the low part
        } else {
            // 发送 '0' 码: 高电平短，低电平长
            LED_PIN = 1;
            _nop_(); _nop_(); // ~2-3 clocks high
            
            LED_PIN = 0;
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); // Padding low
        }
        dat <<= 1;
    }
}

/**
 * @brief 刷新显存到灯带
 * 关闭中断以防止时序被打断
 */
void WS2811_Show(void) {
    uint8_t i;
    EA = 0; // 关中断，严禁打断时序
    
    WS2811_Reset();
    
    for(i = 0; i < LED_COUNT; i++) {
        // HWS2811 通常顺序是 G, R, B 或者 R, G, B
        // 这里假设是 GRB (最常见的 WS2812 顺序)，如果颜色不对，交换这里
        WS2811_SendByte(PixelBuffer[i][1]); // G
        WS2811_SendByte(PixelBuffer[i][0]); // R
        WS2811_SendByte(PixelBuffer[i][2]); // B
    }
    
    EA = 1; // 恢复中断
}