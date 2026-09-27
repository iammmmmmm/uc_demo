#pragma once
#include "Register.h"

// 将 I2C 寄存器的实现放在一个命名空间内，以保持代码组织性
namespace I2cRegisters {

// =========================================================================
// I2C_CTRL1 (控制寄存器 1)
// 偏移地址：0x00，复位值：0x0000 0000
// =========================================================================
class I2C_CTRL1_Register : public Register {
public:
    explicit I2C_CTRL1_Register(uint32_t offset)
        : Register(offset, "I2C_CTRL1 (控制寄存器 1)", 0x00000000) {
        // Bit 0: I2CEN (使能 I2C)
        m_fields.push_back({
            "I2CEN (I2C Enable)", 0, 1,
            {{0, "0: 禁止"}, {1, "1: 使能"}}
        });

        // Bit 6: BCEN (使能从机响应广播)
        m_fields.push_back({
            "BCEN (Slave Responds Broadcast Enable)", 6, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (广播地址 0x00)"}}
        });

        // Bit 7: STRDIS (禁止从机模式时钟延长时间)
        m_fields.push_back({
            "STRDIS (Slave Mode Clock Stretching Disable)", 7, 1,
            {{0, "0: 使能 (允许延长时钟低电平时间)"}, {1, "1: 禁止"}}
        });

        // 忽略 5:1 和 31:8 的保留位
    }
};

// =========================================================================
// I2C_CTRL2 (控制寄存器 2)
// 偏移地址：0x04，复位值：0x0000 0000
// =========================================================================
class I2C_CTRL2_Register : public Register {
public:
    explicit I2C_CTRL2_Register(uint32_t offset)
        : Register(offset, "I2C_CTRL2 (控制寄存器 2)", 0x00000000) {
        // Bit 0: START (发送起始位)
        // 硬件清零逻辑需要外部模拟器或回调函数实现，这里只定义位域
        m_fields.push_back({
            "START (Start Bit Transfer)", 0, 1,
            {{0, "0: 不发送"}, {1, "1: 发送 (软件置 1, 硬件清 0)"}}
        });

        // Bit 1: STOP (发送停止位)
        m_fields.push_back({
            "STOP (Stop Bit Transfer)", 1, 1,
            {{0, "0: 不发送"}, {1, "1: 发送 (软件置 1, 硬件清 0/置 1)"}}
        });

        // Bit 2: ACKEN (发送应答使能)
        m_fields.push_back({
            "ACKEN (Acknowledge Transfer Enable)", 2, 1,
            {{0, "0: 不发送"}, {1, "1: 发送 (软件置 1/清 0)"}}
        });

        // Bit 3: ACKPOS (配置接收数据应答位置)
        m_fields.push_back({
            "ACKPOS (Acknowledge Position Configure)", 3, 1,
            {{0, "0: 接收当前字节时发送 NACK/ACK"}, {1, "1: 接收下一个字节时发送 NACK/ACK"}}
        });

        // Bit 7: SWRST (软件配置 I2C 处于软件复位状态)
        m_fields.push_back({
            "SWRST (Software Reset State)", 7, 1,
            {{0, "0: 未复位"}, {1, "1: 复位"}}
        });

        // 忽略 6:4 和 31:8 的保留位
    }
};

// =========================================================================
// I2C_CLKFREQ (时钟频率寄存器)
// 偏移地址：0x08，复位值：0x0000 0000
// =========================================================================
class I2C_CLKFREQ_Register : public Register {
public:
    explicit I2C_CLKFREQ_Register(uint32_t offset)
        : Register(offset, "I2C_CLKFREQ (时钟频率寄存器)", 0x00000000) {
        // 位域 5:0: FREQ (配置 I2C 时钟频率)
        // 注意：这里只列出部分描述，实际应包含 0MHz 到 48MHz 的所有配置
        m_fields.push_back({
            "FREQ (I2C Clock Frequency Configure)", 0, 6,
            {
                {0b000000, "000000: 禁用"},
                {0b000001, "000001: 1MHz"},
                {0b000010, "000010: 2MHz"},
                {0b011000, "011000: 24MHz"},
                {0b110000, "110000: 48MHz"}
                // 其他值直接显示数值
            }
        });

        // 忽略 31:6 的保留位
    }
};

// =========================================================================
// I2C_ADDR0 (从机地址寄存器 0)
// 偏移地址：0x0C，复位值：0x0000 0000
// =========================================================================
class I2C_ADDR0_Register : public Register {
public:
    explicit I2C_ADDR0_Register(uint32_t offset)
        : Register(offset, "I2C_ADDR0 (从机地址寄存器 0)", 0x00000000) {
        // Bit 0: ADDR[0] (地址模式为 10 位时的第 0 位)
        m_fields.push_back({
            "ADDR[0] (Slave Address Bit 0)", 0, 1,
            {} // R/W 字段，直接显示值
        });

        // 位域 7:1: ADDR[7:1] (从机地址的第 7:1 位)
        m_fields.push_back({
            "ADDR[7:1] (Slave Address Bit 7:1)", 1, 7,
            {} // R/W 字段，直接显示值
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// I2C_ADDR1 (从机地址寄存器 1)
// 偏移地址：0x10，复位值：0x0000 0000
// =========================================================================
class I2C_ADDR1_Register : public Register {
public:
    explicit I2C_ADDR1_Register(uint32_t offset)
        : Register(offset, "I2C_ADDR1 (从机地址寄存器 1)", 0x00000000) {
        // 位域 2:1: ADDR[9:8] (地址模式为 10 位时的第 9:8 位)
        m_fields.push_back({
            "ADDR[9:8] (Slave Address Bit 9:8)", 1, 2,
            {} // R/W 字段，直接显示值
        });

        // Bit 6: ADDRCFG (配置地址模式)
        m_fields.push_back({
            "ADDRCFG (Address Mode Configure)", 6, 1,
            {{0, "0: 未配置"}, {1, "1: 已配置"}}
        });

        // Bit 7: ADDRMODE (配置从机地址模式)
        m_fields.push_back({
            "ADDRMODE (Slave Address Mode Configure)", 7, 1,
            {{0, "0: 7 位地址模式"}, {1, "1: 10 位地址模式"}}
        });

        // 忽略 0, 5:3 和 31:8 的保留位
    }
};

// =========================================================================
// I2C_DATA (数据寄存器)
// 偏移地址：0x18，复位值：0x0000 0000
// =========================================================================
class I2C_DATA_Register : public Register {
public:
    explicit I2C_DATA_Register(uint32_t offset)
        : Register(offset, "I2C_DATA (数据寄存器)", 0x00000000) {
        // 位域 7:0: DATA (数据寄存器)
        m_fields.push_back({
            "DATA (Data Register)", 0, 8,
            {} // R/W 字段，直接显示值
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// I2C_STS1 (状态寄存器 1)
// 偏移地址：0x1C，复位值：0x0000 0000
// 注意：多数为硬件置 1，软件清除标志，这里仅定义读取时的状态描述。
// =========================================================================
class I2C_STS1_Register : public Register {
public:
    explicit I2C_STS1_Register(uint32_t offset)
        : Register(offset, "I2C_STS1 (状态寄存器 1)", 0x00000000) {
        // Bit 0: SBTCF (发送起始位完成标志)
        m_fields.push_back({
            "SBTCF (Start Bit Sent Finished Flag)", 0, 1,
            {{0, "0: 未发送"}, {1, "1: 已发送"}}
        });

        // Bit 1: ADDRF (地址发送完成/接收匹配标志)
        m_fields.push_back({
            "ADDRF (Address Transfer Complete/Receive Match Flag)", 1, 1,
            {{0, "0: 未完成/未接收到匹配地址"}, {1, "1: 已完成/已接收到匹配地址"}}
        });

        // Bit 2: BTCF (完成数据字节传输标志)
        m_fields.push_back({
            "BTCF (Byte Transfer Complete Flag)", 2, 1,
            {{0, "0: 未完成"}, {1, "1: 已完成"}}
        });

        // Bit 3: ADDR10F (主机已发送 10 位地址的地址头标志)
        m_fields.push_back({
            "ADDR10F (10-Bit Address Header Sent Flag)", 3, 1,
            {{0, "0: 未发送"}, {1, "1: 已发送"}}
        });

        // Bit 4: SBDF (停止位检测标志)
        m_fields.push_back({
            "SBDF (Stop Bit Detection Flag)", 4, 1,
            {{0, "0: 未检测到"}, {1, "1: 检测到"}}
        });

        // Bit 6: RXBNEF (接收缓冲器不为空标志)
        m_fields.push_back({
            "RXBNEF (Receive Buffer Not Empty Flag)", 6, 1,
            {{0, "0: 接收缓冲器为空"}, {1, "1: 接收缓冲器不为空"}}
        });

        // Bit 7: TXBEF (发送缓冲器为空标志)
        m_fields.push_back({
            "TXBEF (Transmit Buffer Empty Flag)", 7, 1,
            {{0, "0: 发送缓冲器不为空"}, {1, "1: 发送缓冲器为空"}}
        });

        // 忽略 5 和 31:8 的保留位
    }
};

// =========================================================================
// I2C_STS2 (状态寄存器 2)
// 偏移地址：0x20，复位值：0x0000 0000
// 注意：这些标志是 RC_W0 类型（读为当前状态，写 0 清零），这里仅定义读取时的状态描述。
// =========================================================================
class I2C_STS2_Register : public Register {
public:
    explicit I2C_STS2_Register(uint32_t offset)
        : Register(offset, "I2C_STS2 (状态寄存器 2)", 0x00000000) {
        // Bit 0: BEF (总线错误标志)
        m_fields.push_back({
            "BEF (Bus Error Flag)", 0, 1,
            {{0, "0: 未发生总线错误"}, {1, "1: 发生总线错误"}}
        });

        // Bit 1: ALF (主模式下的仲裁丢失标志)
        m_fields.push_back({
            "ALF (Arbitration Lost Flag)", 1, 1,
            {{0, "0: 未发生仲裁丢失"}, {1, "1: 发生仲裁丢失 (切换回从模式)"}}
        });

        // Bit 2: AEF (应答错误标志)
        m_fields.push_back({
            "AEF (Acknowledge Error Flag)", 2, 1,
            {{0, "0: 未发生应答错误"}, {1, "1: 发生应答错误"}}
        });

        // Bit 3: OUF (发生过载或欠载标志)
        m_fields.push_back({
            "OUF (Overrun/Underrun Flag)", 3, 1,
            {{0, "0: 未发生"}, {1, "1: 发生"}}
        });

        // Bit 5: WFHF (从停机模式唤醒标志)
        m_fields.push_back({
            "WFHF (Halt Mode Wakeup Flag)", 5, 1,
            {{0, "0: 没有从停机模式唤醒"}, {1, "1: 从停机模式换唤醒"}}
        });

        // 忽略 4 和 31:6 的保留位
    }
};

// =========================================================================
// I2C_STS3 (状态寄存器 3)
// 偏移地址：0x24，复位值：0x0000 0000
// =========================================================================
class I2C_STS3_Register : public Register {
public:
    explicit I2C_STS3_Register(uint32_t offset)
        : Register(offset, "I2C_STS3 (状态寄存器 3)", 0x00000000) {
        // Bit 0: MMF (主从模式标志)
        m_fields.push_back({
            "MMF (Master Slave Mode Flag)", 0, 1,
            {{0, "0: 从机模式"}, {1, "1: 主机模式"}}
        });

        // Bit 1: BUSYF (总线忙碌标志)
        m_fields.push_back({
            "BUSYF (Bus Busy Flag)", 1, 1,
            {{0, "0: 总线空闲 (无通信)"}, {1, "1: 总线忙 (正在通信)"}}
        });

        // Bit 2: RWMF (发送器模式/接收器模式标志)
        m_fields.push_back({
            "RWMF (Transmitter/Receiver Mode Flag)", 2, 1,
            {{0, "0: 设备是接收器模式 (读)"}, {1, "1: 设备是发送器模式 (写)"}}
        });

        // Bit 4: RBF (从模式接收到广播地址标志)
        m_fields.push_back({
            "RBF (Slave Mode Received General Call Address Flag)", 4, 1,
            {{0, "0: 未收到广播地址"}, {1, "1: 收到广播地址 (0x00)"}}
        });

        // 忽略 3 和 31:5 的保留位
    }
};

// =========================================================================
// I2C_INTCTRL (中断控制寄存器)
// 偏移地址：0x28，复位值：0x0000 0000
// =========================================================================
class I2C_INTCTRL_Register : public Register {
public:
    explicit I2C_INTCTRL_Register(uint32_t offset)
        : Register(offset, "I2C_INTCTRL (中断控制寄存器)", 0x00000000) {
        // Bit 0: ERRIE (使能出错中断)
        m_fields.push_back({
            "ERRIE (Error Interrupt Enable)", 0, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (OUF, AEF, ALF, BEF)"}}
        });

        // Bit 1: EVTIE (使能事件中断)
        m_fields.push_back({
            "EVTIE (Event Interrupt Enable)", 1, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (SBTCF, ADDRF, ADDR10F, SBDF, BTCF, WFHF)"}}
        });

        // Bit 2: BUFIE (使能缓冲器中断)
        m_fields.push_back({
            "BUFIE (Buffer Interrupt Enable)", 2, 1,
            {{0, "0: 禁止"}, {1, "1: 使能 (TXBEF, RXBNEF)"}}
        });

        // 忽略 31:3 的保留位
    }
};

// =========================================================================
// I2C_CLKCTRL1 (主机时钟控制寄存器 1)
// 偏移地址：0x2C，复位值：0x0000 0000
// =========================================================================
class I2C_CLKCTRL1_Register : public Register {
public:
    explicit I2C_CLKCTRL1_Register(uint32_t offset)
        : Register(offset, "I2C_CLKCTRL1 (主机时钟控制寄存器 1)", 0x00000000) {
        // 位域 7:0: CLKCTRL[7:0] (时钟控制器低 8 位)
        m_fields.push_back({
            "CLKCTRL[7:0] (Clock Setup Low 8 Bits)", 0, 8,
            {} // R/W 字段，直接显示值
        });

        // 忽略 31:8 的保留位
    }
};

// =========================================================================
// I2C_CLKCTRL2 (主机时钟控制寄存器 2)
// 偏移地址：0x30，复位值：0x0000 0000
// =========================================================================
class I2C_CLKCTRL2_Register : public Register {
public:
    explicit I2C_CLKCTRL2_Register(uint32_t offset)
        : Register(offset, "I2C_CLKCTRL2 (主机时钟控制寄存器 2)", 0x00000000) {
        // 位域 3:0: CLKCTRL[11:8] (时钟控制器高 4 位)
        m_fields.push_back({
            "CLKCTRL[11:8] (Clock Setup High 4 Bits)", 0, 4,
            {} // R/W 字段，直接显示值
        });

        // Bit 6: FMDC (配置快速模式下的占空比)
        m_fields.push_back({
            "FMDC (Fast Mode Duty Cycle Configure)", 6, 1,
            {{0, "0: SCLK 占空比 1/3"}, {1, "1: SCLK 占空比 9/25"}}
        });

        // Bit 7: FASTMODE (配置主模式速度)
        m_fields.push_back({
            "FASTMODE (Master Mode Speed Configure)", 7, 1,
            {{0, "0: 标准模式"}, {1, "1: 快速模式"}}
        });

        // 忽略 5:4 和 31:8 的保留位
    }
};

// =========================================================================
// I2C_MRT (上升时间寄存器)
// 偏移地址：0x34，复位值：0x0000 0002
// =========================================================================
class I2C_MRT_Register : public Register {
public:
    explicit I2C_MRT_Register(uint32_t offset)
        : Register(offset, "I2C_MRT (上升时间寄存器)", 0x00000002) {
        // 位域 5:0: MRT (最大上升时间)
        m_fields.push_back({
            "MRT (Maximum Rise Time)", 0, 6,
            {} // R/W 字段，直接显示值
        });

        // 忽略 31:6 的保留位
    }
};

} // namespace I2cRegisters