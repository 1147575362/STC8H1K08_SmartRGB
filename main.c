#include "config.h"

// 引用外部函数
extern void LED_System_Init(void);
// 这里的形参名字可以省略，避免编译器检查差异
extern void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
extern void LED_Task_Loop(void);

volatile bit g_Tick1ms = 0;

void Timer0_Isr(void) interrupt 1 {
    g_Tick1ms = 1;
}

void Auto_Effects(void) {
    static uint16_t time_cnt = 0;
    static uint8_t state = 0;
    uint8_t i; // <--- 关键修改：变量必须在 switch 或 if 之前定义
    
    time_cnt++;
    if (time_cnt < 100) return; // 1秒
    time_cnt = 0;

    switch(state) {
        case 0: 
            // 错误写法：for(uint8_t i=0...)
            // 正确写法：使用上面定义的 i
            for(i=0; i<LED_COUNT; i++) {
                SetLed(i, 255, 0, 0, 200, 1000); 
            }
            state = 1; 
            break;
        case 1: 
            for(i=0; i<LED_COUNT; i++) {
                SetLed(i, 0, 255, 0, 200, 1000); 
            }
            state = 2; 
            break;
        case 2: 
            for(i=0; i<LED_COUNT; i++) {
                SetLed(i, 0, 0, 255, 200, 1000); 
            }
            state = 0; 
            break;
    }
}

void main() {
    P_SW2 |= 0x80; 
    
    LED_System_Init();
    
    // Timer0 配置: 1ms @ 24MHz
    AUXR |= 0x80; 
    TMOD &= 0xF0;
    TL0 = 0x40; TH0 = 0xA2; 
    TR0 = 1; ET0 = 1; EA = 1;
    
    // 初始化熄灭
    { 
        uint8_t j; // 变量定义在块的最开始
        for(j=0; j<LED_COUNT; j++) SetLed(j, 0,0,0, 0, 0); 
    }

    while(1) {
        if (g_Tick1ms) {
            g_Tick1ms = 0;
            {
                // C51中，如果要在 block 里定义变量，必须加括号或放最前
                static uint8_t drive_div = 0; 
                if (++drive_div >= 10) {
                    drive_div = 0;
                    LED_Task_Loop(); 
                    Auto_Effects();
                }
            }
        }
    }
}