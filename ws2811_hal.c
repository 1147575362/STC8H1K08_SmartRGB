#include "config.h"
#include <intrins.h>

// 显存 buffer
uint8_t xdata PixelBuffer[LED_COUNT][3]; 

/**
 * @brief 发送复位信号
 * 修改：大幅增加复位时间 (>300us -> >600us)
 * 作用：让电源不稳的板子有更多时间喘息，复位更彻底
 */
void WS2811_Reset(void) {
    LED_PIN = 0;
    {
        // 延时加倍
        uint8_t i = 20, j = 200; 
        do { while (--j); } while (--i);
    }
}

/**
 * @brief 发送单字节 (抗干扰时序)
 * 策略：稍微牺牲一点刷新速度，换取极致的稳定性
 * 针对 24MHz 主频调整：
 * 0码: 高电平拉长到 ~400ns (原300ns)
 * 1码: 高电平拉长到 ~1000ns (原900ns)
 */
void WS2811_SendByte(uint8_t dat) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (dat & 0x80) { 
            // -------------------------------------------
            // 发送 1 码 (长高电平)
            // 目标: ~1000ns (约 24 个时钟周期)
            // -------------------------------------------
            LED_PIN = 1;
            // 暴力加宽，让信号更稳
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); // 加到20个nop
            
            LED_PIN = 0;
            // 低电平也要保持足够长，防止连码
            _nop_(); _nop_(); _nop_(); _nop_(); 
        } else { 
            // -------------------------------------------
            // 发送 0 码 (短高电平)
            // 目标: ~400ns (约 9~10 个时钟周期)
            // -------------------------------------------
            LED_PIN = 1;
            
            // 原来是4个nop，现在加到7个，防止被电容吞掉
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
            
            LED_PIN = 0;
            
            // 补足周期长度，低电平时间长一点没关系
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
        }
        dat <<= 1;
    }
}

void WS2811_Show(void) {
    uint8_t i;
    EA = 0; // 关中断
    
    WS2811_Reset();
    
    for(i = 0; i < LED_COUNT; i++) {
        WS2811_SendByte(PixelBuffer[i][1]); // G
        WS2811_SendByte(PixelBuffer[i][0]); // R
        WS2811_SendByte(PixelBuffer[i][2]); // B
    }
    
    EA = 1; // 开中断
}