#pragma once
#include "peripheral_factory.h"
#include <map>
#include <memory>

// ----------------------------------------------------
// 1. BUZZER 基础地址和范围
// ----------------------------------------------------
// BUZZER 模块的基地址
#define BUZZER_BASE         ((uint32_t)0x40002800)
// BUZZER 模块的结束地址
#define BUZZER_END          ((uint32_t)0x40002BFF)

// ----------------------------------------------------
// 2. BUZZER 寄存器偏移地址
// ----------------------------------------------------
#define BUZZER_CSTS_OFFSET  ((uint32_t)0x00) // 控制/状态寄存器 (Control/Status Register)

class Register;
class Buzzer : public PeripheralDevice {
  private:
    // 存储 BUZZER 寄存器状态，Key: 偏移地址
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;

    // 初始化寄存器复位值
    void initialize_registers();

  public:
    Buzzer();
    // 继承自 PeripheralDevice 的虚函数
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return BUZZER_BASE; }
    std::string getName() override { return "BUZZER"; }
};