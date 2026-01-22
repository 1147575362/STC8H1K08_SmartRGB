#include "config.h"

// ==========================================
// 外部接口引用
// ==========================================
extern void LED_System_Init(void);
extern void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
extern void LED_Task_Loop(uint8_t ticks_passed); 
// 新增：引用底层发送函数，用于上电立即灭灯
extern void WS2811_Show(void);

// 系统绝对时间计数器
volatile uint32_t g_SysTick = 0;

void Timer0_Isr(void) interrupt 1 {
    g_SysTick++; 
}

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
// 核心逻辑：彩虹 -> 白光 -> 呼吸(亮到暗) -> 循环
// ==========================================
void Auto_Effects(void) {
    static uint8_t state = 0;
    static uint32_t last_update_time = 0;
    static uint32_t state_start_time = 0; 
    static uint8_t rainbow_pos = 0;
    // 新增：强制首帧标志位
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
            // 逻辑优化：如果是第一次运行，或者时间到了 15ms
            if (first_run || (now - last_update_time >= 15)) {
                last_update_time = now;
                first_run = 0; // 清除首帧标志
                
                rainbow_pos++; 

                for(i=0; i<LED_COUNT; i++) {
                    Wheel((rainbow_pos + i * 25) & 0xFF, &r, &g, &b);
                    SetLed(i, r, g, b, 300, 0); 
                }
            }

            // 10秒后切换
            if (now - state_start_time > 10000) {
                state = 1; 
                state_start_time = now;
                
                // 预设渐变到白光
                for(i=0; i<LED_COUNT; i++) {
                    SetLed(i, 255, 255, 255, 300, 1500);
                }
            }
            break;

        // --------------------------------------------------
        // 阶段 1: 白光常亮 (5秒)
        // --------------------------------------------------
        case 1:
            // 等待 5秒
            if (now - state_start_time > 5000) {
                state = 2;
                state_start_time = now;
            }
            break;

        // --------------------------------------------------
        // 阶段 2: 白光呼吸 (5秒)
        // 优化：从亮(300) -> 暗(30) -> 亮(300)
        // --------------------------------------------------
        case 2:
            if (now - last_update_time >= 10) {
                // 变量声明必须在代码块顶部 (C89标准)
                uint32_t time_in_cycle;
                uint16_t pwm_val;

                last_update_time = now;
                
                // 周期 1000ms
                time_in_cycle = (now - state_start_time) % 1000;

                // 【修改点】：前500ms 变暗，后500ms 变亮
                if (time_in_cycle < 500) {
                    // 0~500ms: 亮度 300 -> 30 (变暗)
                    // 公式：300 - (当前时间 * 差值 / 总时间)
                    pwm_val = 300 - (uint32_t)time_in_cycle * 270 / 500;
                } else {
                    // 500~1000ms: 亮度 30 -> 300 (变亮)
                    pwm_val = 30 + (uint32_t)(time_in_cycle - 500) * 270 / 500;
                }

                for(i=0; i<LED_COUNT; i++) {
                    SetLed(i, 255, 255, 255, pwm_val, 0);
                }
            }

            // 5秒后循环
            if (now - state_start_time > 5000) {
                state = 0; 
                state_start_time = now;
                first_run = 1; // 重置标志位，确保下一次彩虹立马开始
            }
            break;
    }
}

void main() {
    static uint32_t last_anim_tick_time = 0;
    
    // 1. 硬件配置
    P_SW2 |= 0x80; 
    LED_System_Init();
    
    // ===============================================
    // 【关键修复 1】：上电立即强制全黑
    // 防止出现 0.5s 的白光闪烁
    // ===============================================
    { 
        uint8_t j; 
        for(j=0; j<LED_COUNT; j++) SetLed(j, 0,0,0, 0, 0); 
    }
    WS2811_Show(); // 立即发送！不等定时器！
    
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