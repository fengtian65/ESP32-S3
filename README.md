# ESP32-S3 教学资产

本仓库包含 ESP32-S3 从入门到进阶的完整教学资源，包括例程代码、教学 PPT 和配套视频，适合电子信息类学生或嵌入式爱好者学习使用。

---

## 🏢 关于我们

**AI融创创新实验室**是**中北大学仪器与电子学院创新精英研究院**旗下的科创实践平台，于**2025年5月8日**正式成立。实验室以“人工智能与AI+”为核心发展方向，聚焦**AI+嵌入式**、**AIGC**、**前沿探索**三大领域，致力于推动AI技术与传统工科的交叉融合，培养兼具创新思维与工程实践能力的复合型人才。

自成立以来，实验室已累计举办多期AI主题工作坊与技术培训，覆盖嵌入式开发、大模型应用、生成式AI等前沿方向，吸引了校内外众多爱好者参与。目前实验室拥有成员70余人，涵盖仪器与电子、计算机、自动化等多个学科，形成了跨专业、梯队化的科创团队。

实验室高度重视教学资源的沉淀与开源共享，此次推出的**ESP32-S3教学资产**便是实验室在“AI+嵌入式”方向的重要成果之一。同时，实验室在bilibili等平台同步发布配套教学视频（账号：**AI融创创新实验室**），结合GitCode开源代码与CSDN文字教程，打造“理论-实践-拓展”三位一体的学习体系，助力更多零基础学习者快速入门嵌入式与AI开发。

未来，实验室将继续深耕AI+领域，持续推出更多优质教学资源与科创项目，积极探索前沿技术在工程实践中的应用，为人工智能与实体经济的深度融合贡献力量。

---

## 📂 目录结构

```
ESP32-S3/
├── examples/              # 基础例程项目代码
│   ├── 01-LED/           # 点亮第一个 LED
│   ├── 02-LED_blink/     # LED 流水灯
│   ├── 03-Buzzer/        # 蜂鸣器与FreeRTOS多任务协同
│   ├── 04-Button/        # GPIO输入/按键控制LED
│   ├── 05-interrupt1/    # 按键中断1（基础）
│   ├── 06-interrupt2/    # 按键中断2（FreeRTOS进阶）
│   ├── 07-timer/         # 硬件定时器
│   ├── 08-PWM/           # PWM呼吸灯
│   ├── 09-UART/          # UART串口通信
│   ├── 10-using-tem/     # DHT11温湿度传感器基础数据读取
│   ├── 11-MPU6050/       # I2C通信与MPU6050六轴传感器
│   ├── ESP-DL-test/      # ESP-DL深度学习库基础测试例程
│   ├── OLED1(developing)/# OLED 屏驱动（开发中）
│   └── WIFI(developing)/ # WiFi 联网功能（开发中）
├── introduce/            # 实验室介绍文档
│   └── README.md         # 实验室介绍文档
├── PPT/                  # 配套教学幻灯片
│   ├── 0课程介绍.pptx
│   ├── 1开发环境搭建.pptx
│   └── 2-GPIO输出（点亮一个LED）.pptx
├── project/              # 进阶实战项目
│   └── tem/              # 温湿度采集+扣子Coze AI智能体实战项目
│       ├── button.c/h    # 按键中断与消抖处理模块
│       ├── coze_api.c/h  # Coze智能体API调用（流式SSE解析）模块
│       ├── dht11.c/h     # DHT11温湿度采集模块
│       ├── main.c        # 主任务调度（WiFi+按键+温湿度+API整合）
│       ├── test.py       # Coze API本地可用性验证脚本
│       └── wifi_conn.c/h # WiFi连接与重连模块
├── esp-dl/               # ESP-DL深度学习库官方依赖文件
├── LICENSE               # 开源协议
└── README.md             # 项目主说明文档
```

---

## 🛠️ 硬件要求

- **开发板**：ESP32-S3 核心板（推荐 ESP32-S3-DevKitC-1）
- **基础元器件**：LED、按键、蜂鸣器、OLED 屏（SSD1306）、杜邦线若干
- **传感器相关**：MPU6050六轴传感器（10-I2C-MPU6050例程专用）、DHT11温湿度传感器（11-using-tem、tem项目专用）、轻触按键（或直接使用开发板BOOT键）
- **调试器**：USB 数据线（支持数据传输）

---

## 💻 软件环境

- **操作系统**：Windows 10/11、macOS 或 Linux
- **开发框架**：ESP-IDF v5.0+
- **编辑器**：VS Code + ESP-IDF 插件
- **辅助工具**：Python3（用于验证Coze API可用性）、串口助手（波特率115200）
- **平台依赖**：Coze（扣子）平台账号（tem项目专用，需完成智能体创建、沙箱部署）

