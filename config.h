#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <STC8G.H>  
#include <intrins.h>

// ==========================================
// 修复数据类型定义
// ==========================================
typedef unsigned char   uint8_t;
typedef unsigned int    uint16_t;
typedef unsigned long   uint32_t;
typedef int             int16_t;  // <--- 新增这一行，解决 undefined identifier 报错

// ================= 硬件定义 =================
// 根据你的实际接线修改，推荐 P3.2 或 P3.3
sbit LED_PIN = P3^2; 

// ================= 参数定义 =================
#define LED_COUNT       66         
#define SYS_CLOCK_HZ    24000000UL 
#define ANIMATION_TICK_MS  10 

#endif