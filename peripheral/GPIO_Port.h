// GPIO_Port.h
#pragma once
#include "peripheral_factory.h"
#include "Register/GPIORegister.h"

// ----------------------------------------------------
// 1. GPIO 模块的基地址
// ----------------------------------------------------
#define GPIOA_BASE          ((uint32_t)0x40000000)
#define GPIOB_BASE          ((uint32_t)0x40000400)
#define GPIOC_BASE          ((uint32_t)0x40000800)
#define GPIOD_BASE          ((uint32_t)0x40000C00)
#define GPIO_PORT_SIZE      0x400 // 每个端口的地址范围大小

class Register;

/**
 * @brief 模拟单个 GPIO 端口（如 GPIOA, GPIOB, GPIOC, GPIOD）
 */
class GPIO_Port : public PeripheralDevice {
  private:
    const uint64_t m_base_address;
    const std::string m_port_name; // 例如 "A" 或 "B"

    // 使用 Map 存储寄存器状态，Key: 偏移地址，Value: Register 实例
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;

    // 初始化寄存器复位值
    void initialize_registers();

  public:
    /**
     * @brief 构造函数
     * @param base_addr 端口的基地址 (如 GPIOA_BASE)
     * @param port_name 端口名称 (如 "A")
     */
    GPIO_Port(uint64_t base_addr, std::string port_name);

    // 实现 PeripheralDevice 接口
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return m_base_address; }
    std::string getName() override { return "GPIO_Port_" + m_port_name; }
};