---

## 🚀 快速开始

### 1. 克隆仓库
```bash
git clone https://gitcode.com/NUCAILab/ESP32-S3.git
cd ESP32-S3
```

### 2. 编译与烧录（重要操作说明）

> ⚠️ **核心注意事项**：ESP-IDF 为单工程编译架构，**必须进入 `examples` 目录下的单个例程文件夹 或 `project` 目录下的实战项目文件夹内执行编译、烧录操作**，禁止在仓库根目录直接操作。

确保已在 VS Code 中安装并配置好**ESP-IDF 插件**，按以下步骤操作：

1. **打开目标工程**：在 VS Code 中通过「文件-打开文件夹」，选择 `examples` 目录下的单个例程文件夹（如 `examples/01-LED/`）或 `project` 目录下的实战项目文件夹（如 `project/tem/`），等待 ESP-IDF 插件自动加载工程。
2. **选择串口**：点击 VS Code 底部状态栏的「插头图标」（或「选择串口端口」文字），在弹出列表中选择你的 ESP32-S3 连接的端口（Windows 常见 `COM3` / `COM4`，Mac/Linux 常见 `/dev/ttyUSB0`）。
3. **选择目标芯片**：点击底部状态栏的「芯片型号」，在弹出列表中选择 `ESP32-S3`。
4. **编译工程**：点击底部状态栏的「✓ 编译」图标（或按 `Ctrl+E` 再按 `B`），等待编译完成（底部终端会显示「Build complete」）。
5. **烧录程序**：点击底部状态栏的「🔥 烧录」图标（或按 `Ctrl+E` 再按 `F`），插件会自动将程序烧录到 ESP32-S3。
6. **打开串口监视器**：点击底部状态栏的「🖥️ 监视器」图标（或按 `Ctrl+E` 再按 `M`），即可查看程序运行日志。

### 3. 进阶项目：温湿度AI生活建议 配置与使用说明

