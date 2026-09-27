#pragma once
#include "Register.h" // 确保包含 Register 基类的定义
#include <vector>
#include <map>
#include <string>

namespace SpiRegisters {

// =========================================================================
// SPI_CTRL1 (SPI 控制寄存器 1)
// 偏移地址：0x00，复位值：0x0000 0000
// =========================================================================
class SPI_CTRL1_Register : public Register {
public:
    explicit SPI_CTRL1_Register(uint32_t offset)
        : Register(offset, "SPI_CTRL1 (SPI 控制寄存器 1)", 0x00000000) {
        // Bit 0: CLKPHA (配置时钟相位)
        m_fields.push_back({
            "CLKPHA (Clock Phase Configure)", 0, 1,
            {{0, "0: 在第 1 个时钟边沿采样"}, {1, "1: 在第 2 个时钟边沿采样"}}
        });

        // Bit 1: CLKPOL (配置时钟极性)
        m_fields.push_back({
            "CLKPOL (Clock Polarity Configure)", 1, 1,
            {{0, "0: 空闲时 SCK 为低电平"}, {1, "1: 空闲时 SCK 为高电平"}}
        });

        // Bit 2: MSTMODE (配置主/从模式)
        m_fields.push_back({
            "MSTMODE (Master/Slave Mode Configure)", 2, 1,
            {{0, "0: 从模式"}, {1, "1: 主模式"}}
        });

        // Bits 5:3: BRC (选择波特率分频系数)
        m_fields.push_back({
            "BRC (Baud Rate Divider Factor Select)", 3, 3,
            {
                {0b000, "000: DIV=2 (FMASTER/2)"},
                {0b001, "001: DIV=4 (FMASTER/4)"},
                {0b010, "010: DIV=8 (FMASTER/8)"},
                {0b011, "011: DIV=16 (FMASTER/16)"},
                {0b100, "100: DIV=32 (FMASTER/32)"},
                {0b101, "101: DIV=64 (FMASTER/64)"},
                {0b110, "110: DIV=128 (FMASTER/128)"},
                {0b111, "111: DIV=256 (FMASTER/256)"}
            }
        });

        // Bit 6: SPIEN (使能 SPI 设备)
        m_fields.push_back({
            "SPIEN (SPI Device Enable)", 6, 1,
            {{0, "0: 禁用"}, {1, "1: 使能"}}
        });

        // Bit 7: LSBF (选择 LSB 首位传输)
        m_fields.push_back({
            "LSBF (LSB First Transfer Select)", 7, 1,
            {{0, "0: 先发送最高有效位 (MSB)"}, {1, "1: 先发送最低有效位 (LSB)"}}
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// SPI_CTRL2 (SPI 控制寄存器 2)
// 偏移地址：0x04，复位值：0x0000 0000
// =========================================================================
class SPI_CTRL2_Register : public Register {
public:
    explicit SPI_CTRL2_Register(uint32_t offset)
        : Register(offset, "SPI_CTRL2 (SPI 控制寄存器 2)", 0x00000000) {
        // Bit 0: ISS (选择内部从设备)
        m_fields.push_back({
            "ISS (Internal Slave Device Select, SSC=1时有效)", 0, 1,
            {{0, "0: 从模式, 内部 NSS 为低电平"}, {1, "1: 主模式, 内部 NSS 为高电平"}}
        });

        // Bit 1: SSC (使能软件从设备)
        m_fields.push_back({
            "SSC (Software Slave Device Enable)", 1, 1,
            {{0, "0: 禁止软件 NSS 模式"}, {1, "1: 使能软件 NSS 模式 (ISS 位的值代替 NSS 引脚)"}}
        });

        // Bit 2: UMRXO (使能仅接收模式)
        m_fields.push_back({
            "UMRXO (Receive Only Mode Enable)", 2, 1,
            {{0, "0: 同时发送和接收"}, {1, "1: 仅接收模式"}}
        });

        // Bit 4: CRCNXT (使能下一个传输数据是 CRC)
        m_fields.push_back({
            "CRCNXT (CRC Transfer Next Enable)", 4, 1,
            {{0, "0: 下一个传输数据来自发送缓冲区"}, {1, "1: 下一个传输数据来自 CRC 寄存器"}}
        });

        // Bit 5: CRCEN (使能 CRC 校验)
        m_fields.push_back({
            "CRCEN (CRC Calculate Enable)", 5, 1,
            {{0, "0: 禁止"}, {1, "1: 使能"}}
        });

        // Bit 6: BMTX (使能双向模式的输出)
        m_fields.push_back({
            "BMTX (Bidirectional Mode Output Enable, BMEN=1时有效)", 6, 1,
            {{0, "0: 仅接收模式 (禁止输出)"}, {1, "1: 仅发送模式 (使能输出)"}}
        });

        // Bit 7: BMEN (使能双向模式)
        m_fields.push_back({
            "BMEN (Bidirectional Mode Enable)", 7, 1,
            {{0, "0: 双线单向模式"}, {1, "1: 单线双向模式"}}
        });

        // 忽略 3, 31:8 的保留位
    }
};

// =========================================================================
// SPI_INTCTRL (SPI 中断控制寄存器)
// 偏移地址：0x08，复位值：0x0000 0000
// =========================================================================
class SPI_INTCTRL_Register : public Register {
public:
    explicit SPI_INTCTRL_Register(uint32_t offset)
        : Register(offset, "SPI_INTCTRL (SPI 中断控制寄存器)", 0x00000000) {
        // Bit 4: WUPIE (使能唤醒中断)
        m_fields.push_back({
            "WUPIE (Wakeup Interrupt Enable)", 4, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (WUPF=1时产生)"}}
        });

        // Bit 5: ERRIE (使能错误中断)
        m_fields.push_back({
            "ERRIE (Error Interrupt Enable)", 5, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (错误发生时产生)"}}
        });

        // Bit 6: RXBNEIE (使能接收缓冲区非空中断)
        m_fields.push_back({
            "RXBNEIE (Receive Buffer Not Empty Interrupt Enable)", 6, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (RXBNEF=1时产生)"}}
        });

        // Bit 7: TXBEIE (使能发送缓冲区空中断)
        m_fields.push_back({
            "TXBEIE (Transmit Buffer Empty Interrupt Enable)", 7, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (TXBEF=1时产生)"}}
        });

        // 忽略 3:0, 31:8 的保留位
    }
};

// =========================================================================
// SPI_STS (SPI 状态寄存器)
// 偏移地址：0x0C，复位值：0x0000 0002
// 注意：RC_W0 字段（位 3, 4, 5, 6）需要软件写 0 清除。
// =========================================================================
class SPI_STS_Register : public Register {
public:
    explicit SPI_STS_Register(uint32_t offset)
        : Register(offset, "SPI_STS (SPI 状态寄存器)", 0x00000002) {
        // Bit 0: RXBNEF (接收缓冲非空标志, R)
        m_fields.push_back({
            "RXBNEF (Receive Buffer Not Empty Flag)", 0, 1,
            {{0, "0: 接收缓冲空"}, {1, "1: 接收缓冲非空"}}
        });

        // Bit 1: TXBEF (发送缓冲器为空标志, R)
        m_fields.push_back({
            "TXBEF (Transmit Buffer Empty Flag)", 1, 1,
            {{0, "0: 发送缓冲非空"}, {1, "1: 发送缓冲空"}}
        });

        // Bit 3: WUPF (发生唤醒事件标志, RC_W0)
        m_fields.push_back({
            "WUPF (Wakeup Event Occur Flag)", 3, 1,
            {{0, "0: 未发生"}, {1, "1: 发生"}}
        });

        // Bit 4: CRCEF (发生 CRC 错误标志, RC_W0)
        m_fields.push_back({
            "CRCEF (CRC Error Occur Flag)", 4, 1,
            {{0, "0: CRC 匹配"}, {1, "1: CRC 不匹配"}}
        });

        // Bit 5: MMEF (发生模式错误标志, RC_W0)
        m_fields.push_back({
            "MMEF (Mode Error Occur Flag)", 5, 1,
            {{0, "0: 未发生"}, {1, "1: 发生"}}
        });

        // Bit 6: RXOF (发生过载标志, RC_W0)
        m_fields.push_back({
            "RXOF (Overrun Occur Flag)", 6, 1,
            {{0, "0: 未发生"}, {1, "1: 发生"}}
        });

        // Bit 7: BUSYF (SPI 忙标志, R)
        m_fields.push_back({
            "BUSYF (SPI Busy Flag)", 7, 1,
            {{0, "0: SPI 空闲"}, {1, "1: SPI 正在通信"}}
        });
        set_read_callback({
        });
        // 忽略 2, 31:8 的保留位
    }

    // // 对于 RC_W0 字段，需要重写 write 方法来模拟“写 0 清零”的行为
    // // 假设基类 Register 支持 set_current_value/get_current_value
    // void write(uint32_t new_value) override {
    //     uint32_t current_value = m_current_value;
    //     uint32_t sticky_bits_mask = (1U << 3) | (1U << 4) | (1U << 5) | (1U << 6);
    //
    //     // 保留原状态位，只清除新值中为 0 的位 (RC_W0)
    //     uint32_t new_sticky_bits = current_value & (~new_value) & sticky_bits_mask;
    //
    //     // 非 RC_W0 位的写入直接生效 (R/W, R 字段在这里应该没有)
    //     uint32_t non_sticky_bits = new_value & (~sticky_bits_mask);
    //
    //     uint32_t final_value = non_sticky_bits | new_sticky_bits;
    //
    //     Register::write(final_value);
    // }
};

// =========================================================================
// SPI_DATA (SPI 数据寄存器)
// 偏移地址：0x10，复位值：0x0000 0000
// =========================================================================
class SPI_DATA_Register : public Register {
public:
    explicit SPI_DATA_Register(uint32_t offset)
        : Register(offset, "SPI_DATA (SPI 数据寄存器)", 0x00000000) {
        // Bits 7:0: DATA (发送接收数据寄存器)
        m_fields.push_back({
            "DATA (Transmit/Receive Data Register)", 0, 8,
            {} // R/W 字段，直接显示值
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// SPI_CRCPOLY (SPI CRC 多项式寄存器)
// 偏移地址：0x14，复位值：0x0000 0007
// =========================================================================
class SPI_CRCPOLY_Register : public Register {
public:
    explicit SPI_CRCPOLY_Register(uint32_t offset)
        : Register(offset, "SPI_CRCPOLY (SPI CRC 多项式寄存器)", 0x00000007) {
        // Bits 7:0: CRCPOLY (设置 CRC 多项式数值)
        m_fields.push_back({
            "CRCPOLY (CRC Polynomial Value Setup)", 0, 8,
            {} // R/W 字段，直接显示值
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// SPI_RXCRC (SPI 接收 CRC 寄存器)
// 偏移地址：0x18，复位值：0x0000 0000
// =========================================================================
class SPI_RXCRC_Register : public Register {
public:
    explicit SPI_RXCRC_Register(uint32_t offset)
        : Register(offset, "SPI_RXCRC (SPI 接收 CRC 寄存器)", 0x00000000) {
        // Bits 7:0: RXCRC (接收数据的 CRC 数值)
        m_fields.push_back({
            "RXCRC (Receive Data CRC Value)", 0, 8,
            {} // 只读字段
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// SPI_TXCRC (SPI 发送 CRC 寄存器)
// 偏移地址：0x1C，复位值：0x0000 0000
// =========================================================================
class SPI_TXCRC_Register : public Register {
public:
    explicit SPI_TXCRC_Register(uint32_t offset)
        : Register(offset, "SPI_TXCRC (SPI 发送 CRC 寄存器)", 0x00000000) {
        // Bits 7:0: TXCRC (发送数据的 CRC 数值)
        m_fields.push_back({
            "TXCRC (Transmit Data CRC Value)", 0, 8,
            {} // 只读字段
        });

        // 忽略 31:8 的保留位
    }
};

} // namespace SpiRegisters