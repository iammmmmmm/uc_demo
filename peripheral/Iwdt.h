#pragma once
#include "peripheral_factory.h"
#include <map>
#include <memory>
#include <string>

// ----------------------------------------------------
// 1. IWDT 基础地址和范围
// ----------------------------------------------------
#define IWDT_BASE         ((uint32_t)0x40002000)
#define IWDT_END          ((uint32_t)0x400023FF)

// ----------------------------------------------------
// 2. IWDT 寄存器偏移地址
// ----------------------------------------------------
#define IWDT_KEYWORD_OFFSET     ((uint32_t)0x00) // 关键字寄存器
#define IWDT_PSC_OFFSET         ((uint32_t)0x04) // 预分频寄存器
#define IWDT_CNTRLD_OFFSET      ((uint32_t)0x08) // 计数器重装载寄存器


class Register;
class Iwdt : public PeripheralDevice {
  private:
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;
    void initialize_registers();

  public:
    Iwdt();
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return IWDT_BASE; }
    std::string getName() override { return "IWDT"; }
};