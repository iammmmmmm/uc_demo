#pragma once
#include "Register.h"
#include <iostream>
#include <map>

namespace UsartRegisters {

// 前向声明所有寄存器类，以便在回调中相互引用
class USART_STS_Register;
class USART_DATA_Register;
// ... (其他寄存器类的声明)

// =========================================================================
// USART_STS (状态寄存器)
// 偏移地址：0x00，复位值：0x0000 00C0 (TXCF/TXBEF 默认置位)
// =========================================================================
class USART_STS_Register : public Register {
public:
    explicit USART_STS_Register(uint32_t offset)
        : Register(offset, "USART_STS (状态寄存器)", 0x000000C0) {

        // --- 错误标志 (PEF, FEF, NEF, OEF, IDLEF) ---
        // 在实际硬件中，这些标志通过特定的读 STS + 读 DATA 序列清除。
        // 在模拟器中，我们暂时将其视为 Read-Only，等待 USART_DATA 读操作的回调逻辑来清除。
        m_fields.push_back({"PEF (校验错误)", 0, 1, {{0, "0: 无错误"}, {1, "1: 出现错误"}}});
        m_fields.push_back({"FEF (帧错误)", 1, 1, {{0, "0: 无错误"}, {1, "1: 出现错误"}}});
        m_fields.push_back({"NEF (噪音错误)", 2, 1, {{0, "0: 无噪音"}, {1, "1: 出现噪音"}}});
        m_fields.push_back({"OEF (过载错误)", 3, 1, {{0, "0: 无错误"}, {1, "1: 出现过载"}}});
        m_fields.push_back({"IDLEF (空闲总线检测)", 4, 1, {{0, "0: 未检测到"}, {1, "1: 检测到"}}});

        // --- 接收/发送状态标志 ---

        // Bit 5: RXBNEF (接收数据缓冲器不为空标志) - RC_W0 或 读 USART_DATA 清除
        m_fields.push_back({"RXBNEF (接收不为空)", 5, 1, {{0, "0: 为空"}, {1, "1: 不为空"}}});

        // Bit 6: TXCF (发送数据完成标志) - RC_W0 或 读 STS + 写 DATA 清除
        m_fields.push_back({"TXCF (发送完成)", 6, 1, {{0, "0: 未完成"}, {1, "1: 完成"}}});

        // Bit 7: TXBEF (发送数据缓冲器为空标志) - 写 USART_DATA 清除
        m_fields.push_back({"TXBEF (发送缓冲为空)", 7, 1, {{0, "0: 不为空"}, {1, "1: 为空"}}});
    }

    // // 重写 write：处理 RC_W0 位的清除逻辑 (仅 TXCF 和 RXBNEF 支持写 0 清除)
    // void write(uint32_t new_value) override {
    //     uint32_t current_value = m_current_value;
    //     // RC_W0 位掩码: RXBNEF(5), TXCF(6)
    //     uint32_t rc_w0_mask = (1U << 5) | (1U << 6);
    //
    //     // 计算应该保持不变的状态位 (只有新值写入 0 且原值为 1 时才清零)
    //     uint32_t sticky_bits = current_value & rc_w0_mask;
    //     uint32_t bits_to_clear = (~new_value) & sticky_bits; // 找到想清零的位
    //
    //     // 结果值 = 非 RC_W0 位 + 剩余的 RC_W0 位
    //     uint32_t final_value = (new_value & (~rc_w0_mask)) | (sticky_bits & (~bits_to_clear));
    //
    //     Register::write(final_value);
    // }

    // 清除接收相关标志 (PEF, FEF, NEF, OEF, IDLEF, RXBNEF)
    void clear_rx_flags() {
        // 清除位 0-5 (PEF, FEF, NEF, OEF, IDLEF, RXBNEF)
        m_current_value &= ~0x3FU;
    }

    // 清除 TXBEF 标志
    void clear_txbe_flag() {
        m_current_value &= ~0x80U; // 清除 Bit 7 (TXBEF)
    }

    // 设置 TXCF 标志 (模拟硬件自动设置)
    void set_txc_flag() {
        m_current_value |= 0x40U; // 设置 Bit 6 (TXCF)
    }
};

// =========================================================================
// USART_DATA (数据寄存器)
// 偏移地址：0x04，复位值：0x0000 0000
// =========================================================================
class USART_DATA_Register : public Register {
private:
    // 存储对 USART_STS 寄存器的引用，用于在读写 DATA 时触发清除逻辑
    USART_STS_Register* m_sts_reg = nullptr;

    // 写回调函数 (清 TXBEF)
    static void write_data_callback(Register &reg, uint32_t new_field_value) {
        USART_DATA_Register* self = static_cast<USART_DATA_Register*>(&reg);
        if (self->m_sts_reg) {
            // 写 DATA 清除 TXBEF (Bit 7)
            self->m_sts_reg->clear_txbe_flag();
            // 模拟发送数据完成后，设置 TXCF (Bit 6)
            self->m_sts_reg->set_txc_flag();
        }
    }

