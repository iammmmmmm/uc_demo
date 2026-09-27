#pragma once
#include "peripheral_factory.h"
// 闪存接口 (FLASH) 基地址
#define FLASH_BASE          (0x40011000UL)

// ----------------------------------------------------------------------
// 寄存器偏移地址 (Offsets)
// ----------------------------------------------------------------------
#define FLASH_CTRL1_OFFSET  (0x00UL) // 控制寄存器1
#define FLASH_KEY_OFFSET    (0x04UL) // 关键字寄存器
#define FLASH_OBKEY_OFFSET  (0x08UL) // 选项字节关键字寄存器
#define FLASH_STS_OFFSET    (0x0CUL) // 状态寄存器
#define FLASH_CTRL2_OFFSET  (0x10UL) // 控制寄存器2
#define FLASH_ADDR_OFFSET   (0x14UL) // 地址寄存器
#define FLASH_OBCS_OFFSET   (0x1CUL) // 选项字节控制/状态寄存器
#define FLASH_WRTPROT_OFFSET (0x20UL) // 写保护寄存器
#define FLASH_LPM_OFFSET    (0x24UL) // 低功耗模式寄存器
#define FLASH_TPO_OFFSET    (0x28UL) // 闪存tpo er_on寄存器

// ----------------------------------------------------------------------
// 寄存器绝对地址 (Addresses)
// ----------------------------------------------------------------------
#define FLASH_CTRL1_ADDR    (FLASH_BASE + FLASH_CTRL1_OFFSET)
#define FLASH_KEY_ADDR      (FLASH_BASE + FLASH_KEY_OFFSET)
#define FLASH_OBKEY_ADDR    (FLASH_BASE + FLASH_OBKEY_OFFSET)
#define FLASH_STS_ADDR      (FLASH_BASE + FLASH_STS_OFFSET)
#define FLASH_CTRL2_ADDR    (FLASH_BASE + FLASH_CTRL2_OFFSET)
#define FLASH_ADDR_ADDR     (FLASH_BASE + FLASH_ADDR_OFFSET)
#define FLASH_OBCS_ADDR     (FLASH_BASE + FLASH_OBCS_OFFSET)
#define FLASH_WRTPROT_ADDR  (FLASH_BASE + FLASH_WRTPROT_OFFSET)
#define FLASH_LPM_ADDR      (FLASH_BASE + FLASH_LPM_OFFSET)
#define FLASH_TPO_ADDR      (FLASH_BASE + FLASH_TPO_OFFSET)

// ----------------------------------------------------------------------
// 寄存器指针定义
// ----------------------------------------------------------------------
#define FLASH_CTRL1         (*((volatile uint32_t *) FLASH_CTRL1_ADDR))
#define FLASH_STS           (*((volatile uint32_t *) FLASH_STS_ADDR))
// Flash Interface 的基地址
#define FLASH_IF_BASE 0x40011000
class Register; // 前向声明 Register 基类

// Flash 模块类定义
class Flash : public PeripheralDevice {
  private:
    // 使用 Map 存储寄存器状态，Key: 偏移地址，Value: Register 智能指针
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;
    // 用于模拟 FLASH_CTRL2 的锁定状态 (复位值 0x00000080，LOCK=1)
    bool m_locked = true;

    // 初始化寄存器复位值
    void initialize_registers();
  public:
    Flash();
    // 继承自 PeripheralDevice
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override { return FLASH_BASE; }
    std::string getName() override { return "FMC (Flash Memory Controller)"; }
};