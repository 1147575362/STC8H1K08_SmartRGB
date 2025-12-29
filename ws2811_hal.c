#include "config.h"
#include <intrins.h>

// 显存 buffer
uint8_t xdata PixelBuffer[LED_COUNT][3]; 

/**
 * @brief 发送复位信号
 * 修改点：增加延时循环次数，确保至少 300us 以上的低电平
 */
void WS2811_Reset(void) {
    LED_PIN = 0;
    // 延时加大到 > 300us
    {
        uint8_t i = 10, j = 200; // 这里的 i 从 4 改到了 10
        do { while (--j); } while (--i);
    }
}

/**
 * @brief 发送单字节
 * 针对 STC8G @ 24MHz 进行时序微调 (1个时钟周期 ≈ 41.6ns)
 * * 0码标准: 高 220ns~380ns (目标 ~300ns, 约7-8个周期)
 * 1码标准: 高 580ns~1000ns (目标 ~800ns, 约19-20个周期)
 */
void WS2811_SendByte(uint8_t dat) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (dat & 0x80) { 
            // -------------------------------------------
            // 发送 1 码 (长高电平)
            // -------------------------------------------
            LED_PIN = 1;
            // 保持高电平 ~800ns
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); // 再多加两个，确保够长
            
            LED_PIN = 0;
            // 这里的低电平时间不敏感，只要别太短就行
        } else { 
            // -------------------------------------------
            // 发送 0 码 (短高电平) -> 这里最容易出问题
            // -------------------------------------------
            LED_PIN = 1;
            
            // 原来只有2个nop，可能只有120ns，太短了
            // 现在增加到 4~5 个 nop，凑够 250ns+
            _nop_(); _nop_(); _nop_(); _nop_(); 
            
            LED_PIN = 0;
            
            // 补足周期长度
            _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 
            _nop_(); _nop_(); _nop_(); _nop_(); 
        }
        dat <<= 1;
    }
}

/**
 * @brief 刷新数据到灯带
 */
void WS2811_Show(void) {
    uint8_t i;
    EA = 0; // 关中断保护时序
    
    WS2811_Reset();
    
    for(i = 0; i < LED_COUNT; i++) {
        // HWS2811 常见顺序是 RGB 或 GRB，如果颜色反了改这里
        WS2811_SendByte(PixelBuffer[i][1]); // G
        WS2811_SendByte(PixelBuffer[i][2]); // R
        WS2811_SendByte(PixelBuffer[i][0]); // B
    }
    
    EA = 1; // 恢复中断
}