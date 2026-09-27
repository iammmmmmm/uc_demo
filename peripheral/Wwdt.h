#pragma once
#include "peripheral_factory.h"
#include <map>
#include <memory>
#include <string>

// ----------------------------------------------------
// 1. WWDT 基础地址和范围
// ----------------------------------------------------
#define WWDT_BASE         ((uint32_t)0x40001C00)
#define WWDT_END          ((uint32_t)0x40001FFF)

// ----------------------------------------------------
// 2. WWDT 寄存器偏移地址
// ----------------------------------------------------
#define WWDT_CTRL_OFFSET     ((uint32_t)0x00) // 控制寄存器
#define WWDT_WDDATA_OFFSET   ((uint32_t)0x04) // 窗口看门狗数据寄存器


class Register;
class Wwdt : public PeripheralDevice {
  private:
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;
    void initialize_registers();

  public:
    Wwdt();
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return WWDT_BASE; }
    std::string getName() override { return "WWDT"; }
};