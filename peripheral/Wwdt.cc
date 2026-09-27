#include "Wwdt.h"
#include <iomanip>
#include <iostream>

#include "WwdtRegisters.h"
#include "Register/Register.h"
#include "../main.h"

using namespace WwdtRegisters;

Wwdt::Wwdt() {
    initialize_registers();
}

void Wwdt::initialize_registers() {
    m_registers[WWDT_CTRL_OFFSET]    = std::make_unique<WWDT_CTRL_Register>(WWDT_CTRL_OFFSET);
    m_registers[WWDT_WDDATA_OFFSET]  = std::make_unique<WWDT_WDDATA_Register>(WWDT_WDDATA_OFFSET);
}


// 处理写入操作
bool Wwdt::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4 && size != 2) {
        std::cerr << "   [" << getName() << " W] 警告: 非 32/16 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - WWDT_BASE);
    const auto new_value = static_cast<uint32_t>(value);

    if (m_registers.contains(offset)) {
        m_registers[offset]->write(new_value);

#if IS_DEBUG
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";
        switch (offset) {
            case WWDT_CTRL_OFFSET: std::cout << "写入 WWDT_CTRL (控制) 寄存器 (注意 WWDTEN R/S 逻辑)"; break;
            case WWDT_WDDATA_OFFSET: std::cout << "写入 WWDT_WDDATA (窗口数据) 寄存器"; break;
            default: std::cout << "写入 未知 WWDT 寄存器"; break;
        }
        std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << new_value << std::endl;
        std::fflush(stdout);
#endif
    } else {
         std::cerr << "   [" << getName() << " W: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    }

    return true;
}

// 处理读取操作
bool Wwdt::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4 && size != 2) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32/16 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF;
        return true;
    }

    auto offset = static_cast<uint32_t>(address - WWDT_BASE);
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
        case WWDT_CTRL_OFFSET: std::cout << "读取 WWDT_CTRL (控制) 寄存器"; break;
        case WWDT_WDDATA_OFFSET: std::cout << "读取 WWDT_WDDATA (窗口数据) 寄存器"; break;
        default: std::cout << "读取 未知 WWDT 寄存器"; break;
    }
    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}