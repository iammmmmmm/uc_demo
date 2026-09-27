#include "Spi.h"
#include <iomanip>
#include <iostream>

#include "SpiRegisters.h"
#include "Register/Register.h"
#include "../main.h"

using namespace SpiRegisters; // 引入 SPI 寄存器命名空间

// 默认构造函数
Spi::Spi() {
    initialize_registers();
}

// 初始化 SPI 寄存器复位值
void Spi::initialize_registers() {
    // 实例化所有 SPI 寄存器对象
    m_registers[SPI_CTRL1_OFFSET]   = std::make_unique<SPI_CTRL1_Register>(SPI_CTRL1_OFFSET);
    m_registers[SPI_CTRL2_OFFSET]   = std::make_unique<SPI_CTRL2_Register>(SPI_CTRL2_OFFSET);
    m_registers[SPI_INTCTRL_OFFSET] = std::make_unique<SPI_INTCTRL_Register>(SPI_INTCTRL_OFFSET);
    m_registers[SPI_STS_OFFSET]     = std::make_unique<SPI_STS_Register>(SPI_STS_OFFSET);
    m_registers[SPI_DATA_OFFSET]    = std::make_unique<SPI_DATA_Register>(SPI_DATA_OFFSET);
    m_registers[SPI_CRCPOLY_OFFSET] = std::make_unique<SPI_CRCPOLY_Register>(SPI_CRCPOLY_OFFSET);
    m_registers[SPI_RXCRC_OFFSET]   = std::make_unique<SPI_RXCRC_Register>(SPI_RXCRC_OFFSET);
    m_registers[SPI_TXCRC_OFFSET]   = std::make_unique<SPI_TXCRC_Register>(SPI_TXCRC_OFFSET);
}


// 处理写入操作
bool Spi::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4) {
        // 仅支持 32 位写入
        std::cerr << "   [" << getName() << " W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - SPI_BASE);
    const auto new_value = static_cast<uint32_t>(value);

    // 处理写入逻辑和详细调试日志
    if (m_registers.contains(offset)) {
        // 调用 Register 类的 write 方法，其中包含位域解析和调试输出
        m_registers[offset]->write(new_value);

#if IS_DEBUG
        std::cout << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

        // 根据偏移量添加中文注释
        switch (offset) {
            case SPI_CTRL1_OFFSET: std::cout << "写入 SPI_CTRL1 (控制 1) 寄存器"; break;
            case SPI_CTRL2_OFFSET: std::cout << "写入 SPI_CTRL2 (控制 2) 寄存器"; break;
            case SPI_INTCTRL_OFFSET: std::cout << "写入 SPI_INTCTRL (中断控制) 寄存器"; break;
            case SPI_STS_OFFSET: std::cout << "写入 SPI_STS (状态) 寄存器 (注意 RC_W0 逻辑)"; break;
            case SPI_DATA_OFFSET: std::cout << "写入 SPI_DATA (数据) 寄存器"; break;
            case SPI_CRCPOLY_OFFSET: std::cout << "写入 SPI_CRCPOLY (CRC 多项式) 寄存器"; break;
            case SPI_RXCRC_OFFSET: std::cout << "写入 SPI_RXCRC (接收 CRC) 寄存器"; break;
            case SPI_TXCRC_OFFSET: std::cout << "写入 SPI_TXCRC (发送 CRC) 寄存器"; break;
            default:
                std::cout << "写入 未知/保留 SPI 寄存器";
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
bool Spi::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4) {
        std::cerr << "   [" << getName() << " R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF; // 默认错误值
        return true;
    }

    auto offset = static_cast<uint32_t>(address - SPI_BASE);
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
        case SPI_CTRL1_OFFSET: std::cout << "读取 SPI_CTRL1 (控制 1) 寄存器"; break;
        case SPI_CTRL2_OFFSET: std::cout << "读取 SPI_CTRL2 (控制 2) 寄存器"; break;
        case SPI_INTCTRL_OFFSET: std::cout << "读取 SPI_INTCTRL (中断控制) 寄存器"; break;
        case SPI_STS_OFFSET: std::cout << "读取 SPI_STS (状态) 寄存器"; break;
        case SPI_DATA_OFFSET: std::cout << "读取 SPI_DATA (数据) 寄存器"; break;
        case SPI_CRCPOLY_OFFSET: std::cout << "读取 SPI_CRCPOLY (CRC 多项式) 寄存器"; break;
        case SPI_RXCRC_OFFSET: std::cout << "读取 SPI_RXCRC (接收 CRC) 寄存器"; break;
        case SPI_TXCRC_OFFSET: std::cout << "读取 SPI_TXCRC (发送 CRC) 寄存器"; break;
        default:
            std::cout << "读取 未知/保留 SPI 寄存器";
            break;
    }

    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}