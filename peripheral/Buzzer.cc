#include "Buzzer.h"
#include <iomanip>
#include <iostream>

#include "Register/BuzzerRegister.h"
#include "Register/Register.h"
#include "../main.h"
// 默认构造函数
Buzzer::Buzzer() {
    initialize_registers();
}

// 初始化寄存器复位值
void Buzzer::initialize_registers() {
    // BUZZER_CSTS (0x00) 复位值: 0x0000001F
    m_registers[BUZZER_CSTS_OFFSET] = std::make_unique<BUZZER_CSTS_Register>();
}


// 处理写入操作
bool Buzzer::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4) {
        // 仅支持 32 位写入
        std::cerr << "   [" << getName() << " W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - BUZZER_BASE);
    const auto new_value = static_cast<uint32_t>(value);
    

    //处理写入逻辑和详细调试日志
    if (m_registers.contains(offset)) {
        // 存储新值到内部状态，并触发 Register 类的详细调试输出 (如果 s_enable_debug_output 为 true)
        m_registers[offset]->write(new_value); 
        
#if IS_DEBUG // 模仿 Rcm.cc 的日志格式
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

        // 根据偏移量添加中文注释
        switch (offset) {
            case BUZZER_CSTS_OFFSET: 
                std::cout << "写入 BUZZER_CSTS (控制/状态) 寄存器";
                break;
            default: 
                std::cout << "写入 未知/保留 BUZZER 寄存器";
                break;
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
bool Buzzer::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF; // 默认错误值
        return true;
    }

    auto offset = static_cast<uint32_t>(address - BUZZER_BASE);
    uint32_t stored_value = 0;

    if (m_registers.contains(offset)) {
        // 调用 Register 类的 read 方法
        stored_value = m_registers[offset]->read();
    } else {
        std::cerr << "   [" << getName() << " R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        *read_value = 0;
        return true;
    }

    *read_value = static_cast<int64_t>(stored_value);

#if IS_DEBUG
    std::cout << "   [" << getName() << " R: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

    // 根据偏移量添加中文注释
    switch (offset) {
        case BUZZER_CSTS_OFFSET: 
            std::cout << "读取 BUZZER_CSTS (控制/状态) 寄存器";
            break;
        default: 
            std::cout << "读取 未知/保留 BUZZER 寄存器";
            break;
    }

    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}