该项目实现「DHT11温湿度采集 → WiFi联网 → 按键触发Coze AI智能体请求 → 串口打印生活建议」全流程，完整保姆级使用教程可参考配套CSDN文档：[ESP32-S3实战教程：温湿度采集+Coze AI智能体全流程开发](https://blog.csdn.net/fengtian65/article/details/158932785?spm=1001.2014.3001.5501)

#### 3.1 核心配置修改
进入 `project/tem/` 文件夹，修改以下文件中的关键参数：
| 文件路径               | 需修改的配置项                | 说明                                  |
|------------------------|-----------------------------|---------------------------------------|
| `wifi_conn.h`          | `WIFI_SSID`/`WIFI_PASS`     | 替换为你的2.4G WiFi名称和密码（仅支持2.4G） |
| `coze_api.c`           | `COZE_URL`/`COZE_TOKEN`/`COZE_PROJECT` | 替换为Coze平台获取的流式API地址、Token、Project ID |
| `dht11.h`（可选）| `DHT11_GPIO`                | 修改DHT11 DATA引脚（默认GPIO5）        |
| `button.c`（可选）| `BUTTON_GPIO`               | 修改触发按键引脚（默认GPIO0，即BOOT键） |

#### 3.2 硬件接线（tem项目专用）
所有接线均使用3.3V供电（避免5V烧毁引脚）：
| 外设       | ESP32-S3 引脚 | 备注                     |
|------------|---------------|--------------------------|
| DHT11-VCC  | 3.3V          | 严禁接5V                 |
| DHT11-GND  | GND           | 共地                     |
| DHT11-DATA | GPIO5（默认） | 可在`dht11.h`中修改      |
| 触发按键   | GPIO0（BOOT） | 按下触发请求，也可自定义引脚 |
| 按键另一端 | GND           | 代码已开启GPIO上拉        |

#### 3.3 功能测试
1. 烧录完成后，串口监视器查看WiFi连接状态（显示`Connected! IP: xxx.xxx.xxx.xxx`即为联网成功）；
2. 按下开发板BOOT键（或自定义按键），触发温湿度采集与AI请求；
3. 串口会依次打印「采集的温湿度数据」→「API请求日志」→「Coze智能体返回的生活建议」。

#### 3.4 排错小技巧
- 若WiFi连接失败：确认WiFi为2.4G、SSID/密码无拼写错误、路由器未开启MAC过滤；
- 若DHT11读取失败：检查接线是否牢固、供电是否为3.3V；
- 若API调用失败：先运行`project/tem/test.py`（修改同配置），验证Coze API是否可用；
- 若curl测试Coze API报503：使用项目内修正后的curl命令（带流式请求头+完整JSON体）。

---

## 📖 配套文字版教程

本系列课程配套完整的文字版保姆级教程已同步发布至CSDN，内容与仓库例程代码、教学视频一一对应，包含详细的原理讲解、硬件接线图示、逐行代码解析和新手常见问题排查，既可以跟着课程逐节学习，也可作为日常开发的查阅手册。

**专栏合集地址**：[ESP32-S3开发教程](https://blog.csdn.net/fengtian65/category_13080617.html)

**已发布教程列表**：
- [ESP32_S3开发环境搭建教程（VS Code+ESP-IDF）](https://blog.csdn.net/fengtian65/article/details/154435731)
- [ESP32-S3开发教程一：点亮一个LED（基于VS Code+ESP-IDF）](https://blog.csdn.net/fengtian65/article/details/156945047)
- [ESP32-S3开发教程二：LED流水灯（基于VS Code+ESP-IDF）](https://blog.csdn.net/fengtian65/article/details/156990542)
- [ESP32-S3开发教程三：蜂鸣器与FreeRTOS多任务协同](https://blog.csdn.net/fengtian65/article/details/157022610)
- [ESP32-S3开发教程四：GPIO输入/按键控制LED](https://blog.csdn.net/fengtian65/article/details/157060861)
- [ESP32-S3开发教程五-按键中断1](https://blog.csdn.net/fengtian65/article/details/157618867)
- [ESP32-S3开发教程五-按键中断2（使用FreeRTOS）](https://blog.csdn.net/fengtian65/article/details/157844582)
- [ESP32-S3开发教程6：硬件定时器](https://blog.csdn.net/fengtian65/article/details/158100984)
- [ESP32-S3开发教程7：PWM呼吸灯](https://blog.csdn.net/fengtian65/article/details/158541436)
- [ESP32-S3开发教程8：UART串口通信](https://blog.csdn.net/fengtian65/article/details/158582354)
- [ESP32-S3开发教程9：扣子智能体快速搭建与API调用例程使用讲解](https://blog.csdn.net/fengtian65/article/details/158932785)
- [ESP32-S3开发教程10：I2C与MPU6050](https://blog.csdn.net/fengtian65/article/details/159156165)

---

## 📺 教学视频

所有配套教学视频已上传至bilibili，点击下方链接观看：

- [第0章：课程介绍](https://www.bilibili.com/video/BV1z2c4zfEPE/)
- [第1章：开发环境搭建](https://www.bilibili.com/video/BV1z2c4zfEFc/)
- _（持续更新中）_

---

## 📝 更新日志

- **2026-03-17**：新增`11-MPU6050`I2C通信与MPU6050六轴传感器例程；补充配套CSDN教程链接；更新项目目录结构
- **2026-03-11**：新增`10-using-tem`温湿度基础读取例程、`ESP-DL-test`深度学习测试例程；补充温湿度AI实战项目完整CSDN使用教程；更新项目目录结构，补全仓库文件层级说明
- **2026-03-08**：完善`project/tem`温湿度AI项目文档，补充配置说明、硬件接线、排错技巧
- **2026-03-06**：新增「UART串口通信」例程，优化项目目录结构，拆分基础例程与进阶实战项目
- **2026-03-02**：新增「PWM呼吸灯」例程与配套教程
- **2026-02-24**：新增「DHT11温湿度采集+扣子Coze智能体」进阶实战项目，修复基础例程若干bug，完善README编译烧录操作说明
- **2026-02-16**：新增「硬件定时器」例程与教程；初始化仓库，添加 LED、按键、蜂鸣器等基础例程
- **2026-02-13**：新增「按键中断（FreeRTOS进阶）」例程与教程
- **2026-02-01**：新增「按键中断（基础）」例程与教程
- **2026-01-17**：新增「GPIO输入/按键控制LED」例程与教程
- **2026-01-16**：新增「蜂鸣器与FreeRTOS多任务协同」、「LED流水灯」例程与教程
- **2026-01-15**：新增「点亮一个LED」例程与教程
- **2025-11-05**：发布「ESP32-S3开发环境搭建教程」

---

## 🤝 贡献

欢迎提交 Issue 或 Pull Request 来完善本教程！
- 基础例程bug修复、代码注释优化；
- 进阶项目功能拓展（如OLED显示温湿度、多按键控制）；
- 教程文档补充、排错案例新增。

---

## 📄 许可证

本项目采用 [MIT License](LICENSE) 开源协议。