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


// 把寄存器复位值种进 guest 内存。固件读的是映射 RAM (Unicorn 的读 Hook 改不了本次读到的
// 值), 不种进去的话 STS 会读成 0: TXBEF 不为 1, 固件 io_putchar() 里的
// while(...== RESET && timeout--) 每发一个字节都要空转到超时。
void USART::plantInitialValues(uc_engine *uc) {
    if (uc == nullptr) {
        return;
    }
    for (const auto &[offset, reg] : m_registers) {
        plantValueToGuest(uc, m_base_address + offset, 4, reg->read());
    }
}

// 处理写入操作
bool USART::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    const auto offset = static_cast<uint32_t>(address - m_base_address);

    if (!m_registers.contains(offset)) {
        std::cerr << "   [" << getName() << " W: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        return true;
    }

    // 固件用 USART_TxData8() 做 8 位写（usart->DATA_B.DATA），这里按访问宽度合并进 32 位寄存器
    uint32_t new_value = 0;
    if (size == 4) {
        new_value = static_cast<uint32_t>(value);
    } else if (size == 1 || size == 2) {
        const uint32_t mask = (1U << (size * 8)) - 1U;
        new_value = (m_registers[offset]->read() & ~mask) | (static_cast<uint32_t>(value) & mask);
    } else {
        std::cerr << "   [" << getName() << " W: 0x" << std::hex << offset
                << "] 警告: 不支持的访问宽度 " << std::dec << size << std::endl;
        return true;
    }

    m_registers[offset]->write(new_value);
    plantValueToGuest(uc, address, size, new_value);

    // —— 发送出口：固件写 DATA（低 8 位）就是一个字符，直接吐到 stdout ——
    if (offset == USART_DATA_OFFSET) {
        const char ch = static_cast<char>(new_value & 0xFFU);
        std::cout << ch << std::flush;
        // 发送瞬时完成：STS 保持复位值 0xC0（TXBEF/TXCF 置位），
        // 让固件 io_putchar() 里的 TXBE 轮询立刻通过，也把它同步给后续读。
        if (const auto it = m_registers.find(USART_STS_OFFSET); it != m_registers.end()) {
            plantValueToGuest(uc, m_base_address + USART_STS_OFFSET, 4, it->second->read());
        }
    }

    if (g_periphLog) {
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0')
                << offset << "] " << m_registers[offset]->getName() << ": 0x" << std::hex
                << std::setw(8) << std::setfill('0') << new_value << std::endl;
        std::fflush(stdout);
    }
    return true;
}

// 处理读取操作
bool USART::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    const auto offset = static_cast<uint32_t>(address - m_base_address);

    if (!m_registers.contains(offset)) {
        std::cerr << "   [" << getName() << " R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        *read_value = 0;
        return true;
    }

    uint32_t stored_value = m_registers[offset]->read();
    if (size > 0 && size < 4) {
        stored_value &= (1U << (size * 8)) - 1U;
    }
    *read_value = static_cast<int64_t>(stored_value);
    plantValueToGuest(uc, address, size, stored_value);

    if (g_periphLog) {
        std::cout << "   [" << getName() << " R: 0x" << std::hex << std::setw(2) << std::setfill('0')
                << offset << "] " << m_registers[offset]->getName() << ": 0x" << std::hex
                << std::setw(8) << std::setfill('0') << stored_value << std::endl;
        std::fflush(stdout);
    }
    return true;
}