#pragma once

#include <map>
#include <memory>
#include "peripheral_factory.h"
#include "Register.h"

// ----------------------------------------------------
// 1. EINT 基础地址和范围
// ----------------------------------------------------
// EINT 模块的基地址
#define EINT_BASE   ((uint32_t)0x40001800)
// EINT 模块的结束地址
#define EINT_END    ((uint32_t)0x40001BFF)
// ----------------------------------------------------
// 2. EINT 寄存器地址偏移
// ----------------------------------------------------
// EINT_CTRL1 控制寄存器 1 的偏移地址 (0x00)
#define EINT_CTRL1_OFFSET       ((uint32_t)0x00)
// EINT_CTRL2 控制寄存器 2 的偏移地址 (0x04)
#define EINT_CTRL2_OFFSET       ((uint32_t)0x04)
// EINT_CLR 中断清除寄存器的偏移地址 (0x08)
#define EINT_CLR_OFFSET         ((uint32_t)0x08)
// ----------------------------------------------------
// 3. EINT 寄存器完整地址 (Absolute Addresses)
// ----------------------------------------------------
#define EINT_CTRL1_ADDR         (EINT_BASE + EINT_CTRL1_OFFSET)
#define EINT_CTRL2_ADDR         (EINT_BASE + EINT_CTRL2_OFFSET)
#define EINT_CLR_ADDR           (EINT_BASE + EINT_CLR_OFFSET)


class EINT: public PeripheralDevice {
  private:
    // 存储 EINT 寄存器状态，Key: 偏移地址
    std::map<uint32_t, std::unique_ptr<Register> > m_registers;

    // 初始化寄存器复位值
    void initialize_registers();

  public:
    // 构造函数
    EINT();

    // PeripheralRegistry 接口的实现
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return EINT_BASE; }
    std::string getName() override { return "Eint"; }
};