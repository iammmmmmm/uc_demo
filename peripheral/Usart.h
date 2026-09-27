#pragma once
#include "peripheral_factory.h"
#include <map>
#include <memory>
#include <string>

// ----------------------------------------------------
// 1. USART 基础地址和范围
// ----------------------------------------------------
#define USART3_BASE         ((uint32_t)0x40004800)
#define USART3_END          ((uint32_t)0x40004BFF)

#define USART1_BASE         ((uint32_t)0x40003400)
#define USART1_END          ((uint32_t)0x400037FF)

#define USART2_BASE         ((uint32_t)0x40001400)
#define USART2_END          ((uint32_t)0x400017FF)


// ----------------------------------------------------
// 2. USART 寄存器偏移地址
// ----------------------------------------------------
#define USART_STS_OFFSET      ((uint32_t)0x00)
#define USART_DATA_OFFSET     ((uint32_t)0x04)
#define USART_BR1_OFFSET      ((uint32_t)0x08)
#define USART_BR0_OFFSET      ((uint32_t)0x0C)
#define USART_CTRL1_OFFSET    ((uint32_t)0x10)
#define USART_CTRL2_OFFSET    ((uint32_t)0x14)
#define USART_CTRL3_OFFSET    ((uint32_t)0x18)
#define USART_CTRL4_OFFSET    ((uint32_t)0x1C)
#define USART_CTRL5_OFFSET    ((uint32_t)0x20)
#define USART_GTS_OFFSET      ((uint32_t)0x24)
#define USART_PSC_OFFSET      ((uint32_t)0x28)
#define USART_SW_OFFSET       ((uint32_t)0x2C)
#define USART_IOSW_OFFSET     ((uint32_t)0x30)


class Register;
class USART : public PeripheralDevice {
  private:
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;
    std::string m_name;
    uint64_t m_base_address;

    void initialize_registers();

  public:
    USART( uint64_t base_address,const std::string& name);
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return m_base_address; }
    std::string getName() override { return m_name; }
};