#pragma once
#include "Register.h"
namespace Rcm {

// =========================================================================
// 外部时钟控制寄存器（RCM_ECC）
//偏移地址：0x04 复位值：0x0000 0000
// =========================================================================
class RCM_ECC_Register: public Register {
  public:
    // 偏移地址 0x04，复位值 0x00000000
    explicit RCM_ECC_Register(uint32_t offset)
      : Register(offset, "RCM_ECC (外部时钟控制)") {
      // 寄存器复位值
      m_current_value = 0x00000000;

      // Bit 0: HXTEN
      m_fields.push_back({
        "HXTEN (外部高速振荡器使能)", 0, 1,
        {{0, "0: 关闭 HXT"}, {1, "1: 打开 HXT"}}
      });

      // Bit 1: HXTRF (Read-only status, we show what the value means)
      m_fields.push_back({
        "HXTRF (HXT 准备就绪标志)", 1, 1,
        {{0, "0: HXT 未准备就绪"}, {1, "1: HXT 准备就绪"}}
      });

      // 忽略 31:2 的保留位
    }
};
// =========================================================================
// RCM_ICC (内部时钟控制寄存器)
// 偏移地址：0x00 ，复位值：0x0000 0001
// =========================================================================
class RCM_ICC_Register: public Register {
  public:
    explicit RCM_ICC_Register(uint32_t offset) : Register(offset, "RCM_ICC (内部时钟控制)") {
      // 初始化位域定义

      // Bit 0: HIRCEN
      m_fields.push_back({
        "HIRCEN (内部高速时钟使能)", 0, 1,
        {{0, "0: 关闭内部高速时钟"}, {1, "1: 打开内部高速时钟"}}
      });

      // Bit 1: HIRCRF (Read-only status, but we show what the value means)
      m_fields.push_back({
        "HIRCRF (HIRC 准备就绪标志)", 1, 1,
        {{0, "0: HIRC 未准备就绪"}, {1, "1: HIRC 准备就绪"}}
      });

      // Bit 2: FWFLHEN
      m_fields.push_back({
        "FWFLHEN (快速唤醒使能)", 2, 1,
        {{0, "0: 禁止快速唤醒"}, {1, "1: 使能快速唤醒"}}
      });

      // Bit 3: LIRCEN
      m_fields.push_back({
        "LIRCEN (内部低速振荡器使能)", 3, 1,
        {{0, "0: 关闭 LIRC"}, {1, "1: 打开 LIRC"}}
      });

      // Bit 4: LIRCRF (Read-only status)
      m_fields.push_back({
        "LIRCRF (LIRC 准备就绪标志)", 4, 1,
        {{0, "0: LIRC 未准备就绪"}, {1, "1: LIRC 准备就绪"}}
      });

      // Bit 5: RPOEN
      m_fields.push_back({
        "RPOEN (电压调节器电源关闭使能)", 5, 1,
        {{0, "0: 在 Active-halt 模式下打开电压调节器"}, {1, "1: 在 Active-halt 模式下关闭电压调节器"}}
      });

      // 忽略 31:6 的保留位，只列出有意义的控制位
    }
};
// =========================================================================
// RCM_MCS (主时钟状态寄存器)
// 偏移地址 0x0C，复位值 0x0000 00E1
// =========================================================================
class RCM_MCS_Register: public Register {
  public:
    explicit RCM_MCS_Register(uint32_t offset)
      : Register(offset, "RCM_MCS (主时钟状态)") {
      // 寄存器复位值
      m_current_value = 0x000000E1;

      // --- 初始化位域定义 (根据提供的图表) ---

      // 位域 7:0: MCS (主时钟状态)
      m_fields.push_back({
        "MCS (主时钟状态)", 0, 8, // 从 Bit 0 开始，宽度为 8 位
        {
          {0xE1, "0XE1: 表明 HIRC 是主时钟源"},
          {0xD2, "0XD2: 表明 LIRC 是主时钟源"},
          {0xB4, "0XB4: 表明 HXT 是主时钟源"}
          // 其他值将使用默认的 "写入值: X" 显示
        }
      });

      // 忽略 31:8 的保留位
    }
};


// =========================================================================
// RCM_MCC (主时钟配置寄存器)
// 偏移地址 0x10，复位值 0x0000 00E1
// =========================================================================

class RCM_MCC_Register: public Register {
  public:
    explicit RCM_MCC_Register(uint32_t offset)
      : Register(offset, "RCM_MCC (主时钟配置)") {
      // 寄存器复位值
      m_current_value = 0x000000E1;

      // --- 初始化位域定义 (根据提供的图表) ---

      // 位域 7:0: MCC (主时钟配置)
      m_fields.push_back({
        "MCC (主时钟配置)", 0, 8, // 从 Bit 0 开始，宽度为 8 位
        {
          {0xE1, "0XE1: 配置 HIRC 是主时钟源"},
          {0xD2, "0XD2: 配置 LIRC 是主时钟源"},
          {0xB4, "0XB4: 配置 HXT 是主时钟源"}
        }
      });

      // 忽略 31:8 的保留位
    }
};
// =========================================================================
// RCM_CSC (时钟切换控制寄存器)
// 偏移地址 0x14，复位值 0x0000 00XX
// =========================================================================

class RCM_CSC_Register: public Register {
  public:
    explicit RCM_CSC_Register(uint32_t offset)
      : Register(offset, "RCM_CSC (时钟切换控制)") {
      // 寄存器复位值: 0x0000 00XX。由于XX表示可能变化或未定义，
      // 我们将其初始化为 0x00，或者依赖于软件在启动时初始化。
      // 这里使用 0x00，若需要特定值，请调整。
      m_current_value = 0x00000000;

      // --- 初始化位域定义 (根据提供的图表) ---

      // Bit 0: CSBF (时钟切换忙标志)
      m_fields.push_back({
        "CSBF (时钟切换忙标志)", 0, 1,
        {{0, "0: 未进行时钟切换"}, {1, "1: 正在进行时钟切换"}}
      });

      // Bit 1: CSEN (时钟切换使能)
      m_fields.push_back({
        "CSEN (Clock Switch Enable)", 1, 1,
        {{0, "0: 禁止时钟切换"}, {1, "1: 使能时钟切换"}}
      });

      // Bit 2: CSIE (时钟切换中断使能)
      m_fields.push_back({
        "CSIE (Clock Switch Interrupt Enable)", 2, 1,
        {{0, "0: 禁止时钟切换中断"}, {1, "1: 使能时钟切换中断"}}
      });

      // Bit 3: CSIF (时钟切换中断标志位)
      m_fields.push_back({
        "CSIF (Clock Switch Interrupt Flag)", 3, 1,
        {
          // 注意：CSIF 的含义取决于 CSEN 的状态 (手动/自动切换)
          {0, "0: (手动CSEN=0) 目标时钟还未稳定 / (自动CSEN=1) 没有发生时钟切换事件"},
          {1, "1: (手动CSEN=0) 目标时钟已经稳定 / (自动CSEN=1) 发生时钟切换事件"}
        }
      });

      // 忽略 31:4 的保留位
    }
};


// =========================================================================
// RCM_CLKDIV (时钟预分频寄存器)
// 偏移地址 0x18，复位值 0x0000 0018
// 注意: 0x18 对应二进制 011000
// 011 (HIRC Divider) = 8
// 000 (CPU Divider) = 2
// 0 (HDS) = HIRC/3
// =========================================================================

class RCM_CLKDIV_Register: public Register {
  public:
    explicit RCM_CLKDIV_Register(uint32_t offset)
      : Register(offset, "RCM_CLKDIV (时钟预分频)") {
      // 寄存器复位值
      m_current_value = 0x00000018;

      // --- 初始化位域定义 (根据提供的图表) ---

      // 位域 2:0: CPUDIV (CPU 时钟分频系数)
      m_fields.push_back({
        "CPUDIV (CPU Clock Divider Factor)", 0, 3,
        {
          {0b000, "000: / 2"}, {0b001, "001: / 2"},
          {0b010, "010: / 4"}, {0b011, "011: / 8"},
          {0b100, "100: / 16"}, {0b101, "101: / 32"},
          {0b110, "110: / 64"}, {0b111, "111: / 128"}
        }
      });

      // 位域 4:3: HIRCDIV (内部高速时钟分频系数)
      m_fields.push_back({
        "HIRCDIV (HIRC Clock Divider Factor)", 3, 2,
        {
          {0b00, "00: / 1"}, {0b01, "01: / 2"},
          {0b10, "10: / 4"}, {0b11, "11: / 8"}
        }
      });

      // Bit 5: HDS (内部高速分频时钟设置)
      m_fields.push_back({
        "HDS (HIRC Divider Set)", 5, 1,
        {{0, "0: HIRC/3"}, {1, "1: HIRC"}}
      });

      // 忽略 31:6 的保留位
    }
};
// =========================================================================
// RCM_APBEN1 (APB 时钟使能寄存器 1)
// 偏移地址 0x1C，复位值 0x0000 00FF
// =========================================================================

class RCM_APBEN1_Register: public Register {
  public:
    // 构造函数使用 explicit 关键字
    explicit RCM_APBEN1_Register(uint32_t offset)
      : Register(offset, "RCM_APBEN1 (APB 时钟使能 1)") {
      m_current_value = 0x000000FF;

      // Bit 0: I2CCEN
      m_fields.push_back({
        "I2CCEN (I2C Clock Enable)", 0, 1,
        {{0, "0: 禁止 I2C 时钟"}, {1, "1: 使能 I2C 时钟"}}
      });

      // Bit 1: SPICEN
      m_fields.push_back({
        "SPICEN (SPI Clock Enable)", 1, 1,
        {{0, "0: 禁止 SPI 时钟"}, {1, "1: 使能 SPI 时钟"}}
      });

      // Bit 2: 保留 (忽略)

      // Bit 3: USART1CEN
      m_fields.push_back({
        "USART1CEN (USART1 Clock Enable)", 3, 1,
        {{0, "0: 禁止 USART1 时钟"}, {1, "1: 使能 USART1 时钟"}}
      });

      // Bit 4: TMR4CEN
      m_fields.push_back({
        "TMR4CEN (TMR4 Clock Enable)", 4, 1,
        {{0, "0: 禁止 TMR4 时钟"}, {1, "1: 使能 TMR4 时钟"}}
      });

      // Bit 5: TMR2CEN
      m_fields.push_back({
        "TMR2CEN (TMR2 Clock Enable)", 5, 1,
        {{0, "0: 禁止 TMR2 时钟"}, {1, "1: 使能 TMR2 时钟"}}
      });

      // Bit 7: TMR1CEN
      m_fields.push_back({
        "TMR1CEN (TMR1 Clock Enable)", 7, 1,
        {{0, "0: 禁止 TMR1 时钟"}, {1, "1: 使能 TMR1 时钟"}}
      });

      // 忽略 6 和 31:8 的保留位
    }
};
// =========================================================================
// RCM_CSS (时钟保护系统寄存器)
// 偏移地址 0x20，复位值 0x0000 0000
// =========================================================================

class RCM_CSS_Register: public Register {
  public:
    // 构造函数使用 explicit 关键字
    explicit RCM_CSS_Register(uint32_t offset)
      : Register(offset, "RCM_CSS (时钟保护系统)") {
      m_current_value = 0x00000000;

      // Bit 0: CSSEN (时钟保护系统使能)
      m_fields.push_back({
        "CSSEN (Clock Security System Enable)", 0, 1,
        {{0, "0: 禁止 CSS"}, {1, "1: 使能 CSS"}}
      });

      // Bit 1: BCEN (备用时钟源使能)
      m_fields.push_back({
        "BCEN (Backup Clock Enable)", 1, 1,
        {{0, "0: 禁止备用时钟"}, {1, "1: 使能备用时钟"}}
      });

      // Bit 2: CSSFDIE (CSS 故障检测中断使能)
      m_fields.push_back({
        "CSSFDIE (CSS Fault Detect Interrupt Enable)", 2, 1,
        {{0, "0: 禁止 CSS 故障检测中断"}, {1, "1: 使能 CSS 故障检测中断"}}
      });

      // Bit 3: CSSFDIF (CSS 故障检测中断标志)
      m_fields.push_back({
        "CSSFDIF (CSS Fault Detect Interrupt Flag)", 3, 1,
        {{0, "0: 未检测到 HXT 时钟故障"}, {1, "1: 检测到 HXT 时钟故障"}}
      });

      // 忽略 31:4 的保留位
    }
};
// =========================================================================
// RCM_COC (时钟输出控制寄存器)
// 偏移地址 0x24，复位值 0x0000 0000
// =========================================================================

class RCM_COC_Register: public Register {
  public:
    // 构造函数使用 explicit 关键字
    explicit RCM_COC_Register(uint32_t offset)
      : Register(offset, "RCM_COC (时钟输出控制)") {
      m_current_value = 0x00000000;

      // Bit 0: COEN (时钟输出使能)
      m_fields.push_back({
        "COEN (Clock Output Enable)", 0, 1,
        {{0, "0: 禁止"}, {1, "1: 使能"}}
      });

      // 位域 4:1: COS (时钟输出源选择)
      m_fields.push_back({
        "COS (Clock Output Select)", 1, 4,
        {
          {0b0000, "0000: fHIRCDIV"},
          {0b0001, "0001: fLIRC"},
          {0b0010, "0010: fHXT"},
          {0b0011, "0011: 保留"}, // 0x3
          {0b0100, "0100: fCPU"},
          {0b0101, "0101: fCPU/2"},
          {0b0110, "0110: fCPU/4"},
          {0b0111, "0111: fCPU/8"},
          {0b1000, "1000: fCPU/16"},
          {0b1001, "1001: fCPU/32"},
          {0b1010, "1010: fCPU/64"},
          {0b1011, "1011: fHIRC"},
          {0b1100, "1100: fMASTER"},
          {0b1101, "1101: fCPU"},
          {0b1110, "1110: fCPU"},
          {0b1111, "1111: fCPU"}
        }
      });

      // Bit 5: CORF (时钟输出准备就绪标志)
      m_fields.push_back({
        "CORF (Clock Output Ready Flag)", 5, 1,
        {{0, "0: 时钟输出已经准备就绪"}, {1, "1: 时钟输出未准备就绪"}}
      });

      // Bit 6: COBF (时钟输出忙标志)
      m_fields.push_back({
        "COBF (Clock Output Busy Flag)", 6, 1,
        {{0, "0: 时钟输出所选时钟源空闲"}, {1, "1: 时钟输出所选时钟源忙"}}
      });

      // 忽略 31:7 的保留位
    }
};
// =========================================================================
// RCM_APBEN2 (APB 时钟使能寄存器 2)
// 偏移地址 0x28，复位值 0x0000 00FF
// =========================================================================

class RCM_APBEN2_Register : public Register {
  public:
    // 构造函数使用 explicit 关键字
    explicit RCM_APBEN2_Register(uint32_t offset)
        : Register(offset, "RCM_APBEN2 (APB 时钟使能 2)")
    {
      // 寄存器复位值
      m_current_value = 0x000000FF;

      // --- 初始化位域定义 (根据提供的图表) ---

      // 位域 1:0: 保留 (无需定义 BitField)

      // Bit 2: WUPTCEN (唤醒定时器时钟使能)
      m_fields.push_back({
          "WUPTCEN (Wakeup TMR Clock Enable)", 2, 1,
          {{0, "0: 禁止唤醒定时器时钟"}, {1, "1: 使能唤醒定时器时钟"}}
      });

      // Bit 3: ADCCEN (ADC 时钟使能)
      m_fields.push_back({
          "ADCCEN (ADC Clock Enable)", 3, 1,
          {{0, "0: 禁止 ADC 时钟"}, {1, "1: 使能 ADC 时钟"}}
      });

      // 位域 31:4: 保留 (无需定义 BitField)
    }
};
// =========================================================================
// RCM_HIRCTRIM (内部高速时钟调整寄存器)
// 偏移地址 0x30，复位值 0x0000 0000
// =========================================================================

class RCM_HIRCTRIM_Register : public Register {
public:
    // 构造函数使用 explicit 关键字
    explicit RCM_HIRCTRIM_Register(uint32_t offset)
        : Register(offset, "RCM_HIRCTRIM (HIRC 调整)")
    {
        m_current_value = 0x00000000;

        // 位域 3:0: TRIM (HIRC 调整值)
        // 这是一个 R/W 字段，用于设置值，无需特定的描述映射。
        m_fields.push_back({
            "TRIM (HIRC 调整值)", 0, 4,
            {} // 不需要值到描述的映射，直接显示写入的数值
        });

        // 忽略 31:4 的保留位
    }
};

// =========================================================================
// RCM_RSTSTS (复位状态寄存器)
// 偏移地址 0x38，复位值 0x0000 00XX
// 注意：复位标志通常是 RC_W1 类型，即读为 1，写 1 清零。
// 在我们的模拟器中，我们只关注读取时的状态解析。
// =========================================================================

class RCM_RSTSTS_Register : public Register {
public:
    // 构造函数使用 explicit 关键字
    explicit RCM_RSTSTS_Register(uint32_t offset)
        : Register(offset, "RCM_RSTSTS (复位状态)")
    {
        // 寄存器复位值 0x0000 00XX，由于是标志寄存器，我们通常初始化为 0x00
        m_current_value = 0x00000000;

        // --- 初始化位域定义 (根据提供的图表) ---

        // Bit 0: WWDTRF (窗口看门狗定时器复位标志)
        m_fields.push_back({
            "WWDTRF (WWDT Reset Flag)", 0, 1,
            {{0, "0: 无复位发生"}, {1, "1: 发生复位"}}
        });

        // Bit 1: IWDTRF (独立看门狗定时器复位标志)
        m_fields.push_back({
            "IWDTRF (IWDT Reset Flag)", 1, 1,
            {{0, "0: 无复位发生"}, {1, "1: 发生复位"}}
        });

        // Bit 2: CPURF (CPU 软件复位标志)
        m_fields.push_back({
            "CPURF (CPU Software Reset Flag)", 2, 1,
            {{0, "0: 无 CPU 软件复位发生"}, {1, "1: 发生 CPU 软件复位"}}
        });

        // Bit 3: 保留 (忽略)

        // Bit 4: EMCRF (EMC 复位标志)
        m_fields.push_back({
            "EMCRF (EMC Reset Flag)", 4, 1,
            {{0, "0: 无复位发生"}, {1, "1: 发生复位"}}
        });

        // 忽略 31:5 的保留位
    }

