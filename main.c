#include "config.h"

// ==========================================
// 外部接口引用
// ==========================================
extern void LED_System_Init(void);
extern void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
extern void LED_Task_Loop(uint8_t ticks_passed); 

// 系统绝对时间计数器
volatile uint32_t g_SysTick = 0;

void Timer0_Isr(void) interrupt 1 {
    g_SysTick++; 
}

// ==========================================
// 新增：Gamma 校正函数
// 作用：修正人眼对亮度的非线性感知，让红色看起来更宽、颜色更鲜艳
// 原理：y = x^2 / 256 (简化版 Gamma 2.0)
// ==========================================
uint8_t Gamma(uint8_t val) {
    // 使用 16位 乘法防止溢出，然后右移8位（除以256）
    return (uint8_t)(((uint16_t)val * val) >> 8);
}

// ==========================================
// 优化后的颜色轮
// ==========================================
void Wheel(uint8_t WheelPos, uint8_t* r, uint8_t* g, uint8_t* b) {
    uint8_t tr, tg, tb;
    
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
        tr = 255 - WheelPos * 3;
        tg = 0;
        tb = WheelPos * 3;
    } else if(WheelPos < 170) {
        WheelPos -= 85;
        tr = 0;
        tg = WheelPos * 3;
        tb = 255 - WheelPos * 3;
    } else {
        WheelPos -= 170;
        tr = WheelPos * 3;
        tg = 255 - WheelPos * 3;
        tb = 0;
    }
    
    // 关键修正：输出前经过 Gamma 校正
    // 这会让混合色（如黄、青、紫）变窄，纯色（红、绿、蓝）视觉范围变宽
    *r = Gamma(tr);
    *g = Gamma(tg);
    *b = Gamma(tb);
}

// ==========================================
// 核心逻辑：上电彩虹流光 -> 常亮白光
// ==========================================
void Auto_Effects(void) {
    static uint8_t state = 0;
    static uint32_t last_update_time = 0;
    static uint8_t rainbow_pos = 0;
    uint8_t i;
    uint8_t r, g, b;

    uint32_t now = g_SysTick;

    switch(state) {
        // --------------------------------------------------
        // 阶段 0: 酷炫彩虹流光 (10秒)
        // --------------------------------------------------
        case 0:
            // 20ms 刷新率 = 50FPS，流畅度很高
            // 此处将刷新率改为10ms，速度翻倍
            if (now - last_update_time >= 10) {
                last_update_time = now;
                rainbow_pos++; 

                for(i=0; i<LED_COUNT; i++) {
                    // i*25 决定了彩虹的“密度”，25适合10个像素点铺满一圈
                    Wheel((rainbow_pos + i * 25) & 0xFF, &r, &g, &b);
                    
                    // 亮度给 600，配合 Gamma 效果已经很艳丽了
                    SetLed(i, r, g, b, 600, 0); 
                }
            }

            // 15秒后切换
            if (now > 15000) {
                state = 1; 
            }
            break;

        // --------------------------------------------------
        // 阶段 1: 平滑过渡到白光
        // --------------------------------------------------
        case 1:
            // 2秒渐变到白光
            for(i=0; i<LED_COUNT; i++) {
                // 这里的亮度设为 800 (约80%)，防止60颗全开太烫
                // 如果电源够强且散热好，可以改回 1023
                SetLed(i, 255, 255, 255, 800, 2000);
            }
            state = 2; 
            break;

        // --------------------------------------------------
        // 阶段 2: 保持常亮
        // --------------------------------------------------
        case 2:
            break;
    }
}

void main() {
    static uint32_t last_anim_tick_time = 0;
    
    P_SW2 |= 0x80; 
    LED_System_Init();
    
    // Timer0: 1ms @ 24MHz
    AUXR |= 0x80; TMOD &= 0xF0; TL0 = 0x40; TH0 = 0xA2; 
    TR0 = 1; ET0 = 1; EA = 1;
    
    // 初始黑
    { uint8_t j; for(j=0; j<LED_COUNT; j++) SetLed(j, 0,0,0, 0, 0); }

    while(1) {
        uint32_t current_time = g_SysTick;
        
        // 任务调度 10ms
        if (current_time - last_anim_tick_time >= ANIMATION_TICK_MS) {
            uint32_t diff_ms = current_time - last_anim_tick_time;
            uint8_t ticks = diff_ms / ANIMATION_TICK_MS;
            last_anim_tick_time += (uint32_t)ticks * ANIMATION_TICK_MS;
            
            LED_Task_Loop(ticks); 
            Auto_Effects();
        }
    }
}