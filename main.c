#include "config.h"

extern void LED_System_Init(void);
extern void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
// 注意：这里函数原型变了，加了参数
extern void LED_Task_Loop(uint8_t ticks_passed); 

// 系统绝对时间计数器 (1ms 累加)
volatile uint32_t g_SysTick = 0;

void Timer0_Isr(void) interrupt 1 {
    g_SysTick++; // 无论主循环多卡，中断里的计数永远是准的
}

void Blink_Task(void) {
    static uint32_t last_blink_time = 0;
    // 使用差值判断，防止溢出问题
    if (g_SysTick - last_blink_time >= 500) { // 500ms
        last_blink_time = g_SysTick;
        ONBOARD_LED = !ONBOARD_LED;
    }
}

void Auto_Effects(void) {
    static uint8_t state = 0;
    static uint32_t state_start_time = 0; // 记录进入状态的时间点
    static uint32_t wait_ms = 0;
    uint8_t i;
    
    // 检查是否到达预定时间
    if (g_SysTick - state_start_time < wait_ms) {
        return; 
    }

    // 时间到，切换状态
    switch(state) {
        case 0: 
            wait_ms = 1000; // 3秒渐变
            for(i=0; i<LED_COUNT; i++) SetLed(i, 0, 255, 0, 500, wait_ms);
            
            state_start_time = g_SysTick; // 记录当前时间
            state = 1; 
            break;
            
        case 1: 
            wait_ms = 1000;
            for(i=0; i<LED_COUNT; i++) SetLed(i, 0, 0, 0, 0, wait_ms);
            
            state_start_time = g_SysTick;
            state = 2; 
            break;
            
        case 2: 
            wait_ms = 500;
            for(i=0; i<15; i++) SetLed(i, 0, 0, 255, 200, wait_ms);
            
            state_start_time = g_SysTick;
            state = (i = 15 ? 0 : 2); 
            break;
    }
}

void main() {
    static uint32_t last_anim_time = 0;
    
    P_SW2 |= 0x80; 
    LED_System_Init();
    
    AUXR |= 0x80; TMOD &= 0xF0; TL0 = 0x40; TH0 = 0xA2; 
    TR0 = 1; ET0 = 1; EA = 1;
    
    { uint8_t j; for(j=0; j<LED_COUNT; j++) SetLed(j, 0,0,0, 0, 0); }

    while(1) {
        // ====================================================
        // 核心修正：基于绝对时间差的任务调度
        // ====================================================
        uint32_t current_time = g_SysTick;
        
        // 只有当时间差 >= 10ms 时才运行一帧
        if (current_time - last_anim_time >= ANIMATION_TICK_MS) {
            
            // 计算具体过去了几个 10ms (Ticks)
            // 如果系统卡顿了，这个值可能是 2, 3 甚至更大
            uint32_t diff_ms = current_time - last_anim_time;
            uint8_t ticks = diff_ms / ANIMATION_TICK_MS;
            
            // 更新“上一次运行时间”，注意要对齐到 10ms 的倍数，防止累计误差
            last_anim_time += (uint32_t)ticks * ANIMATION_TICK_MS;
            
            // 运行任务，并告诉它“你落后了 ticks 帧，请直接跳过去”
            LED_Task_Loop(ticks); 
            
            // 运行上层逻辑
            Auto_Effects();
            Blink_Task();
        }
    }
}