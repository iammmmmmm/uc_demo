#include "Flash.h"
#include <iomanip>
#include <iostream>
#include <utility>
#include "Register/FlashRegisters.h"
#include "Register/Register.h"
#include "../main.h"
// 实现 Flash 构造函数
Flash::Flash() {
    // 调用初始化函数设置复位值
    initialize_registers();
}

// 初始化寄存器复位值
void Flash::initialize_registers() {
    // 实例化所有 Flash 寄存器对象
    m_registers[FLASH_CTRL1_OFFSET] = std::make_unique<Reg_FLASH_CTRL1>(FLASH_CTRL1_OFFSET);
    m_registers[FLASH_KEY_OFFSET] = std::make_unique<Reg_FLASH_KEY>(FLASH_KEY_OFFSET);
    m_registers[FLASH_OBKEY_OFFSET] = std::make_unique<Reg_FLASH_OBKEY>(FLASH_OBKEY_OFFSET);
    m_registers[FLASH_STS_OFFSET] = std::make_unique<Reg_FLASH_STS>(FLASH_STS_OFFSET);
    m_registers[FLASH_CTRL2_OFFSET] = std::make_unique<Reg_FLASH_CTRL2>(FLASH_CTRL2_OFFSET);
    m_registers[FLASH_ADDR_OFFSET] = std::make_unique<Reg_FLASH_ADDR>(FLASH_ADDR_OFFSET);
    m_registers[FLASH_OBCS_OFFSET] = std::make_unique<Reg_FLASH_OBCS>(FLASH_OBCS_OFFSET);
    m_registers[FLASH_WRTPROT_OFFSET] = std::make_unique<Reg_FLASH_WRTPROT>(FLASH_WRTPROT_OFFSET);
    m_registers[FLASH_LPM_OFFSET] = std::make_unique<Reg_FLASH_LPM>(FLASH_LPM_OFFSET);
    m_registers[FLASH_TPO_OFFSET] = std::make_unique<Reg_FLASH_TPO>(FLASH_TPO_OFFSET);

    // 默认锁定状态由 FLASH_CTRL2 的复位值决定
    m_locked = (m_registers[FLASH_CTRL2_OFFSET]->read() & (1U << 7)) != 0;
}

// 处理写入操作
bool Flash::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    if (size != 4) { // 仅支持 32 位写入
        std::cerr << "   [Flash W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        return true;
    }

    const auto offset = static_cast<uint32_t>(address - FLASH_BASE);
    const auto new_value = static_cast<uint32_t>(value);

    if (!m_registers.contains(offset)) {
        std::cerr << "   [Flash W: 0x" << std::hex << offset << "] 警告: 写入未注册寄存器!" << std::endl;
        return true;
    }

    // --- 关键逻辑：特殊寄存器处理 ---

    // 1. FLASH_KEY (解锁) 寄存器处理
    // 假设写入正确的 KEY 值（例如 0x45670123）可以解锁 FLASH_CTRL2
    if (offset == FLASH_KEY_OFFSET && new_value != 0) {
        // 简化模拟：写入任意非零值被认为是解锁尝试，并成功解锁
        if (m_locked) {
            m_locked = false;
            std::cout << "   [Flash W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] 写入 FLASH_KEY (关键字)。Flash CTRL2 已解锁。" << std::endl;
        }
    }

    // 2. FLASH_CTRL2 (控制寄存器2) 锁定检查
    if (offset == FLASH_CTRL2_OFFSET) {
        if (m_locked) {
            std::cerr << "   [Flash W: 0x" << std::hex << offset << "] 错误: 写入 FLASH_CTRL2 (控制寄存器2) 失败，寄存器处于锁定状态 (LOCK=1)。" << std::endl;
            return true; // 锁定状态下禁止写入
        }

        // 检查写入值是否包含 LOCK 位 (Bit 7) 置 1
        if (new_value & (1U << 7)) {
            m_locked = true;
        }
    }


    // 存储新值到内部状态
    m_registers[offset]->write(new_value);

