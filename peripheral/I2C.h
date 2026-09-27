#pragma once
#include "peripheral_factory.h"
#include <map>
#include <memory>
#include <string>

// ----------------------------------------------------
// 1. I2C 基础地址和范围 (来自地址映射图)
// ----------------------------------------------------
// I2C 模块的基地址 (0x4000 3000)
#define I2C_BASE         ((uint32_t)0x40003000)
// I2C 模块的结束地址 (0x4000 33FF)
#define I2C_END          ((uint32_t)0x400033FF)

// ----------------------------------------------------
// 2. I2C 寄存器偏移地址 (来自 I2C 寄存器地址映射)
// ----------------------------------------------------
#define I2C_CTRL1_OFFSET     ((uint32_t)0x00) // 控制寄存器 1
#define I2C_CTRL2_OFFSET     ((uint32_t)0x04) // 控制寄存器 2
#define I2C_CLKFREQ_OFFSET   ((uint32_t)0x08) // 时钟频率寄存器
#define I2C_ADDR0_OFFSET     ((uint32_t)0x0C) // 从机地址寄存器 0
#define I2C_ADDR1_OFFSET     ((uint32_t)0x10) // 从机地址寄存器 1
#define I2C_DATA_OFFSET      ((uint32_t)0x18) // 数据寄存器
#define I2C_STS1_OFFSET      ((uint32_t)0x1C) // 状态寄存器 1
#define I2C_STS2_OFFSET      ((uint32_t)0x20) // 状态寄存器 2
#define I2C_STS3_OFFSET      ((uint32_t)0x24) // 状态寄存器 3
#define I2C_INTCTRL_OFFSET   ((uint32_t)0x28) // 中断控制寄存器
#define I2C_CLKCTRL1_OFFSET  ((uint32_t)0x2C) // 主机时钟控制寄存器 1
#define I2C_CLKCTRL2_OFFSET  ((uint32_t)0x30) // 主机时钟控制寄存器 2
#define I2C_MRT_OFFSET       ((uint32_t)0x34) // 上升时间寄存器

class Register; // 前向声明
class I2c : public PeripheralDevice {
  private:
    // 存储 I2C 寄存器状态，Key: 偏移地址
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;

    // 初始化所有 I2C 寄存器及其复位值
    void initialize_registers();

  public:
    I2c();
    // 继承自 PeripheralDevice 的虚函数
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return I2C_BASE; }
    std::string getName() override { return "I2C"; }
};