    // 额外说明：RC_W1 字段（如这些标志）在实际硬件中写入 1 会清零，
    // 在模拟器中，您可能需要重写 write 方法来模拟这个行为。
};


// =========================================================================
// RCM_APBEN3 (APB 时钟使能寄存器 3)
// 偏移地址 0x3C，复位值 0x0000 0007
// =========================================================================

class RCM_APBEN3_Register : public Register {
public:
    // 构造函数使用 explicit 关键字
    explicit RCM_APBEN3_Register(uint32_t offset)
        : Register(offset, "RCM_APBEN3 (APB 时钟使能 3)")
    {
        m_current_value = 0x00000007;

        // --- 初始化位域定义 (根据提供的图表) ---

        // Bit 0: TMR1ACEN
        m_fields.push_back({
            "TMR1ACEN (TMR1A Clock Enable)", 0, 1,
            {{0, "0: 禁止 TMR1A 时钟"}, {1, "1: 使能 TMR1A 时钟"}}
        });

        // Bit 1: USART2CEN
        m_fields.push_back({
            "USART2CEN (USART2 Clock Enable)", 1, 1,
            {{0, "0: 禁止 USART2 时钟"}, {1, "1: 使能 USART2 时钟"}}
        });

        // Bit 2: USART3CEN
        m_fields.push_back({
            "USART3CEN (USART3 Clock Enable)", 2, 1,
            {{0, "0: 禁止 USART3 时钟"}, {1, "1: 使能 USART3 时钟"}}
        });

        // 忽略 31:3 的保留位
    }
};
}
