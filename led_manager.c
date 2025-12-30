#include "config.h"

extern uint8_t xdata PixelBuffer[LED_COUNT][3];
uint8_t xdata StartBuffer[LED_COUNT][3];
uint8_t xdata TargetBuffer[LED_COUNT][3];
uint16_t xdata CurrentTicks[LED_COUNT];
uint16_t xdata TotalTicks[LED_COUNT];

// 新增：脏标记，用于记录是否有新的 SetLed 操作
static bit s_IsDirty = 0;

void LED_System_Init(void) {
    uint8_t i;
    P1M0 |= 0x09; P1M1 &= ~0x09; 
    
    for(i=0; i<LED_COUNT; i++) {
        PixelBuffer[i][0]=0; PixelBuffer[i][1]=0; PixelBuffer[i][2]=0;
        StartBuffer[i][0]=0; StartBuffer[i][1]=0; StartBuffer[i][2]=0;
        TargetBuffer[i][0]=0; TargetBuffer[i][1]=0; TargetBuffer[i][2]=0;
        CurrentTicks[i] = 0; TotalTicks[i] = 0;
    }
    s_IsDirty = 0;
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
    
    StartBuffer[index][0] = PixelBuffer[index][0];
    StartBuffer[index][1] = PixelBuffer[index][1];
    StartBuffer[index][2] = PixelBuffer[index][2];
    
    TargetBuffer[index][0] = (uint8_t)tr;
    TargetBuffer[index][1] = (uint8_t)tg;
    TargetBuffer[index][2] = (uint8_t)tb;
    
    if (fade_ms == 0) {
        PixelBuffer[index][0] = TargetBuffer[index][0];
        PixelBuffer[index][1] = TargetBuffer[index][1];
        PixelBuffer[index][2] = TargetBuffer[index][2];
        TotalTicks[index] = 0;
        CurrentTicks[index] = 0;
    } else {
        TotalTicks[index] = fade_ms / ANIMATION_TICK_MS;
        if (TotalTicks[index] == 0) TotalTicks[index] = 1;
        CurrentTicks[index] = 0;
    }
    
    // 关键修正：标记数据已变动
    s_IsDirty = 1;
}

uint8_t Interpolate(uint8_t start, uint8_t target, uint16_t current_tick, uint16_t total_tick) {
    int32_t val;
    int16_t diff;
    if (current_tick >= total_tick) return target;
    diff = (int16_t)target - (int16_t)start;
    val = (int32_t)start + ((int32_t)diff * current_tick) / total_tick;
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (uint8_t)val;
}

// 核心修改：增加 ticks_passed 参数
void LED_Task_Loop(uint8_t ticks_passed) {
    extern void WS2811_Show(void);
    uint8_t i;
    bit need_update = 0;
    
    // 如果没有时间流逝且没有新的设置指令，直接返回
    if (ticks_passed == 0 && s_IsDirty == 0) return;

    for(i=0; i<LED_COUNT; i++) {
        if (CurrentTicks[i] < TotalTicks[i]) {
            need_update = 1;
            // 关键：一次性加上过去的所有时间，实现“追帧”
            CurrentTicks[i] += ticks_passed; 
    // 防止超调
            if (CurrentTicks[i] > TotalTicks[i]) CurrentTicks[i] = TotalTicks[i];
            
            PixelBuffer[i][0] = Interpolate(StartBuffer[i][0], TargetBuffer[i][0], CurrentTicks[i], TotalTicks[i]);
            PixelBuffer[i][1] = Interpolate(StartBuffer[i][1], TargetBuffer[i][1], CurrentTicks[i], TotalTicks[i]);
            PixelBuffer[i][2] = Interpolate(StartBuffer[i][2], TargetBuffer[i][2], CurrentTicks[i], TotalTicks[i]);
        } else if (TotalTicks[i] > 0) {
            // 结束时强制对齐
            if (PixelBuffer[i][0] != TargetBuffer[i][0] || PixelBuffer[i][1] != TargetBuffer[i][1] || PixelBuffer[i][2] != TargetBuffer[i][2]) {
                PixelBuffer[i][0] = TargetBuffer[i][0];
                PixelBuffer[i][1] = TargetBuffer[i][1];
                PixelBuffer[i][2] = TargetBuffer[i][2];
                need_update = 1;
            }
            TotalTicks[i] = 0;
        }
    }
    
    // 关键修正：如果有渐变需要更新，或者刚才 SetLed 修改了数据，都刷新
    if (need_update || s_IsDirty) {
        WS2811_Show();
        s_IsDirty = 0; // 清除标记
    }
}