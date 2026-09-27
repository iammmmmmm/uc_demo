#pragma once

#include "Register.h" // 包含 Register 类的定义
#include <map>
#include <string>

// ----------------------------------------------------
// 1. ENIT 基础地址和偏移地址宏定义
// ----------------------------------------------------
#define ENIT_BASE               ((uint32_t)0x40001800)
#define ENIT_EINT_CTRL1_OFFSET  0x00
#define ENIT_EINT_CTRL2_OFFSET  0x04
#define ENIT_EINT_CLR_OFFSET    0x08


// =========================================================================
// 2. EINT_CTRL1 (控制寄存器1)
// 偏移地址：0x00 复位值：0x0000 0000
// =========================================================================

class EINT_CTRL1_Register : public Register {
public:
    // 构造函数：接受寄存器完整的偏移地址
    explicit EINT_CTRL1_Register(uint32_t offset)
        : Register(offset, "EINT_CTRL1 (控制寄存器1)") {

        m_current_value = 0x00000000; // 复位值

        // 0. 定义触发模式的通用描述
        map<uint32_t, string> trigger_desc = {
            {0b00, "00: 下降沿和低电平"},
            {0b01, "01: 上升沿"},
            {0b10, "10: 下降沿"},
            {0b11, "11: 上升沿和下降沿"}
        };

        // 位域 1:0: PAIT[1:0]
        m_fields.push_back({"PAIT[1:0] (端口A中断触发配置)", 0, 2, trigger_desc, nullptr});

        // 位域 3:2: PBIT[1:0]
        m_fields.push_back({"PBIT[1:0] (端口B中断触发配置)", 2, 2, trigger_desc, nullptr});

        // 位域 5:4: PCIT[1:0]
        m_fields.push_back({"PCIT[1:0] (端口C中断触发配置)", 4, 2, trigger_desc, nullptr});

        // 位域 7:6: PDIT[1:0]
        m_fields.push_back({"PDIT[1:0] (端口D中断触发配置)", 6, 2, trigger_desc, nullptr});

        // 忽略 31:8 的保留位
    }
    ~EINT_CTRL1_Register() override = default;
};


// =========================================================================
// 3. EINT_CTRL2 (控制寄存器2)
// 偏移地址：0x04 复位值：0x0000 0000
// =========================================================================

class EINT_CTRL2_Register : public Register {
public:
    explicit EINT_CTRL2_Register(uint32_t offset)
        : Register(offset, "EINT_CTRL2 (控制寄存器2)") {

        m_current_value = 0x00000000; // 复位值

        // NMIT: 配置不可屏蔽中断触发 (位 2)
        map<uint32_t, string> nmit_desc = {
            {0, "0: 下降沿"},
            {1, "1: 上升沿"}
        };
        // 注意：此位写入有特殊限制 (外部中断禁用时才能写入)
        m_fields.push_back({"NMIT (不可屏蔽中断触发配置)", 2, 1, nmit_desc, nullptr});

        // 忽略保留位
    }
    ~EINT_CTRL2_Register() override = default;
};


// =========================================================================
// 4. EINT_CLR (中断清除寄存器)
// 偏移地址：0x08 复位值：0x0000 0000
// 注意：该寄存器位域写 1 清零
// =========================================================================

class EINT_CLR_Register : public Register {
public:
    explicit EINT_CLR_Register(uint32_t offset)
        : Register(offset, "EINT_CLR (中断清除寄存器)") {

        m_current_value = 0x00000000; // 复位值

        // 清除位域的通用描述 (写1清除标志位)
        map<uint32_t, string> clear_desc = {
            {0, "0: 无操作"},
            {1, "1: 清除中断标志位"}
        };

        // 写入回调函数：模拟写 1 清除标志的动作 (虽然该寄存器在读回时会是 0)
        auto clear_callback = [](Register &reg, uint32_t new_field_value) {
            if (new_field_value == 1) {
                // 实际的硬件中，写 1 会清除状态寄存器中的对应标志位
                if (s_enable_debug_output) {
                    std::cout << "    [Callback] **" << "EINT_CLR_Register" << "** 中的中断标志位已被请求清除。" << std::endl;
                }
            }
        };

        // Bit 0: PAIC
        m_fields.push_back({"PAIC (清除端口A中断)", 0, 1, clear_desc, clear_callback});

        // Bit 1: PBIC
        m_fields.push_back({"PBIC (清除端口B中断)", 1, 1, clear_desc, clear_callback});

        // Bit 2: PCIC
        m_fields.push_back({"PCIC (清除端口C中断)", 2, 1, clear_desc, clear_callback});

        // Bit 3: PDIC
        m_fields.push_back({"PDIC (清除端口D中断)", 3, 1, clear_desc, clear_callback});

        // Bit 6: NMIC (跳过 Bit 5:4 保留位)
        m_fields.push_back({"NMIC (清除不可屏蔽中断)", 6, 1, clear_desc, clear_callback});

        // 忽略保留位
    }
    ~EINT_CLR_Register() override = default;
};