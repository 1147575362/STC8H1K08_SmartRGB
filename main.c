#include "config.h"
#include <STC8H.H>

// 声明外部函数
extern void LED_System_Init(void);
extern void SetLed_Interface(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);
extern void LED_Task_Loop(void);

// 全局 Tick 标志，由定时器中断置位
volatile bit g_TickFlag = 0;

// 定时器0中断服务程序 (10ms)
void TM0_Isr() interrupt 1 {
    g_TickFlag = 1;
}

// 简单的Demo：流水灯效果逻辑
void Demo_Effect_Run(void) {
    static uint16_t step_timer = 0;
    static uint8_t current_led = 0;
    
    // 每 200ms 触发一次逻辑
    step_timer++;
    if (step_timer >= 20) { // 20 * 10ms = 200ms
        step_timer = 0;
        
        // 效果：点亮当前灯为红色，并在500ms内渐变为蓝色，前一个灯熄灭
        
        // 1. 设置当前灯：红色 (R=255, G=0, B=0), 亮度500, 渐变时间 500ms
        SetLed_Interface(current_led, 255, 0, 0, 500, 500);
        
        // 2. 熄灭前一个灯 (索引环绕)
        {
            uint8_t prev = (current_led == 0) ? (LED_COUNT - 1) : (current_led - 1);
            SetLed_Interface(prev, 0, 0, 0, 0, 500); // 500ms 渐变熄灭
        }
        
        // 移动索引
        current_led++;
        if (current_led >= LED_COUNT) current_led = 0;
    }
}

void main() {
    // 1. 硬件初始化
    // 必须配置 STC8H 的时钟为 24MHz
    P_SW2 |= 0x80; // 允许访问扩展寄存器
    HIRCCR = 0x30; // 开启内部高频振荡器 (24MHz 需要查看手册具体的寄存器配置值，这里假设默认或已通过ISP烧录配置)
    // 建议：在STC-ISP烧录软件中直接设置“IRC频率”为 24.000MHz
    
    LED_System_Init();
    
    // 2. 配置定时器0 (16位自动重装载, 10ms @ 24MHz)
    // 24,000,000 / 12 / 100 = 20000 cycles (12T mode) or similar
    // STC8H default is 1T. 24MHz, 10ms = 240,000 clocks.
    // 65536 - 240000 溢出，需要分频或用 T0 做 1ms 再计数
    // 这里简化配置：假设 Timer0 1T模式，需要用较短时间，比如 1ms 中断一次，我们在 loop 里计数 10 次
    AUXR |= 0x80;    // Timer0 1T mode
    TMOD &= 0xF0;    // Timer0 16-bit auto-reload
    TL0 = 0x40;      // 65536 - 24000 = 41536 = 0xA240 (1ms)
    TH0 = 0xA2;
    TR0 = 1;         // Start Timer0
    ET0 = 1;         // Enable Timer0 interrupt
    EA = 1;          // Global Interrupt
    
    // 3. 上电初始效果：全部设为暗白色
    {
        uint8_t i;
        for(i=0; i<LED_COUNT; i++) {
            SetLed_Interface(i, 255, 255, 255, 100, 2000); // 2秒渐变亮起
        }
    }
    
    while(1) {
        // 主循环等待 Tick (1ms一次)
        if (g_TickFlag) {
            g_TickFlag = 0;
            
            // 降频处理：LED 动画任务每 10ms 跑一次
            static uint8_t divider = 0;
            divider++;
            if (divider >= 10) {
                divider = 0;
                
                // 运行底层驱动逻辑 (处理渐变)
                LED_Task_Loop();
                
                // 运行应用层灯效逻辑
                Demo_Effect_Run();
            }
        }
    }
}