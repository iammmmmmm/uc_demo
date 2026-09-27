// GPIO_Port.cc
#include "GPIO_Port.h"
#include <iomanip>
#include <iostream>

// 实现 GPIO_Port 构造函数
GPIO_Port::GPIO_Port(uint64_t base_addr, std::string port_name)
    : m_base_address(base_addr), m_port_name(std::move(port_name)) {
    // 调用初始化函数设置复位值
    initialize_registers();
}

// 初始化寄存器复位值
void GPIO_Port::initialize_registers() {
    const std::string name_prefix = "GPIO" + m_port_name + "_";

    // 1. GPIOx_DOUT (0x00)
    m_registers[GPIOx_DOUT_OFFSET] = std::make_unique<GPIOx_DOUT_Register>(name_prefix + "DOUT");

    // 2. GPIOx_DIN (0x04)
    m_registers[GPIOx_DIN_OFFSET] = std::make_unique<GPIOx_DIN_Register>(name_prefix + "DIN");
    
    // 3. GPIOx_MODE (0x08)
    m_registers[GPIOx_MODE_OFFSET] = std::make_unique<GPIOx_MODE_Register>(name_prefix + "MODE");

    // 4. GPIOx_CTRL1 (0x0C) - 注意 GPIOD 的特殊复位值
    uint32_t ctrl1_reset = (m_port_name == "D") ? 0x00000002 : 0x00000000;
    m_registers[GPIOx_CTRL1_OFFSET] = std::make_unique<GPIOx_CTRL1_Register>(name_prefix + "CTRL1", ctrl1_reset);

    // 5. GPIOx_CTRL2 (0x10)
    m_registers[GPIOx_CTRL2_OFFSET] = std::make_unique<GPIOx_CTRL2_Register>(name_prefix + "CTRL2");
    
    // 6. GPIO_JTAGDIS (0x100) - 假设这个寄存器只在 GPIOD 模块中有效或被初始化
    if (m_port_name == "D") {
        m_registers[GPIO_JTAGDIS_OFFSET] = std::make_unique<GPIO_JTAGDIS_Register>();
    }
}


// 处理写入操作
bool GPIO_Port::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        std::fflush(stderr);
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - m_base_address);
    const auto new_value = static_cast<uint32_t>(value);

    if (m_registers.contains(offset)) {
        // 调用 Register 基类的 write 方法，该方法会自动处理位域回调和调试输出
        m_registers[offset]->write(new_value);
    } else {
        std::cerr << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    }

    // 实际输出信号的逻辑（在 DOUT 写入时）可以在 DOUT 寄存器的写回调中实现。
    std::fflush(stderr);
    return true;
}

// 处理读取操作
bool GPIO_Port::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF; // 返回一个模拟的错误值
        std::fflush(stderr);
        return true;
    }

    auto offset = static_cast<uint32_t>(address - m_base_address);
    uint32_t stored_value = 0;

    if (m_registers.contains(offset)) {
        // 调用 Register 基类的 read 方法。
        // 对于 GPIOx_DIN (0x04)，其内部值应由外部逻辑（例如 set_input_value() 或 read callback）控制。
        stored_value = m_registers[offset]->read();
        
    } else {
        std::cerr << "   [" << getName() << " R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        *read_value = 0; // 返回 0
        std::fflush(stderr);
        return true;
    }

    *read_value = static_cast<int64_t>(stored_value);

    return true;
}