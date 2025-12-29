#include "config.h"

extern void LED_System_Init(void);
extern void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
extern void LED_Task_Loop(void);

volatile bit g_Tick1ms = 0;

void Timer0_Isr(void) interrupt 1 {
    g_Tick1ms = 1;
}

// ==========================================
// 任务1：LED1 呼吸/闪烁 (1s 周期)
// ==========================================
void Blink_Task(void) {
    static uint16_t blink_cnt = 0;
    // 10ms 调用一次
    blink_cnt++;
    if (blink_cnt >= 50) { // 50 * 10ms = 500ms 翻转一次，周期 1s
        blink_cnt = 0;
        ONBOARD_LED = !ONBOARD_LED; // 翻转状态
    }
}

// ==========================================
// 任务2：RGB 自动灯效 (重写版)
// ==========================================
void Auto_Effects(void) {
    static uint8_t state = 0;
    static uint16_t wait_ticks = 0; // 动态延时计数器
    uint8_t i;
    
    // 如果还在等待动画完成，就直接退出
    if (wait_ticks > 0) {
        wait_ticks--;
        return; 
    }

    // 状态机：一旦 wait_ticks 归零，立即执行下一个动作
    switch(state) {
        case 0: // 变红
            {
                uint16_t fade_time = 3000; // 设定渐变时间
                for(i=0; i<LED_COUNT; i++) SetLed(i, 255, 0, 0, 200, fade_time);
                
                // 关键点：让状态机正好等待 fade_time 这么久
                wait_ticks = fade_time / ANIMATION_TICK_MS; 
            }
            state = 1; 
            break;
            
        case 1: // 变绿
            {
                uint16_t fade_time = 3000; // 设定渐变时间
                for(i=0; i<LED_COUNT; i++) SetLed(i, 0, 255, 0, 200, fade_time);
                wait_ticks = fade_time / ANIMATION_TICK_MS;
            }
            state = 2; 
            break;
            
        case 2: // 变蓝
            {
                uint16_t fade_time = 3000; // 设定渐变时间
                for(i=0; i<LED_COUNT; i++) SetLed(i, 0, 0, 255, 200, fade_time);
                wait_ticks = fade_time / ANIMATION_TICK_MS;
            }
            state = 0; 
            break;
    }
}

void main() {
    P_SW2 |= 0x80; 
    
    LED_System_Init();
    
    // Timer0 配置: 1ms @ 24MHz
    AUXR |= 0x80; TMOD &= 0xF0;
    TL0 = 0x40; TH0 = 0xA2; 
    TR0 = 1; ET0 = 1; EA = 1;
    
    // 初始化全黑
    { uint8_t j; for(j=0; j<LED_COUNT; j++) SetLed(j, 0,0,0, 0, 0); }

    while(1) {
        if (g_Tick1ms) {
            g_Tick1ms = 0;
            
            // 使用 block 包含 static 变量
            {
                static uint8_t drive_div = 0; 
                if (++drive_div >= 10) { // 每 10ms 运行一次
                    drive_div = 0;
                    
                    LED_Task_Loop(); // 底层驱动
                    Auto_Effects();  // 应用逻辑
                    Blink_Task();    // LED1 闪烁
                }
            }
        }
    }
}