#if IS_DEBUG
    // 打印写入日志
    std::cout << "   [Flash W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";
    switch (offset) {
        case FLASH_CTRL1_OFFSET: std::cout << "写入 FLASH_CTRL1 (控制寄存器1)";
            break;
        case FLASH_KEY_OFFSET: std::cout << "写入 FLASH_KEY (关键字)";
            break;
        case FLASH_OBKEY_OFFSET: std::cout << "写入 FLASH_OBKEY (选项字节关键字)";
            break;
        case FLASH_STS_OFFSET: std::cout << "写入 FLASH_STS (状态寄存器) - R/W 清除标志";
            break;
        case FLASH_CTRL2_OFFSET: {
            std::cout << "写入 FLASH_CTRL2 (控制寄存器2)";
            if (m_locked) {
                std::cout << " -> Flash CTRL2 已被锁定";
            }
            break;
        }
        case FLASH_ADDR_OFFSET: std::cout << "写入 FLASH_ADDR (地址寄存器)";
            break;
        case FLASH_OBCS_OFFSET: std::cout << "写入 FLASH_OBCS (选项字节控制/状态)";
            break;
        case FLASH_WRTPROT_OFFSET: std::cout << "写入 FLASH_WRTPROT (写保护)";
            break;
        case FLASH_LPM_OFFSET: std::cout << "写入 FLASH_LPM (低功耗模式)";
            break;
        case FLASH_TPO_OFFSET: std::cout << "写入 FLASH_TPO (启动时间)";
            break;
        default: std::cout << "写入 未知/保留 Flash 寄存器";
            break;
    }

    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << new_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}

// 处理读取操作
bool Flash::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    if (size != 4) {
        std::cerr << "   [Flash R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
        *read_value = 0xDEADBEEF;
        return true;
    }

    auto offset = static_cast<uint32_t>(address - FLASH_BASE);
    uint32_t stored_value = 0;

    if (!m_registers.contains(offset)) {
        std::cerr << "   [Flash R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        *read_value = 0;
        return true;
    }

    // 从寄存器对象中读取当前值
    stored_value = m_registers[offset]->read();

    // --- 关键逻辑：特殊寄存器处理 ---

    // FLASH_KEY (0x04) 和 FLASH_OBKEY (0x08) 寄存器读取时返回 0
    if (offset == FLASH_KEY_OFFSET || offset == FLASH_OBKEY_OFFSET) {
        stored_value = 0x00000000;
    }

    *read_value = static_cast<int64_t>(stored_value);

#if IS_DEBUG
    // 打印读取日志
    std::cout << "   [Flash R: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";
    switch (offset) {
        case FLASH_CTRL1_OFFSET:    std::cout << "读取 FLASH_CTRL1 (控制寄存器1)"; break;
        case FLASH_KEY_OFFSET:      std::cout << "读取 FLASH_KEY (关键字) - 返回 0"; break;
        case FLASH_OBKEY_OFFSET:    std::cout << "读取 FLASH_OBKEY (选项字节关键字) - 返回 0"; break;
        case FLASH_STS_OFFSET:      std::cout << "读取 FLASH_STS (状态寄存器)"; break;
        case FLASH_CTRL2_OFFSET:    std::cout << "读取 FLASH_CTRL2 (控制寄存器2)"; break;
        case FLASH_ADDR_OFFSET:     std::cout << "读取 FLASH_ADDR (地址寄存器)"; break;
        case FLASH_OBCS_OFFSET:     std::cout << "读取 FLASH_OBCS (选项字节控制/状态)"; break;
        case FLASH_WRTPROT_OFFSET:  std::cout << "读取 FLASH_WRTPROT (写保护)"; break;
        case FLASH_LPM_OFFSET:      std::cout << "读取 FLASH_LPM (低功耗模式)"; break;
        case FLASH_TPO_OFFSET:      std::cout << "读取 FLASH_TPO (启动时间)"; break;
        default:                    std::cout << "读取 未知/保留 Flash 寄存器"; break;
    }

    std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
    std::fflush(stdout);
#endif

    return true;
}