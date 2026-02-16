#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

// ==================== 教程专用：简单配置（新手只需改这里） ====================
#define OLED_I2C_ADDR    0x3C    // OLED地址（大部分是0x3C，少数0x3D）
#define OLED_SCL_PIN     9      // SCL引脚（教程固定）
#define OLED_SDA_PIN     10      // SDA引脚（教程固定）
#define OLED_WIDTH       128     // OLED宽度（固定）
#define OLED_HEIGHT      64      // OLED高度（固定）

// OLED设备句柄（新手不用理解，只需要知道是I2C设备标识）
static i2c_master_dev_handle_t oled_dev;

// ==================== 核心基础函数（教程重点讲解） ====================
// 向OLED写命令（底层基础，不用改）
static void oled_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd}; // 0x00=写命令模式
    i2c_master_transmit(oled_dev, data, 2, -1);
}

// 向OLED写数据（底层基础，不用改）
static void oled_write_data(uint8_t data)
{
    uint8_t buf[2] = {0x40, data}; // 0x40=写数据模式
    i2c_master_transmit(oled_dev, buf, 2, -1);
}

// 清屏（教程必讲：最简单的OLED操作）
static void oled_clear(void)
{
    for(int page=0; page<8; page++) { // OLED按8页划分，每页8行
        oled_write_cmd(0xB0 + page);   // 选择第page页
        oled_write_cmd(0x00);          // 列地址低4位
        oled_write_cmd(0x10);          // 列地址高4位
        for(int col=0; col<128; col++) {
            oled_write_data(0x00);     // 写0=熄灭像素
        }
    }
}

// 初始化OLED（教程必讲：只需知道这是“开机设置”）
static void oled_init(void)
{
    // 1. 初始化ESP32-S3的I2C总线
    i2c_master_bus_config_t i2c_cfg = {
        .i2c_port = I2C_NUM_0,
        .scl_io_num = OLED_SCL_PIN,
        .sda_io_num = OLED_SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t i2c_bus;
    i2c_new_master_bus(&i2c_cfg, &i2c_bus);

    // 2. 添加OLED设备到I2C总线
    i2c_device_config_t oled_cfg = {
        .device_address = OLED_I2C_ADDR,
        .dev_addr_length = I2C_ADDR_BIT_7,
        .scl_speed_hz = 400000,
    };
    i2c_master_bus_add_device(i2c_bus, &oled_cfg, &oled_dev);

    // 3. OLED基础初始化（固定指令，教程不用深讲，知道是“初始化设置”即可）
    vTaskDelay(pdMS_TO_TICKS(100)); // 上电等100ms
    oled_write_cmd(0xAE); // 关闭显示
    oled_write_cmd(0x20); // 内存地址模式
    oled_write_cmd(0x10); // 水平寻址模式
    oled_write_cmd(0xB0); // 起始页地址
    oled_write_cmd(0xC8); // 列扫描方向
    oled_write_cmd(0x00); // 低列地址
    oled_write_cmd(0x10); // 高列地址
    oled_write_cmd(0x40); // 起始行地址
    oled_write_cmd(0x81); // 对比度设置
    oled_write_cmd(0xFF); // 最大对比度
    oled_write_cmd(0xA1); // 段重映射
    oled_write_cmd(0xA6); // 正常显示
    oled_write_cmd(0xA8); // 多路复用率
    oled_write_cmd(0x3F); // 64行
    oled_write_cmd(0xA4); // 显示RAM内容
    oled_write_cmd(0xD3); // 显示偏移
    oled_write_cmd(0x00); // 无偏移
    oled_write_cmd(0xD5); // 时钟分频
    oled_write_cmd(0xF0); // 分频因子
    oled_write_cmd(0xD9); // 预充电周期
    oled_write_cmd(0x22); // 预充电15个时钟，放电1个时钟
    oled_write_cmd(0xDA); // COM引脚配置
    oled_write_cmd(0x12);
    oled_write_cmd(0xDB); // VCOMH电压
    oled_write_cmd(0x20);
    oled_write_cmd(0x8D); // 电荷泵
    oled_write_cmd(0x14); // 开启电荷泵
    oled_write_cmd(0xAF); // 开启显示

    oled_clear(); // 初始化后清屏
}

// ==================== 教程核心：简单显示函数 ====================
// 8x16点阵字模（只保留教程需要的字符：空格、0-9、ESP32-S3相关字母）
const unsigned char font8x16[] = {
    // 空格
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // 0
    0x00,0x7C,0x12,0x11,0x12,0x7C,0x00,0x00,0x00,0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,
    // 1
    0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0x00,0x00,0x00,0x00,0x00,
    // 2
    0x00,0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x41,0x41,0x41,0x41,0x3E,0x00,0x00,
    // 3
    0x00,0x24,0x24,0x24,0x24,0x7C,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x3E,0x00,0x00,
    // E
    0x00,0x78,0x44,0x44,0x44,0x78,0x00,0x00,0x00,0x7F,0x41,0x41,0x41,0x7F,0x00,0x00,
    // S
    0x00,0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x1E,0x00,0x00,
    // P
    0x00,0x7C,0x12,0x12,0x12,0x0C,0x00,0x00,0x00,0x3E,0x41,0x41,0x41,0x20,0x00,0x00,
    // 3（重复，方便索引）
    0x00,0x24,0x24,0x24,0x24,0x7C,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x3E,0x00,0x00,
    // 2（重复）
    0x00,0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x41,0x41,0x41,0x41,0x3E,0x00,0x00,
    // S（重复）
    0x00,0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x1E,0x00,0x00,
};

// 显示单个字符（教程重点：简单易懂）
static void oled_show_char(int x, int y, char c)
{
    // 计算字符在字模表中的索引（只处理教程用到的字符）
    int idx = 0;
    if(c == ' ') idx = 0;
    else if(c >= '0' && c <= '3') idx = c - '0' + 1;
    else if(c == 'E') idx = 5;
    else if(c == 'S') idx = 6;
    else if(c == 'P') idx = 7;

    // 设置显示位置：x=列(0-127)，y=页(0-7)
    oled_write_cmd(0xB0 + y);
    oled_write_cmd(0x00 + (x & 0x0F));
    oled_write_cmd(0x10 + (x >> 4));

    // 写字符上8行
    for(int i=0; i<8; i++) oled_write_data(font8x16[idx*16 + i]);
    // 设置下一页，写字符下8行
    oled_write_cmd(0xB0 + y + 1);
    oled_write_cmd(0x00 + (x & 0x0F));
    oled_write_cmd(0x10 + (x >> 4));
    for(int i=0; i<8; i++) oled_write_data(font8x16[idx*16 + i + 8]);
}

// 显示简单字符串（教程重点：循环显示字符）
static void oled_show_string(int x, int y, char *str)
{
    while(*str) {
        oled_show_char(x, y, *str);
        x += 8; // 每个字符占8列
        str++;
    }
}

// ==================== 教程主任务：最终效果演示 ====================
void app_main(void)
{
    // 1. 初始化OLED
    oled_init();

    // 2. 循环显示内容（教程演示核心）
    uint16_t count = 0;
    char num_buf[5]; // 存储数字转字符串
    while(1) {
        oled_clear(); // 先清屏
        
        // 显示固定文字：ESP32-S3
        oled_show_string(10, 0, "ESP32-S3");
        // 显示计数数字：Count: 0000
        oled_show_string(10, 3, "Count: ");
        snprintf(num_buf, sizeof(num_buf),"%04d", count); // 数字转4位字符串
        oled_show_string(60, 3, num_buf);

        count++;
        if(count > 9999) count = 0; 
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
    }
}