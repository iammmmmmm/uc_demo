#pragma once
#include "peripheral_factory.h"
#include <map>
#include <memory>
#include <string>

// ----------------------------------------------------
// 1. SPI 基础地址和范围
// ----------------------------------------------------
#define SPI_BASE         ((uint32_t)0x40002C00)
// SPI 模块的结束地址
#define SPI_END          ((uint32_t)0x40002FFF)

// ----------------------------------------------------
// 2. SPI 寄存器偏移地址 (来自用户提供的表格)
// ----------------------------------------------------
#define SPI_CTRL1_OFFSET     ((uint32_t)0x00) // SPI 控制寄存器 1
#define SPI_CTRL2_OFFSET     ((uint32_t)0x04) // SPI 控制寄存器 2
#define SPI_INTCTRL_OFFSET   ((uint32_t)0x08) // SPI 中断控制寄存器
#define SPI_STS_OFFSET       ((uint32_t)0x0C) // SPI 状态寄存器
#define SPI_DATA_OFFSET      ((uint32_t)0x10) // SPI 数据寄存器
#define SPI_CRCPOLY_OFFSET   ((uint32_t)0x14) // SPI CRC 多项式寄存器
#define SPI_RXCRC_OFFSET     ((uint32_t)0x18) // SPI 接收 CRC 寄存器
#define SPI_TXCRC_OFFSET     ((uint32_t)0x1C) // SPI 发送 CRC 寄存器


class Register; // 前向声明
class Spi : public PeripheralDevice {
  private:
    // 存储 SPI 寄存器状态，Key: 偏移地址
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;

    // 初始化所有 SPI 寄存器及其复位值
    void initialize_registers();

  public:
    Spi();
    // 继承自 PeripheralDevice 的虚函数
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return SPI_BASE; }
    std::string getName() override { return "SPI"; }
};