# ESP32-S3 教学资产

本仓库包含 ESP32-S3 从入门到进阶的完整教学资源，包括历程代码、教学 PPT 和配套视频，适合电子信息类学生或嵌入式爱好者学习使用。

## 📂 目录结构

```
ESP32-S3/
├── examples/          # 历程项目代码
│   ├── 01-LED/               # 点亮第一个 LED
│   ├── 02-LED_blink/         # LED 闪烁
│   ├── 03-Buzzer/            # 蜂鸣器控制
│   ├── 04-Button/            # 按键输入
│   ├── 05-interrupt1/        # 外部中断（基础）
│   ├── 06-interrupt2/        # 外部中断（进阶）
│   ├── 07-timer/             # 定时器使用
│   ├── OLED1(developing)/    # OLED 驱动（开发中）
│   └── WIFI(developing)/     # WiFi 功能（开发中）
├── PPT/               # 教学幻灯片
│   ├── 0课程介绍.pptx
│   ├── 1开发环境搭建.pptx
│   └── 2-GPIO输出.pptx
└── videos/            # 教学视频（见下方链接）
```

## 🛠️ 硬件要求

- 开发板：ESP32-S3 核心板（推荐 ESP32-S3-DevKitC-1）
- 元器件：LED、按键、蜂鸣器、OLED 屏（SSD1306）、杜邦线若干
- 调试器：USB 数据线（支持数据传输）

## 💻 软件环境

- 操作系统：Windows 10/11、macOS 或 Linux
- 开发框架：ESP-IDF v5.0+
- 编辑器：VS Code + ESP-IDF 插件

## 🚀 快速开始

1. **克隆仓库**
   ```bash
   git clone https://gitcode.com/fengtian65/ESP32-S3.git
   cd ESP32-S3
   ```

2. **编译与烧录**
   确保已在 VS Code 中安装并配置好 **ESP-IDF 插件**，然后按以下可视化步骤操作（无需命令行）：

   1. **打开项目**：在 VS Code 中打开任意历程项目文件夹（如 `examples/01-LED/`），等待 ESP-IDF 插件自动加载。
   2. **选择串口**：点击 VS Code 底部状态栏的「插头图标」（或「选择串口端口」文字），在弹出列表中选择你的 ESP32-S3 连接的端口（Windows 常见 `COM3`/`COM4`，Mac/Linux 常见 `/dev/ttyUSB0`）。
   3. **选择目标芯片**：点击底部状态栏的「芯片型号」（默认可能是 `ESP32`），在弹出列表中选择 `ESP32-S3`。
   4. **编译**：点击底部状态栏的「✓ 编译」图标（或按 `Ctrl+E` 再按 `B`），等待编译完成（底部终端会显示「Build complete」）。
   5. **烧录**：点击底部状态栏的「🔥 烧录」图标（或按 `Ctrl+E` 再按 `F`），插件会自动将程序烧录到 ESP32-S3。
   6. **打开串口监视器**：点击底部状态栏的「🖥️ 监视器」图标（或按 `Ctrl+E` 再按 `M`），即可查看程序运行日志。

## 📖 配套文字版教程
本系列课程配套完整的文字版保姆级教程已同步发布至CSDN，内容与仓库历程代码、教学视频一一对应，包含详细的原理讲解、硬件接线图示、逐行代码解析和新手常见问题排查，既可以跟着课程逐节学习，也可作为日常开发的查阅手册。

专栏合集地址：[ESP32-S3开发教程](https://blog.csdn.net/fengtian65/category_13080617.html)

## 📺 教学视频

所有配套教学视频已上传至bilibili，点击下方链接观看：
- [第0章：课程介绍]https://www.bilibili.com/video/BV1z2c4zfEPE/
- [第1章：开发环境搭建]https://www.bilibili.com/video/BV1z2c4zfEFc/
- *（持续更新中）*

## 📝 更新日志

- 2026-02-16：初始化仓库，添加 LED、按键、蜂鸣器等基础历程

## 🤝 贡献

欢迎提交 Issue 或 Pull Request 来完善本教程！