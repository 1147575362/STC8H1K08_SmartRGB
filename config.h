#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <STC8H.H>  
#include <intrins.h>

// ================= 类型定义 =================
typedef unsigned char   uint8_t;
typedef unsigned int    uint16_t;
typedef unsigned long   uint32_t;
typedef int             int16_t;
typedef long            int32_t;

// ================= 硬件定义 =================
// 1. 灯带信号脚修改为 P1.3
sbit LED_PIN = P1^3; 

// 2. 板载指示灯定义 (根据常见核心板，通常是 P1.0，如果不对请改这里)
// sbit ONBOARD_LED = P1^2;

// ================= 参数定义 =================
#define LED_COUNT       10         
#define SYS_CLOCK_HZ    24000000UL 

// 动画任务刷新周期 10ms
#define ANIMATION_TICK_MS  10 

#endif