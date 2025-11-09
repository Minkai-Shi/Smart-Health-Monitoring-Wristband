---
title: Smart Health Monitoring Wristband
date: 2025-10-06 19:27:40
tags: EEPW & EEWorld Activity
categories: EEPW & EEWorld Activity
---
# 🩺 Smart Health Monitoring Wristband / 智能健康监测手环  
### Embedded System Based on MAX78000FTHR / 基于 MAX78000FTHR 的嵌入式系统  

[![Platform](https://img.shields.io/badge/Platform-MAX78000FTHR-blue.svg)]()  
[![Award](https://img.shields.io/badge/Award-EEPW%20%26%20DigiKey%20DIY%20Challenge%202024-brightgreen.svg)]()  
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-lightgrey.svg)]()  
[![Status](https://img.shields.io/badge/Status-Completed-success.svg)]()  

A DIY low-power health monitoring wristband built on the MAX78000FTHR platform, focusing on **embedded system design** and **real-time biosignal processing**.  
该项目基于 MAX78000FTHR 平台开发低功耗健康监测手环，重点展示嵌入式系统设计与实时生理信号处理能力。  

---

## 📷 Project Overview / 项目概述

<p align="center">
  <img src="https://minkai-shi.github.io/upload/SMART_HEALTH_BLOCK_DIAGRAM.png" width="600" alt="System Block Diagram"/>
</p>

With wearable health devices growing in popularity, this project explores **how embedded systems acquire and process biosignals in real time**.  
随着可穿戴健康设备的普及，本项目旨在探索嵌入式系统如何实时采集与处理生理信号。  

**Features / 功能特点：**  
- RGB LED 指示系统状态 / RGB LED system status indication  
- 实时心率与血氧测量（1Hz 刷新） / Real-time HR & SpO₂ (1Hz refresh)  
- OLED 三层菜单界面 / OLED 3-level interactive menu  
- RTC 实时时钟显示 / RTC timekeeping  
- 双按键交互（SW1 菜单切换，SW2 确认） / Button control (SW1 menu, SW2 confirm)  

---

## ⚙️ Hardware Architecture / 硬件架构

| Component / 组件 | Model / 型号 | Function / 功能 |
|------------------|---------------|------------------|
| **MCU** | MAX78000FTHR | Cortex-M4 MCU，支持 NPU 与 I²C 通信 |
| **Sensor / 传感器** | MAXREFDES117 (MAX30102) | PPG 双通道红外/红光心率与血氧采集 |
| **Display / 显示屏** | SSD1306 0.96" OLED | 实时数据显示 128×64 I²C 屏幕 |
| **Clock / 时钟模块** | DS3231 RTC | 硬件时钟模块 |
| **Accessories / 配件** | Breadboard, Jumpers | 原型连接，I²C 线长优化 <5cm |

> The MAX78000’s built-in NPU allows for potential **AI-based health analytics**, while maintaining ultra-low power consumption.  
> MAX78000 内置神经网络加速单元 (NPU)，具备未来 AI 健康分析潜力，同时保持极低功耗。  

---

## 🧠 Development Environment / 开发环境

- **IDE:** Visual Studio Code + Cortex-Debug  
- **SDK:** Analog Devices MSDK  
- **Toolchain:** GCC ARM Embedded 10.3 + Makefile  
- **Communication:** I²C @ 100kHz (optimized timing)  
- **Algorithms / 算法：**  
  - 滑动窗口滤波 / Sliding window filter  
  - 峰值检测 / Peak detection  
  - 比率法血氧计算 / Ratio-based SpO₂ estimation  
  *(Adapted from `maxim_heart_rate_and_oxygen_saturation` in `algorithm.h`)*  

---

## 🔧 Key Technical Challenges / 技术难点与解决方案

| Problem / 问题 | Cause / 原因 | Solution / 解决方案 | Result / 结果 |
|----------------|---------------|----------------------|----------------|
| 字库乱码 / Font corruption | C 注释转义错误 | 重定义特殊字符 | 字符显示正常 |
| I²C 信号不稳定 | 面包板连线过长 | 缩短连线至 <5cm | 数据丢失率 <0.1% |
| OLED 文字镜像 | 初始化寄存器错误 | 调整 `0xC0/0xC8` 参数 | 显示方向正确 |
| 数据刷新延迟 200ms+ | 同步读取与绘制阻塞 | 分离采样与渲染中断 | 延迟 <50ms |

---

## 💻 Core Functions / 核心功能示例

```c
// RGB LED Control / RGB LED 控制
void SetLEDs(uint8_t state) {
    MXC_I2C_WriteByte(I2C0, PMIC_ADDR, LED_REG, state);
}

// HR & SpO₂ Calculation / 心率与血氧算法
maxim_heart_rate_and_oxygen_saturation(ir_data, red_data,
    &spo2, &heart_rate, &valid_spo2, &valid_hr);
```

---

## 🚀 Future Work / 后续计划
- 设计定制 PCB，缩小体积 60%  
- 基于 NPU 实现 CNN 异常心率检测  
- 加入 BLE 蓝牙通信模块（MAX32660）  
- 优化低功耗休眠模式，续航 7+ 天  

---

## 🧭 Learnings / 项目收获
This project deepened understanding of **embedded firmware, biosignal processing, and system optimization**, demonstrating how theory translates to real applications.  
本项目加深了我对嵌入式固件开发、生理信号处理与系统优化的理解，实现了从理论到实践的完整闭环。  

---

## 🙏 Acknowledgments / 致谢
Special thanks to **EEPW** and **DigiKey** for hardware support.  
特别感谢《电子工程与产品世界》和 DigiKey 提供硬件支持。  

📘 **Documentation includes:** 源代码 · 电路图 · I²C 时序日志 · 校准数据  
📎 **GitHub Repository:** [Add your repo link here / 在此处添加仓库链接]  



## 🎥 Project Demonstration Video / 项目演示视频

> 📺 [YouTube Video Link / 视频链接](https://youtu.be/Fb3Psc4_ksg)

---

## 🌐 Official Event & EEPW Forum Posts / 官方活动与论坛帖子

- **EEPW x DigiKey Smart Wristband DIY Challenge (Official Page)**  
  🔗 [https://www.eepw.com.cn/event/action/digikey/diy_2025_sec.html](https://www.eepw.com.cn/event/action/digikey/diy_2025_sec.html)

### 📄 My EEPW Forum Posts / 我在 EEPW 的参赛记录
1. [Project Log #1 – Hardware Design and Wiring 硬件设计与布线](https://forum.eepw.com.cn/thread/396901/1)  
2. [Project Log #2 – Algorithm Development and Debug 调试与算法实现](https://forum.eepw.com.cn/thread/396903/1)  
3. [Project Log #3 – Final Testing and Optimization 测试与优化总结](https://forum.eepw.com.cn/thread/396904/1)