    // 读回调函数 (清 RXBNEF 及错误标志)
    static void read_data_callback(Register &reg, uint32_t &value_to_read) {
        USART_DATA_Register* self = static_cast<USART_DATA_Register*>(&reg);
        if (self->m_sts_reg) {
            // 读 DATA 清除 PEF, FEF, NEF, OEF, IDLEF, RXBNEF (Bit 0-5)
            self->m_sts_reg->clear_rx_flags();
        }
    }

public:
    explicit USART_DATA_Register(uint32_t offset)
        : Register(offset, "USART_DATA (数据寄存器)", 0x00000000) {

        // Bits 7:0: DATA (数据值)
        m_fields.push_back({"DATA (Transmit/Receive Data)", 0, 8, {}});

        // 设置回调
        // set_write_callback( write_data_callback);
        // set_read_callback(read_data_callback);
    }

    void set_sts_register(USART_STS_Register* sts_reg) {
        m_sts_reg = sts_reg;
    }
};


// =========================================================================
// USART_BR1 (波特率寄存器 1) / USART_BR0 (波特率寄存器 0)
// 结合起来形成 16 位的 DIV 字段
// =========================================================================

// USART_BR1: 偏移地址 0x08, DIV[11:4]
class USART_BR1_Register : public Register {
public:
    explicit USART_BR1_Register(uint32_t offset)
        : Register(offset, "USART_BR1 (波特率寄存器 1)", 0x00000000) {
        // Bit 7:0: DIV[11:4] (分频系数的位 11 到位 4)
        m_fields.push_back({"DIV[11:4]", 0, 8, {}});
    }
};

// USART_BR0: 偏移地址 0x0C, DIV[15:12] 和 DIV[3:0]
class USART_BR0_Register : public Register {
public:
    explicit USART_BR0_Register(uint32_t offset)
        : Register(offset, "USART_BR0 (波特率寄存器 0)", 0x00000000) {
        // Bit 3:0: DIV[3:0] (分频系数的低四位)
        m_fields.push_back({"DIV[3:0]", 0, 4, {}});
        // Bit 7:4: DIV[15:12] (分频系数的高四位)
        m_fields.push_back({"DIV[15:12]", 4, 4, {}});
    }
};

// =========================================================================
// USART_CTRL1 (控制寄存器 1)
// 偏移地址：0x10，复位值：0x0000 0000
// =========================================================================
class USART_CTRL1_Register : public Register {
public:
    explicit USART_CTRL1_Register(uint32_t offset)
        : Register(offset, "USART_CTRL1 (控制寄存器 1)", 0x00000000) {
        m_fields.push_back({"PIE (校验错误中断)", 0, 1, {{0, "禁用"}, {1, "使能"}}});
        m_fields.push_back({"PSEL (奇偶校验选择)", 1, 1, {{0, "偶校验"}, {1, "奇校验"}}});
        m_fields.push_back({"PEN (奇偶校验使能)", 2, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"WMS (唤醒方式配置)", 3, 1, {{0, "空闲总线唤醒"}, {1, "地址标记唤醒"}}});
        m_fields.push_back({"DBL (数据位长度)", 4, 1, {{0, "8 个数据位"}, {1, "9 个数据位"}}});
        m_fields.push_back({"USARTDIS (禁止 USART)", 5, 1, {{0, "使能模块"}, {1, "禁用模块"}}});
        m_fields.push_back({"TDB8 (发送数据位 8)", 6, 1, {}});
        m_fields.push_back({"RDB8 (接收数据位 8)", 7, 1, {}});
    }
};

// =========================================================================
// USART_CTRL2 (控制寄存器 2)
// 偏移地址：0x14，复位值：0x0000 0000
// =========================================================================
class USART_CTRL2_Register : public Register {
public:
    explicit USART_CTRL2_Register(uint32_t offset)
        : Register(offset, "USART_CTRL2 (控制寄存器 2)", 0x00000000) {
        m_fields.push_back({"TXBRK (发送断开帧)", 0, 1, {{0, "未发送"}, {1, "将要发送"}}});
        m_fields.push_back({"RMM (接收静默模式)", 1, 1, {{0, "正常工作"}, {1, "静默模式"}}});
        m_fields.push_back({"RXEN (接收使能)", 2, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"TXEN (发送使能)", 3, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"IDLEIE (IDLE 中断使能)", 4, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"RXIE (接收缓冲区非空中断使能)", 5, 1, {{0, "禁止"}, {1, "使能 (OEF 或 RXBNEF 置位)"}}});
        m_fields.push_back({"TXCIE (发送完成中断使能)", 6, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"TXIE (发送缓冲区空中断使能)", 7, 1, {{0, "禁止"}, {1, "使能"}}});
    }
};

// =========================================================================
// USART_CTRL3 (控制寄存器 3)
// 偏移地址：0x18，复位值：0x0000 0000
// =========================================================================
class USART_CTRL3_Register : public Register {
public:
    explicit USART_CTRL3_Register(uint32_t offset)
        : Register(offset, "USART_CTRL3 (控制寄存器 3)", 0x00000000) {
        m_fields.push_back({"LBCP (最后一位时钟脉冲输出)", 0, 1, {{0, "不输出"}, {1, "输出"}}});
        m_fields.push_back({"CLKPHA (时钟相位)", 1, 1, {{0, "第一个边沿"}, {1, "第二个边沿"}}});
        m_fields.push_back({"CLKPOL (时钟极性)", 2, 1, {{0, "空闲低电平"}, {1, "空闲高电平"}}});
        m_fields.push_back({"CLKEN (CK 引脚使能)", 3, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"SBS (停止位配置)", 4, 2, {{0b00, "1 个停止位"}, {0b10, "2 个停止位"}, {0b11, "1.5 个停止位"}}});
        m_fields.push_back({"LINEN (LIN 模式使能)", 6, 1, {{0, "禁止"}, {1, "使能"}}});
    }
};

// =========================================================================
// USART_CTRL4 (控制寄存器 4)
// 偏移地址：0x1C，复位值：0x0000 0000
// =========================================================================
class USART_CTRL4_Register : public Register {
public:
    explicit USART_CTRL4_Register(uint32_t offset)
        : Register(offset, "USART_CTRL4 (控制寄存器 4)", 0x00000000) {
        m_fields.push_back({"ADDR (设备节点地址)", 0, 4, {}});
        m_fields.push_back({"LMBDF (LIN 断开标志)", 4, 1, {{0, "未检测到"}, {1, "检测到"}}});
        m_fields.push_back({"LMBDL (LIN 断开符检测长度)", 5, 1, {{0, "10 位"}, {1, "11 位"}}});
        m_fields.push_back({"LMBDIE (LIN 断开符检测中断使能)", 6, 1, {{0, "禁止"}, {1, "使能"}}});
    }
};

// =========================================================================
// USART_CTRL5 (控制寄存器 5)
// 偏移地址：0x20，复位值：0x0000 0000
// =========================================================================
class USART_CTRL5_Register : public Register {
public:
    explicit USART_CTRL5_Register(uint32_t offset)
        : Register(offset, "USART_CTRL5 (控制寄存器 5)", 0x00000000) {
        m_fields.push_back({"IRDAEN (红外功能使能)", 1, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"ILPM (红外低功耗模式)", 2, 1, {{0, "普通模式"}, {1, "低功耗模式"}}});
        m_fields.push_back({"HDMEN (半双工模式使能)", 3, 1, {{0, "禁止"}, {1, "使能"}}});
        m_fields.push_back({"NACKEN (NACK 传输使能)", 4, 1, {{0, "不发送 NACK"}, {1, "发送 NACK"}}});
        m_fields.push_back({"SMEN (智能卡功能使能)", 5, 1, {{0, "禁止"}, {1, "使能"}}});
    }
};

// =========================================================================
// USART_GTS (保护时间设置寄存器)
// 偏移地址：0x24，复位值：0x0000 0000
// =========================================================================
class USART_GTS_Register : public Register {
public:
    explicit USART_GTS_Register(uint32_t offset)
        : Register(offset, "USART_GTS (保护时间设置寄存器)", 0x00000000) {
        m_fields.push_back({"GTS (设置保护时间值)", 0, 8, {}});
    }
};

// =========================================================================
// USART_PSC (预分频寄存器)
// 偏移地址：0x28，复位值：0x0000 0000
// =========================================================================
class USART_PSC_Register : public Register {
public:
    explicit USART_PSC_Register(uint32_t offset)
        : Register(offset, "USART_PSC (预分频寄存器)", 0x00000000) {
        // PSC[7:0]
        m_fields.push_back({"PSC (设置预分频系数)", 0, 8, {}});
    }
};

// =========================================================================
// USART_SW (切换寄存器) - 仅适用于 USART2/3
// 偏移地址：0x2C，复位值：0x0000 0000
// =========================================================================
class USART_SW_Register : public Register {
public:
    explicit USART_SW_Register(uint32_t offset)
        : Register(offset, "USART_SW (切换寄存器)", 0x00000000) {
        m_fields.push_back({"SW (打开 USART)", 0, 1, {{0, "关闭 USART"}, {1, "打开 USART"}}});
    }
};

// =========================================================================
// USART_IOSW (I/O 切换寄存器) - 仅适用于 USART3
// 偏移地址：0x30，复位值：0x0000 0000
// =========================================================================
class USART_IOSW_Register : public Register {
public:
    explicit USART_IOSW_Register(uint32_t offset)
        : Register(offset, "USART_IOSW (I/O 切换寄存器)", 0x00000000) {
        m_fields.push_back({"SW (打开 USART IO 端口)", 0, 1, {{0, "关闭"}, {1, "打开"}}});
    }
};

} // namespace UsartRegisters