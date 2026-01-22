#include "config.h"

// ==========================================
// 外部接口引用
// ==========================================
extern void LED_System_Init(void);
extern void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
extern void LED_Task_Loop(uint8_t ticks_passed); 
extern void WS2811_Show(void);

// 系统绝对时间计数器
volatile uint32_t g_SysTick = 0;

void Timer0_Isr(void) interrupt 1 {
    g_SysTick++; 
}

// ---------------------------------------------------------
// 简易软件延时 (用于上电初期，此时定时器未启动)
// ---------------------------------------------------------
// void Delay_Soft_Ms(uint16_t ms) {
//     uint16_t i, j;
//     for(i=0; i<ms; i++) {
//         // 24MHz主频下，循环大概调整到1ms
//         for(j=0; j<2000; j++) { _nop_(); _nop_(); _nop_(); _nop_(); }
//     }
// }

// ==========================================
// Gamma 校正
// ==========================================
uint8_t Gamma(uint8_t val) {
    return (uint8_t)(((uint16_t)val * val) >> 8);
}

// ==========================================
// 颜色轮 (柔和版)
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
    
    // 降低饱和度 (柔和粉彩风)
    tr = (tr >> 1) + 50; 
    tg = (tg >> 1) + 50;
    tb = (tb >> 1) + 50;

    *r = Gamma(tr);
    *g = Gamma(tg);
    *b = Gamma(tb);
}

// ==========================================
// 核心逻辑：彩虹 -> 白光 -> 呼吸 -> (渐变) -> 循环
// ==========================================
void Auto_Effects(void) {
    static uint8_t state = 0;
    static uint32_t last_update_time = 0;
    static uint32_t state_start_time = 0; 
    static uint8_t rainbow_pos = 0;
    static bit first_run = 1;
    
    uint8_t i;
    uint8_t r, g, b;
    uint32_t now = g_SysTick;
    
    // 状态机
    switch(state) {
        // --------------------------------------------------
        // 阶段 0: 柔和彩虹流水 (10秒)
        // --------------------------------------------------
        case 0:
            if (first_run || (now - last_update_time >= 15)) {
                last_update_time = now;
                first_run = 0; 
                
                rainbow_pos++; 

                for(i=0; i<LED_COUNT; i++) {
                    Wheel((rainbow_pos + i * 25) & 0xFF, &r, &g, &b);
                    SetLed(i, r, g, b, 400, 0); 
                }
            }

            // 10秒后切换到白光常亮
            if (now - state_start_time > 10000) {
                state = 1; 
                state_start_time = now;
                
                // 1.5秒渐变到白光
                for(i=0; i<LED_COUNT; i++) {
                    SetLed(i, 255, 255, 255, 300, 1500);
                }
            }
            break;

        // --------------------------------------------------
        // 阶段 1: 白光常亮 (5秒)
        // --------------------------------------------------
        case 1:
            if (now - state_start_time > 5000) {
                state = 2;
                state_start_time = now;
            }
            break;

        // --------------------------------------------------
        // 阶段 2: 白光呼吸 (5秒)
        // 修正点2：亮度减半 (Max 150, Min 15)
        // --------------------------------------------------
        case 2:
            if (now - last_update_time >= 10) {
                uint32_t time_in_cycle;
                uint16_t pwm_val;

                last_update_time = now;
                
                // 周期 2000ms
                time_in_cycle = (now - state_start_time) % 2000;

                // 亮度范围改为: 15 ~ 300 (差值 285)
                if (time_in_cycle < 1000) {
                    // 0~500ms: 变暗 (300 -> 15)
                    pwm_val = 300 - (uint32_t)time_in_cycle * 285 / 1000;
                } else {
                    // 500~1000ms: 变亮 (15 -> 300)
                    pwm_val = 15 + (uint32_t)(time_in_cycle - 1000) * 285 / 1000;
                }

                for(i=0; i<LED_COUNT; i++) {
                    SetLed(i, 255, 255, 255, pwm_val, 0);
                }
            }

            // 5秒后，准备切回彩虹
            if (now - state_start_time > 5000) {
                // 修正点3：不直接跳 case 0，而是去 case 3 做过渡
                state = 3; 
                state_start_time = now;

                // 提前计算彩虹模式的第0帧颜色，并在 500ms 内渐变过去
                // 注意：目标亮度恢复为 300 (匹配彩虹模式亮度)
                for(i=0; i<LED_COUNT; i++) {
                    Wheel((0 + i * 25) & 0xFF, &r, &g, &b);
                    SetLed(i, r, g, b, 300, 500); // 500ms 渐变
                }
            }
            break;

        // --------------------------------------------------
        // 阶段 3: 过渡等待期 (0.5秒)
        // --------------------------------------------------
        case 3:
            // 等待 SetLed 的 500ms 渐变动画播放完
            if (now - state_start_time > 500) {
                state = 0; 
                state_start_time = now;
                
                // 重置彩虹参数，确保衔接自然
                first_run = 1; 
                rainbow_pos = 0; 
            }
            break;
    }
}

void main() {
    static uint32_t last_anim_tick_time = 0;
    
    // 1. 硬件配置
    P_SW2 |= 0x80; 
    LED_PIN = 0; // 确保IO口初始为低
    LED_System_Init();
    
    // ===============================================
    // 【关键修复 1】：上电强延时 + 强制全黑
    // LED驱动芯片上电需要时间复位，必须先延时！
    // ===============================================
    // Delay_Soft_Ms(200); // 这里的延时非常重要！
    
    // { 
    //     uint8_t j; 
    //     for(j=0; j<LED_COUNT; j++) SetLed(j, 0,0,0, 0, 0); 
    // }
    // WS2811_Show(); 
    // Delay_Soft_Ms(10); // 发完数据再稳一下
    
    // 2. 定时器配置 (1ms @ 24MHz)
    AUXR |= 0x80; TMOD &= 0xF0; TL0 = 0x40; TH0 = 0xA2; 
    TR0 = 1; ET0 = 1; EA = 1;
    
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