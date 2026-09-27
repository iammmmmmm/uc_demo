#include "Iwdt.h"
#include <iomanip>
#include <iostream>

#include "IwdtRegisters.h"
#include "Register/Register.h" // 确保路径正确
#include "../main.h"           // 确保路径正确

using namespace IwdtRegisters;

Iwdt::Iwdt() {
    initialize_registers();
}

void Iwdt::initialize_registers() {
    m_registers[IWDT_KEYWORD_OFFSET] = std::make_unique<IWDT_KEYWORD_Register>(IWDT_KEYWORD_OFFSET);
    m_registers[IWDT_PSC_OFFSET]     = std::make_unique<IWDT_PSC_Register>(IWDT_PSC_OFFSET);
    m_registers[IWDT_CNTRLD_OFFSET]  = std::make_unique<IWDT_CNTRLD_Register>(IWDT_CNTRLD_OFFSET);
}


// 处理写入操作
bool Iwdt::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4 && size != 2) { // 考虑到支持 16 位和 32 位操作
        std::cerr << "   [" << getName() << " W] 警告: 非 32/16 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - IWDT_BASE);
    const auto new_value = static_cast<uint32_t>(value);

    if (m_registers.contains(offset)) {
        m_registers[offset]->write(new_value);

#if IS_DEBUG
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";
        switch (offset) {
            case IWDT_KEYWORD_OFFSET:
                std::cout << "写入 IWDT_KEYWORD (关键字) 寄存器: 0x" << std::hex << std::setw(4) << std::setfill('0') << (new_value & 0xFFFF);
                if ((new_value & 0xFFFF) == 0xAAAA) {
                    std::cout << " -> 喂狗/重装载";
                }
                break;
            case IWDT_PSC_OFFSET: std::cout << "写入 IWDT_PSC (预分频) 寄存器"; break;
            case IWDT_CNTRLD_OFFSET: std::cout << "写入 IWDT_CNTRLD (重装载) 寄存器"; break;
            default: std::cout << "写入 未知 IWDT 寄存器"; break;
        }
        std::cout << std::endl;
        std::fflush(stdout);
#endif
    } else {
         std::cerr << "   [" << getName() << " W: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    }

    return true;
}

// 处理读取操作
bool Iwdt::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4 && size != 2) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32/16 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF;
        return true;
    }

    auto offset = static_cast<uint32_t>(address - IWDT_BASE);
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
    switch (offset) {
        case IWDT_KEYWORD_OFFSET: std::cout << "读取 IWDT_KEYWORD (关键字) 寄存器"; break;
        case IWDT_PSC_OFFSET: std::cout << "读取 IWDT_PSC (预分频) 寄存器"; break;
        case IWDT_CNTRLD_OFFSET: std::cout << "读取 IWDT_CNTRLD (重装载) 寄存器"; break;
        default: std::cout << "读取 未知 IWDT 寄存器"; break;
    }
    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}