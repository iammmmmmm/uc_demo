#include "I2c.h"
#include <iomanip>
#include <iostream>

#include "I2cRegisters.h"
#include "Register/Register.h"
#include "../main.h"

using namespace I2cRegisters; // 引入 I2C 寄存器命名空间

// 默认构造函数
I2c::I2c() {
    initialize_registers();
}

// 初始化 I2C 寄存器复位值
void I2c::initialize_registers() {
    // 根据 I2C 寄存器列表实例化所有寄存器对象
    m_registers[I2C_CTRL1_OFFSET]     = std::make_unique<I2C_CTRL1_Register>(I2C_CTRL1_OFFSET);
    m_registers[I2C_CTRL2_OFFSET]     = std::make_unique<I2C_CTRL2_Register>(I2C_CTRL2_OFFSET);
    m_registers[I2C_CLKFREQ_OFFSET]   = std::make_unique<I2C_CLKFREQ_Register>(I2C_CLKFREQ_OFFSET);
    m_registers[I2C_ADDR0_OFFSET]     = std::make_unique<I2C_ADDR0_Register>(I2C_ADDR0_OFFSET);
    m_registers[I2C_ADDR1_OFFSET]     = std::make_unique<I2C_ADDR1_Register>(I2C_ADDR1_OFFSET);
    m_registers[I2C_DATA_OFFSET]      = std::make_unique<I2C_DATA_Register>(I2C_DATA_OFFSET);
    m_registers[I2C_STS1_OFFSET]      = std::make_unique<I2C_STS1_Register>(I2C_STS1_OFFSET);
    m_registers[I2C_STS2_OFFSET]      = std::make_unique<I2C_STS2_Register>(I2C_STS2_OFFSET);
    m_registers[I2C_STS3_OFFSET]      = std::make_unique<I2C_STS3_Register>(I2C_STS3_OFFSET);
    m_registers[I2C_INTCTRL_OFFSET]   = std::make_unique<I2C_INTCTRL_Register>(I2C_INTCTRL_OFFSET);
    m_registers[I2C_CLKCTRL1_OFFSET]  = std::make_unique<I2C_CLKCTRL1_Register>(I2C_CLKCTRL1_OFFSET);
    m_registers[I2C_CLKCTRL2_OFFSET]  = std::make_unique<I2C_CLKCTRL2_Register>(I2C_CLKCTRL2_OFFSET);
    m_registers[I2C_MRT_OFFSET]       = std::make_unique<I2C_MRT_Register>(I2C_MRT_OFFSET);
}


// 处理写入操作
bool I2c::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4) {
        // 仅支持 32 位写入
        std::cerr << "   [" << getName() << " W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - I2C_BASE);
    const auto new_value = static_cast<uint32_t>(value);

    // 处理写入逻辑和详细调试日志
    if (m_registers.contains(offset)) {
        // 调用 Register 类的 write 方法，其中包含位域解析和调试输出
        m_registers[offset]->write(new_value);

#if IS_DEBUG
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

        // 根据偏移量添加中文注释 (模仿 Buzzer.cc 的日志格式)
        switch (offset) {
            case I2C_CTRL1_OFFSET: std::cout << "写入 I2C_CTRL1 (控制 1) 寄存器"; break;
            case I2C_CTRL2_OFFSET: std::cout << "写入 I2C_CTRL2 (控制 2) 寄存器"; break;
            case I2C_CLKFREQ_OFFSET: std::cout << "写入 I2C_CLKFREQ (时钟频率) 寄存器"; break;
            case I2C_ADDR0_OFFSET: std::cout << "写入 I2C_ADDR0 (从机地址 0) 寄存器"; break;
            case I2C_ADDR1_OFFSET: std::cout << "写入 I2C_ADDR1 (从机地址 1) 寄存器"; break;
            case I2C_DATA_OFFSET: std::cout << "写入 I2C_DATA (数据) 寄存器"; break;
            case I2C_INTCTRL_OFFSET: std::cout << "写入 I2C_INTCTRL (中断控制) 寄存器"; break;
            case I2C_CLKCTRL1_OFFSET: std::cout << "写入 I2C_CLKCTRL1 (时钟控制 1) 寄存器"; break;
            case I2C_CLKCTRL2_OFFSET: std::cout << "写入 I2C_CLKCTRL2 (时钟控制 2) 寄存器"; break;
            case I2C_MRT_OFFSET: std::cout << "写入 I2C_MRT (上升时间) 寄存器"; break;
            default:
                std::cout << "写入 未知/保留 I2C 寄存器";
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
bool I2c::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF; // 默认错误值
        return true;
    }

    auto offset = static_cast<uint32_t>(address - I2C_BASE);
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

    // 根据偏移量添加中文注释 (模仿 Buzzer.cc 的日志格式)
    switch (offset) {
        case I2C_CTRL1_OFFSET: std::cout << "读取 I2C_CTRL1 (控制 1) 寄存器"; break;
        case I2C_CTRL2_OFFSET: std::cout << "读取 I2C_CTRL2 (控制 2) 寄存器"; break;
        case I2C_CLKFREQ_OFFSET: std::cout << "读取 I2C_CLKFREQ (时钟频率) 寄存器"; break;
        case I2C_ADDR0_OFFSET: std::cout << "读取 I2C_ADDR0 (从机地址 0) 寄存器"; break;
        case I2C_ADDR1_OFFSET: std::cout << "读取 I2C_ADDR1 (从机地址 1) 寄存器"; break;
        case I2C_DATA_OFFSET: std::cout << "读取 I2C_DATA (数据) 寄存器"; break;
        case I2C_STS1_OFFSET: std::cout << "读取 I2C_STS1 (状态 1) 寄存器"; break;
        case I2C_STS2_OFFSET: std::cout << "读取 I2C_STS2 (状态 2) 寄存器"; break;
        case I2C_STS3_OFFSET: std::cout << "读取 I2C_STS3 (状态 3) 寄存器"; break;
        case I2C_INTCTRL_OFFSET: std::cout << "读取 I2C_INTCTRL (中断控制) 寄存器"; break;
        case I2C_CLKCTRL1_OFFSET: std::cout << "读取 I2C_CLKCTRL1 (时钟控制 1) 寄存器"; break;
        case I2C_CLKCTRL2_OFFSET: std::cout << "读取 I2C_CLKCTRL2 (时钟控制 2) 寄存器"; break;
        case I2C_MRT_OFFSET: std::cout << "读取 I2C_MRT (上升时间) 寄存器"; break;
        default:
            std::cout << "读取 未知/保留 I2C 寄存器";
            break;
    }

    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}