#include "config.h"

extern uint8_t xdata PixelBuffer[LED_COUNT][3];
uint8_t xdata TargetBuffer[LED_COUNT][3];
uint16_t xdata FadeCounters[LED_COUNT];

void LED_System_Init(void) {
    uint8_t i;
    
    // STC8G P3.2 推挽输出配置
    P3M0 |= 0x04; 
    P3M1 &= ~0x04;
    
    for(i=0; i<LED_COUNT; i++) {
        PixelBuffer[i][0]=0; PixelBuffer[i][1]=0; PixelBuffer[i][2]=0;
        TargetBuffer[i][0]=0; TargetBuffer[i][1]=0; TargetBuffer[i][2]=0;
        FadeCounters[i] = 0;
    }
}

void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms) {
    uint32_t tr, tg, tb;
    
    if (index >= LED_COUNT) return;
    
    tr = ((uint32_t)r * brightness) >> 10;
    tg = ((uint32_t)g * brightness) >> 10;
    tb = ((uint32_t)b * brightness) >> 10;
    
    if (tr > 255) tr = 255;
    if (tg > 255) tg = 255;
    if (tb > 255) tb = 255;
    
    TargetBuffer[index][0] = (uint8_t)tr;
    TargetBuffer[index][1] = (uint8_t)tg;
    TargetBuffer[index][2] = (uint8_t)tb;
    
    if (fade_ms == 0) {
        PixelBuffer[index][0] = TargetBuffer[index][0];
        PixelBuffer[index][1] = TargetBuffer[index][1];
        PixelBuffer[index][2] = TargetBuffer[index][2];
        FadeCounters[index] = 0;
    } else {
        FadeCounters[index] = fade_ms / ANIMATION_TICK_MS;
        if (FadeCounters[index] == 0) FadeCounters[index] = 1; 
    }
}

// 修复后的 Approach 函数：变量定义在顶部
uint8_t Approach(uint8_t current, uint8_t target, uint16_t steps) {
    int16_t diff;      // 变量声明前置
    int16_t step_val;  // 变量声明前置

    // 强转为 int16_t 以计算负数差值
    diff = (int16_t)target - current;
    
    if (steps == 0 || diff == 0) return target;
    
    step_val = diff / (int16_t)steps;
    
    // 如果步进太小被整除为0，强制移动1步
    if (step_val == 0) {
        if (diff > 0) step_val = 1;
        else step_val = -1;
    }
    
    return (uint8_t)(current + step_val);
}

void LED_Task_Loop(void) {
    extern void WS2811_Show(void);
    uint8_t i;
    bit need_update = 0;
    
    for(i=0; i<LED_COUNT; i++) {
        if (FadeCounters[i] > 0) {
            need_update = 1;
            PixelBuffer[i][0] = Approach(PixelBuffer[i][0], TargetBuffer[i][0], FadeCounters[i]);
            PixelBuffer[i][1] = Approach(PixelBuffer[i][1], TargetBuffer[i][1], FadeCounters[i]);
            PixelBuffer[i][2] = Approach(PixelBuffer[i][2], TargetBuffer[i][2], FadeCounters[i]);
            FadeCounters[i]--;
        } else {
            // 确保最终颜色一致
            if (PixelBuffer[i][0] != TargetBuffer[i][0]) { PixelBuffer[i][0] = TargetBuffer[i][0]; need_update = 1; }
            if (PixelBuffer[i][1] != TargetBuffer[i][1]) { PixelBuffer[i][1] = TargetBuffer[i][1]; need_update = 1; }
            if (PixelBuffer[i][2] != TargetBuffer[i][2]) { PixelBuffer[i][2] = TargetBuffer[i][2]; need_update = 1; }
        }
    }
    
    if (need_update) WS2811_Show();
}