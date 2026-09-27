#include "Usart.h"
#include <iomanip>
#include <iostream>

#include "UsartRegisters.h"
#include "Register/Register.h" // 确保路径正确
#include "../main.h"           // 确保路径正确

using namespace UsartRegisters;

USART::USART( uint64_t base_address,const std::string& name)
    : m_name(name), m_base_address(base_address) {
    initialize_registers();
}

void USART::initialize_registers() {
    // 实例化所有寄存器对象
    m_registers[USART_STS_OFFSET]   = std::make_unique<USART_STS_Register>(USART_STS_OFFSET);
    m_registers[USART_DATA_OFFSET]  = std::make_unique<USART_DATA_Register>(USART_DATA_OFFSET);
    m_registers[USART_BR1_OFFSET]   = std::make_unique<USART_BR1_Register>(USART_BR1_OFFSET);
    m_registers[USART_BR0_OFFSET]   = std::make_unique<USART_BR0_Register>(USART_BR0_OFFSET);
    m_registers[USART_CTRL1_OFFSET] = std::make_unique<USART_CTRL1_Register>(USART_CTRL1_OFFSET);
    m_registers[USART_CTRL2_OFFSET] = std::make_unique<USART_CTRL2_Register>(USART_CTRL2_OFFSET);
    m_registers[USART_CTRL3_OFFSET] = std::make_unique<USART_CTRL3_Register>(USART_CTRL3_OFFSET);
    m_registers[USART_CTRL4_OFFSET] = std::make_unique<USART_CTRL4_Register>(USART_CTRL4_OFFSET);
    m_registers[USART_CTRL5_OFFSET] = std::make_unique<USART_CTRL5_Register>(USART_CTRL5_OFFSET);
    m_registers[USART_GTS_OFFSET]   = std::make_unique<USART_GTS_Register>(USART_GTS_OFFSET);
    m_registers[USART_PSC_OFFSET]   = std::make_unique<USART_PSC_Register>(USART_PSC_OFFSET);

    // USART_SW 仅用于 USART2/3，USART_IOSW 仅用于 USART3，但为了简化映射，我们全部分配。
    m_registers[USART_SW_OFFSET]    = std::make_unique<USART_SW_Register>(USART_SW_OFFSET);
    m_registers[USART_IOSW_OFFSET]  = std::make_unique<USART_IOSW_Register>(USART_IOSW_OFFSET);

    // 关联 USART_DATA 和 USART_STS 寄存器，以便 DATA 读写时触发 STS 标志清除逻辑。
    // 这需要强制类型转换，因为 map 存储的是基类指针。
    auto* sts_reg = static_cast<USART_STS_Register*>(m_registers[USART_STS_OFFSET].get());
    auto* data_reg = static_cast<USART_DATA_Register*>(m_registers[USART_DATA_OFFSET].get());

    if (data_reg && sts_reg) {
        data_reg->set_sts_register(sts_reg);
    }
}


// 处理写入操作
bool USART::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - m_base_address);
    const auto new_value = static_cast<uint32_t>(value);

    if (m_registers.contains(offset)) {
        m_registers[offset]->write(new_value);

#if IS_DEBUG
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";
        // 打印寄存器名称
        std::cout << m_registers[offset]->getName() << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << new_value << std::endl;
        std::fflush(stdout);
#endif
    } else {
         std::cerr << "   [" << getName() << " W: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    }

    return true;
}

// 处理读取操作
bool USART::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF;
        return true;
    }

    auto offset = static_cast<uint32_t>(address - m_base_address);
    uint32_t stored_value = 0;

    if (m_registers.contains(offset)) {
        stored_value = m_registers[offset]->read();
    } else {
        std::cerr << "   [" << getName() << " R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        *read_value = 0;
        return true;
    }

    *read_value = static_cast<int64_t>(stored_value);

#if IS_DEBUG
    std::cout << "   [" << getName() << " R: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";
    // 打印寄存器名称
    std::cout << m_registers[offset]->getName() << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}