# SmartRGB Controller (STC8 Series)

## 项目简介
本项目是一个基于 STC8 系列单片机（STC8G/STC8H）的智能 LED 灯带控制器。
目前主要驱动 **66 颗 SM16703P/WS2811** 协议的 RGB 灯珠，实现了基于时间插值的平滑色彩渐变算法。

## 硬件规格

| 项目             | 参数                   | 备注                                |
| :--------------- | :--------------------- | :---------------------------------- |
| **MCU**          | STC8G1K08 / STC8H1K08  | 目前测试用 STC8G，最终目标 STC8H    |
| **封装**         | TSSOP20 / SOP8         |                                     |
| **主频**         | **24MHz**              | 必须精确匹配，影响驱动时序          |
| **LED 驱动芯片** | SM16703P (兼容 WS2811) | 归零码协议                          |
| **LED 数量**     | 66 颗                  | 显存占用约 200 Bytes                |
| **RAM 占用**     | ~910 Bytes (XDATA)     | 芯片总容量 1024 Bytes，**资源紧张** |

### 引脚定义 (可在 `config.h` 中修改)
* **LED 数据信号 (DI)**: `P1.3` (推挽输出)
* **板载指示灯**: `P1.0` (推挽输出)
* **调试/下载**: `P3.0 (RX)`, `P3.1 (TX)`

## 软件架构

### 文件说明
* `main.c`: 主程序入口，包含系统时钟计数、任务调度器 (Task Scheduler) 和顶层灯效状态机。
* `led_manager.c`: **核心逻辑层**。
    * 实现了 `SetLed` 接口。
    * 包含 **基于时间追帧的线性插值算法**，确保渐变时间准确，不随 CPU 负载变慢。
    * 管理双缓冲：`PixelBuffer`(当前显存), `StartBuffer`(渐变起点), `TargetBuffer`(渐变终点)。
* `ws2811_hal.c`: **硬件驱动层**。
    * 使用汇编级 `_nop_` 精确控制纳秒级时序。
    * 针对 SM16703P 时序优化 (0码~300ns, 1码~900ns)。
* `config.h`: 全局配置文件，包含引脚定义、时钟频率、数据类型重定义。

### 开发环境设置 (Keil C51)
**重要：** 由于使用了大量 Buffer，必须正确配置内存模式，否则编译报错或运行死机。
1.  **Target** -> **Memory Model**: 选择 `Large: variables in XDATA`.
2.  **Target** -> **Code Rom Size**: 选择 `Large: 64K program`.
3.  **Output**: 勾选 `Create HEX File`.

## API 使用说明

### 设置 LED 颜色 (带渐变)
```c
/**
 * @brief 设置某颗 LED 的颜色和渐变时间
 * @param index      LED 索引 (0 ~ 65)
 * @param r          红色 (0~255)
 * @param g          绿色 (0~255)
 * @param b          蓝色 (0~255)
 * @param brightness 亮度缩放 (0~1023, 1023为原色)
 * @param fade_ms    渐变时长 (ms)。例如 3000 代表 3秒内由当前色变到目标色
 */
void SetLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint16_t brightness, uint16_t fade_ms);