#pragma once

#include "Register.h"
#include <map>
#include <string>
#include <utility> // 确保 std::move 可用

// -------------------------------------------------------------------------
// 寄存器偏移地址定义
// -------------------------------------------------------------------------
#define GPIOx_DOUT_OFFSET    0x00
#define GPIOx_DIN_OFFSET     0x04
#define GPIOx_MODE_OFFSET    0x08
#define GPIOx_CTRL1_OFFSET   0x0C
#define GPIOx_CTRL2_OFFSET   0x10
#define GPIO_JTAGDIS_OFFSET  0x100
// 宏定义常用的位宽，GPIO端口位域宽度为8位 (Bits 7:0)
// 注意：这个宏现在只用于描述范围，不再直接用于位域宽度
#define GPIO_PORT_FIELD_WIDTH 8

// =========================================================================
// 1. GPIOx_DOUT 端口输出数据寄存器 (Offset: 0x00)复位值：0x0000 0000
//    拆分为 8 个独立的 1 位位域 (DOUT7 到 DOUT0)
// =========================================================================
class GPIOx_DOUT_Register : public Register {
    public:
      explicit   GPIOx_DOUT_Register(std::string name)
        : Register(GPIOx_DOUT_OFFSET, std::move(name), 0x00000000) {
            // 拆分：i=0 到 i=7
            for (int i = 0; i < 8; ++i) {
                m_fields.push_back({
                    "DOUT" + std::to_string(i) + " (Pin " + std::to_string(i) + " 输出)",
                    i, // 起始位
                    1, // 宽度为 1
                    {
                        {0, "输出 Low"},
                        {1, "输出 High"}
                    },
                    nullptr
                });
            }
        }
};

// =========================================================================
// 2. GPIOx_DIN 端口输入数据寄存器 (Offset: 0x04)复位值：0x0000 0000
//    拆分为 8 个独立的 1 位位域 (DIN7 到 DIN0)
// =========================================================================
class GPIOx_DIN_Register : public Register {
    public:
       explicit  GPIOx_DIN_Register(std::string name)
         : Register(GPIOx_DIN_OFFSET, std::move(name), 0x00000000) {
            // 拆分：i=0 到 i=7
            for (int i = 0; i < 8; ++i) {
                m_fields.push_back({
                    "DIN" + std::to_string(i) + " (Pin " + std::to_string(i) + " 输入)",
                    i, // 起始位
                    1, // 宽度为 1
                    {
                        {0, "低电平"},
                        {1, "高电平"}
                    },
                    nullptr
                });
            }
        }
};

// =========================================================================
// 3. GPIOx_MODE 端口模式寄存器 (Offset: 0x08)复位值：0x0000 0000
//    拆分为 8 个独立的 1 位位域 (MODE7 到 MODE0)
// =========================================================================
class GPIOx_MODE_Register : public Register {
    public:
        explicit GPIOx_MODE_Register(std::string name)
          : Register(GPIOx_MODE_OFFSET, std::move(name), 0x00000000) {
            // 拆分：i=0 到 i=7
            for (int i = 0; i < 8; ++i) {
                m_fields.push_back({
                    "MODE" + std::to_string(i) + " (Pin " + std::to_string(i) + " 模式)",
                    i, // 起始位
                    1, // 宽度为 1
                    {
                        {0, "输入模式"},
                        {1, "输出模式"}
                    },
                    nullptr
                });
            }
        }
};

// =========================================================================
// 4. GPIOx_CTRL1 端口控制寄存器1 (Offset: 0x0C)复位值：0x0000 0000（GPIOD_CTRL1的复位值是0x00000002）
//    拆分为 8 个独立的 1 位位域 (CR7 到 CR0)
// =========================================================================
class GPIOx_CTRL1_Register : public Register {
    // 移除 m_reset_value，因为它只用于初始化，不应存储
    public:
        // 构造函数：接受寄存器名称和特定的复位值
        explicit GPIOx_CTRL1_Register(std::string name, uint32_t reset_val = 0x00000000)
        : Register(GPIOx_CTRL1_OFFSET, std::move(name), reset_val) { // 直接传递 reset_val 给基类
            // 拆分：i=0 到 i=7
            for (int i = 0; i < 8; ++i) {
                m_fields.push_back({
                    "CR" + std::to_string(i) + " (Pin " + std::to_string(i) + " 功能)",
                    i, // 起始位
                    1, // 宽度为 1
                    {
                        {0, "输入: 浮空输入 / 输出: 开漏输出"},
                        {1, "输入: 上拉输入 / 输出: 推挽输出"}
                    },
                    nullptr
                });
            }
        }
};

// =========================================================================
// 5. GPIOx_CTRL2 端口控制寄存器2 (Offset: 0x10)复位值：0x0000 0000
//    拆分为 8 个独立的 1 位位域 (CR7 到 CR0)
// =========================================================================
class GPIOx_CTRL2_Register : public Register {
    public:
        explicit GPIOx_CTRL2_Register(std::string name)
          : Register(GPIOx_CTRL2_OFFSET, std::move(name), 0x00000000) {
            // 拆分：i=0 到 i=7
            for (int i = 0; i < 8; ++i) {
                 m_fields.push_back({
                    "CR" + std::to_string(i) + " (Pin " + std::to_string(i) + " 速度/中断)",
                    i, // 起始位
                    1, // 宽度为 1
                    {
                        {0, "输入: 禁止中断 / 输出: 2MHz"},
                        {1, "输入: 使能中断 / 输出: 10MHz"}
                    },
                    nullptr
                });
            }
        }
};

// =========================================================================
// 6. GPIO_JTAGDIS JTAG 禁止寄存器 (Offset: 0x100)复位值：0x0000w0000
//    保持不变，因为它已经是 1 位位域。
// =========================================================================
class GPIO_JTAGDIS_Register : public Register {
    public:
        explicit GPIO_JTAGDIS_Register(std::string name = "GPIO_JTAGDIS")
            : Register(GPIO_JTAGDIS_OFFSET, std::move(name), 0x00000000) {
            // JTAGDIS (Bit 0)
            m_fields.push_back({
                "JTAGDIS",
                0,
                1, // 宽度为 1
                {
                    {0, "使能 JTAG 接口 (PD1/PD2 为 SWDIO/SWCLK)"},
                    {1, "禁止 JTAG 接口 (PD1/PD2 为普通 IO)"}
                },
                nullptr
            });
        }
};