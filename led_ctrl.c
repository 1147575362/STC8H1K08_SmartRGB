#include "config.h"
#include <stdlib.h>

// 引用底层buffer
extern uint8_t xdata PixelBuffer[LED_COUNT][3];

// 目标颜色缓存 (用于渐变)
uint8_t xdata TargetBuffer[LED_COUNT][3];

// 渐变倒计时 (单位: 动画帧数)
// 如果 fade_ms = 1000ms, TICK = 10ms, 则 count = 100
uint16_t xdata FadeCounters[LED_COUNT];

/**
 * @brief 系统初始化
 */
void LED_System_Init(void) {
    uint8_t i;
    
    // 配置GPIO模式 (STC8需要配置 P1M0/P1M1)
    // 假设 P1.2 是推挽输出
    P1M0 |= 0x04; // 0000 0100
    P1M1 &= ~0x04;
    
    // 清空显存
    for(i=0; i<LED_COUNT; i++) {
        PixelBuffer[i][0] = 0; PixelBuffer[i][1] = 0; PixelBuffer[i][2] = 0;
        TargetBuffer[i][0] = 0; TargetBuffer[i][1] = 0; TargetBuffer[i][2] = 0;
        FadeCounters[i] = 0;
    }
    
    // 初始化定时器0 用于系统滴答 (10ms)
    // 这里省略具体寄存器配置，在 main.c 中统一做
}

/**
 * @brief 用户接口：设置LED
 * 亮度处理：在设置目标值时直接应用亮度缩放，节省运行时计算
 */
void SetLed_Interface(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms) {
    uint32_t tempR, tempG, tempB;
    
    if (index >= LED_COUNT) return;
    
    // 1. 应用亮度 (0-1023) -> 映射到 (0-255)
    // 公式: Val = (Color * Brightness) / 1023
    // 使用位移加速除法: /1024 约等于 >> 10
    tempR = ((uint32_t)r * brightness) >> 10;
    tempG = ((uint32_t)g * brightness) >> 10;
    tempB = ((uint32_t)b * brightness) >> 10;
    
    // 钳位防止溢出
    if (tempR > 255) tempR = 255;
    if (tempG > 255) tempG = 255;
    if (tempB > 255) tempB = 255;
    
    // 2. 设置目标
    TargetBuffer[index][0] = (uint8_t)tempR;
    TargetBuffer[index][1] = (uint8_t)tempG;
    TargetBuffer[index][2] = (uint8_t)tempB;
    
    // 3. 设置渐变时间
    if (fade_ms == 0) {
        // 立即生效
        PixelBuffer[index][0] = TargetBuffer[index][0];
        PixelBuffer[index][1] = TargetBuffer[index][1];
        PixelBuffer[index][2] = TargetBuffer[index][2];
        FadeCounters[index] = 0;
    } else {
        // 转换为Ticks
        FadeCounters[index] = fade_ms / ANIMATION_TICK_MS;
        if (FadeCounters[index] == 0) FadeCounters[index] = 1; // 至少1帧
    }
}

/**
 * @brief 辅助函数：向目标逼近一步
 * 算法：Ease-Out 逼近。
 * 每次移动距离 = (目标 - 当前) / 剩余帧数
 * 这样可以保证在指定时间内到达，且不需要存储浮点步长。
 */
uint8_t MoveTowards(uint8_t current, uint8_t target, uint16_t steps_left) {
    int16_t diff = (int16_t)target - (int16_t)current;
    int16_t step;
    
    if (steps_left == 0) return target;
    if (diff == 0) return target;
    
    // 计算本帧步长
    step = diff / (int16_t)steps_left;
    
    // 如果差距很小，除法结果为0，必须强制移动至少1
    if (step == 0) {
        if (diff > 0) step = 1;
        else step = -1;
    }
    
    return (uint8_t)(current + step);
}

/**
 * @brief 核心任务循环 (需在主循环中不断调用)
 * 处理所有LED的渐变逻辑，并刷新硬件
 */
void LED_Task_Loop(void) {
    static uint8_t is_dirty = 0; // 标记是否有数据变化
    uint8_t i;
    
    // 这里需要一个非阻塞的时间控制
    // 假设外部有一个 volatile uint32_t sys_tick_ms; 
    // 或者简单一点，每次调用此函数如果检测到Tick标志位才运行
    // 为了简化，我们假设 main 循环里控制了频率，或者这里只是纯逻辑
    
    is_dirty = 0;
    
    for (i = 0; i < LED_COUNT; i++) {
        if (FadeCounters[i] > 0) {
            // 需要渐变
            is_dirty = 1;
            
            // 计算 R, G, B 的新值
            PixelBuffer[i][0] = MoveTowards(PixelBuffer[i][0], TargetBuffer[i][0], FadeCounters[i]);
            PixelBuffer[i][1] = MoveTowards(PixelBuffer[i][1], TargetBuffer[i][1], FadeCounters[i]);
            PixelBuffer[i][2] = MoveTowards(PixelBuffer[i][2], TargetBuffer[i][2], FadeCounters[i]);
            
            FadeCounters[i]--;
        } 
        // 额外的校验：如果时间到了，强制对齐（消除计算误差）
        else if (PixelBuffer[i][0] != TargetBuffer[i][0] || 
                 PixelBuffer[i][1] != TargetBuffer[i][1] || 
                 PixelBuffer[i][2] != TargetBuffer[i][2]) {
            PixelBuffer[i][0] = TargetBuffer[i][0];
            PixelBuffer[i][1] = TargetBuffer[i][1];
            PixelBuffer[i][2] = TargetBuffer[i][2];
            is_dirty = 1;
        }
    }
    
    // 只有当颜色发生变化时才刷新灯带，节省CPU
    if (is_dirty) {
        WS2811_Show(); 
    }
}