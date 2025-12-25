#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <STC8H.H>  // 请确保你的Keil里有STC8的头文件，如果没有，可以用 reg51.h 替代但需手动定义特殊寄存器

// ================= 硬件定义 =================
// LED控制引脚定义 (根据你的原理图修改这里)
sbit LED_PIN = P1^2; 

// ================= 参数定义 =================
#define LED_COUNT       66      // LED总数量
#define SYS_CLOCK_HZ    24000000UL // 系统时钟 24MHz

// 渐变任务的刷新周期 (ms)
// 越小动画越平滑，但CPU占用越高。10ms是比较好的平衡点。
#define ANIMATION_TICK_MS  10 

#endif