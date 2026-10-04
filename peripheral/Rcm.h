#pragma once
#include "peripheral_factory.h"
// ----------------------------------------------------
// 1. RCM (Clock Management Unit) 基础地址和范围
// ----------------------------------------------------
// RCM/CMU 模块的基地址
#define RCM_BASE            ((uint32_t)0x40010000)
// RCM/CMU 模块的结束地址
#define RCM_END             ((uint32_t)0x400103FF)
// ----------------------------------------------------
// 2. RCM 寄存器偏移地址 (Offset Addresses)
// ----------------------------------------------------
// 偏移地址用于在驱动程序中计算寄存器位置：
// 实际地址 = RCM_BASE + 偏移地址

#define RCM_ICC_OFFSET      ((uint32_t)0x00) // 内部时钟控制寄存器 (Internal Clock Control)
#define RCM_ECC_OFFSET      ((uint32_t)0x04) // 外部时钟控制寄存器 (External Clock Control)
// 注意：表格中 RCM_MCS (主时钟状态寄存器) 缺失 0x08 偏移地址，
// 0x08 可能是保留或未列出的寄存器。使用表格中给出的值。
#define RCM_MCS_OFFSET      ((uint32_t)0x0C) // 主时钟状态寄存器 (Main Clock Status)
#define RCM_MCC_OFFSET      ((uint32_t)0x10) // 主时钟配置寄存器 (Main Clock Configuration)
#define RCM_CSC_OFFSET      ((uint32_t)0x14) // 时钟切换控制寄存器 (Clock Switch Control)
#define RCM_CLKDIV_OFFSET   ((uint32_t)0x18) // 时钟预分频寄存器 (Clock Divider)
#define RCM_APBEN1_OFFSET   ((uint32_t)0x1C) // APB时钟使能寄存器1 (APB Clock Enable 1)
#define RCM_CSS_OFFSET      ((uint32_t)0x20) // 时钟保护系统寄存器 (Clock Security System)
#define RCM_COC_OFFSET      ((uint32_t)0x24) // 时钟输出控制寄存器 (Clock Output Control)
#define RCM_APBEN2_OFFSET   ((uint32_t)0x28) // APB时钟使能寄存器2 (APB Clock Enable 2)
// 0x2C 偏移缺失，可能是保留
#define RCM_HIRCTRIM_OFFSET ((uint32_t)0x30) // 内部高速时钟调整寄存器 (HIRC Trim)
// 0x34 偏移缺失，可能是保留
#define RCM_RSTSTS_OFFSET   ((uint32_t)0x38) // 复位状态寄存器 (Reset Status)
#define RCM_APBEN3_OFFSET   ((uint32_t)0x3C) // APB时钟使能寄存器3 (APB Clock Enable 3)
// ----------------------------------------------------
// 3. RCM 寄存器完整地址 (Absolute Addresses)
// ----------------------------------------------------
// 完整地址用于方便地引用和检查：
#define RCM_ICC_ADDR        (RCM_BASE + RCM_ICC_OFFSET)
#define RCM_ECC_ADDR        (RCM_BASE + RCM_ECC_OFFSET)
#define RCM_MCS_ADDR        (RCM_BASE + RCM_MCS_OFFSET)
#define RCM_MCC_ADDR        (RCM_BASE + RCM_MCC_OFFSET)
#define RCM_CSC_ADDR        (RCM_BASE + RCM_CSC_OFFSET)
#define RCM_CLKDIV_ADDR     (RCM_BASE + RCM_CLKDIV_OFFSET)
#define RCM_APBEN1_ADDR     (RCM_BASE + RCM_APBEN1_OFFSET)
#define RCM_CSS_ADDR        (RCM_BASE + RCM_CSS_OFFSET)
#define RCM_COC_ADDR        (RCM_BASE + RCM_COC_OFFSET)
#define RCM_APBEN2_ADDR     (RCM_BASE + RCM_APBEN2_OFFSET)
#define RCM_HIRCTRIM_ADDR   (RCM_BASE + RCM_HIRCTRIM_OFFSET)
#define RCM_RSTSTS_ADDR     (RCM_BASE + RCM_RSTSTS_OFFSET)
#define RCM_APBEN3_ADDR     (RCM_BASE + RCM_APBEN3_OFFSET)
// CMU (Clock Management Unit) 的基地址
#define CMU_BASE 0x40010000

class Register;
class RCM : public PeripheralDevice {
  private:
    // 使用 Map 存储寄存器状态，Key: 偏移地址，Value: 32位寄存器值
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;

    // 初始化寄存器复位值
    void initialize_registers();
  public:
    RCM();
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    void plantInitialValues(uc_engine *uc) override;
    uint64_t getBaseAddress() override { return RCM_BASE; }
    std::string getName() override { return "CMU (RCM)"; }
};
