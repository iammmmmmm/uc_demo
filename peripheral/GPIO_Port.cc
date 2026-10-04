// GPIO_Port.cc
#include "GPIO_Port.h"

#include <iomanip>
#include <iostream>
#include <string>

// 实现 GPIO_Port 构造函数
GPIO_Port::GPIO_Port(uint64_t base_addr, std::string port_name)
    : m_base_address(base_addr), m_port_name(std::move(port_name)) {
    // 调用初始化函数设置复位值
    initialize_registers();
    refreshPinConfig();
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
    const uint32_t ctrl1_reset = (m_port_name == "D") ? 0x00000002 : 0x00000000;
    m_registers[GPIOx_CTRL1_OFFSET] =
            std::make_unique<GPIOx_CTRL1_Register>(name_prefix + "CTRL1", ctrl1_reset);

    // 5. GPIOx_CTRL2 (0x10)
    m_registers[GPIOx_CTRL2_OFFSET] = std::make_unique<GPIOx_CTRL2_Register>(name_prefix + "CTRL2");

    // 6. GPIO_JTAGDIS (0x100) - 只有 GPIOD 有
    if (m_port_name == "D") {
        m_registers[GPIO_JTAGDIS_OFFSET] = std::make_unique<GPIO_JTAGDIS_Register>();
    }
}

uint32_t GPIO_Port::regValue(uint32_t offset) {
    if (const auto it = m_registers.find(offset); it != m_registers.end()) {
        return it->second->read();
    }
    return 0;
}

// MODE / CTRL1 / DOUT -> 引脚状态
void GPIO_Port::refreshPinConfig() {
    const uint32_t dout = regValue(GPIOx_DOUT_OFFSET);
    const uint32_t mode = regValue(GPIOx_MODE_OFFSET);
    const uint32_t ctrl1 = regValue(GPIOx_CTRL1_OFFSET);

    for (int i = 0; i < 8; ++i) {
        const uint32_t bit = 1U << i;
        m_dout[i] = (dout & bit) != 0;
        m_output[i] = (mode & bit) != 0;   // MODE: 1 = 输出, 0 = 输入
        m_pullUp[i] = (ctrl1 & bit) != 0;  // 输入时 CTRL1: 1 = 上拉
    }
}

int GPIO_Port::pinLevel(int pin_index) const {
    if (pin_index < 0 || pin_index > 7) {
        return 0;
    }
    if (m_extDriven[pin_index]) {
        return m_extLevel[pin_index] != 0 ? 1 : 0;   // 外部注入优先
    }
    if (m_output[pin_index]) {
        return m_dout[pin_index] ? 1 : 0;            // 推挽/开漏输出读回自己驱动的电平
    }
    if (m_pullUp[pin_index]) {
        return 1;                                    // 上拉输入，空闲为高（按键未按下）
    }
    return 0;                                        // 浮空输入，默认低
}

uint32_t GPIO_Port::synthesizeDin(bool allowPrompt) {
    uint32_t value = 0;
    for (int i = 0; i < 8; ++i) {
        // 浮空且未注入的输入引脚：交给用户/外部电路决定
        if (allowPrompt && !m_extDriven[i] && !m_output[i] && !m_pullUp[i] && m_inputQuery) {
            int level = 0;
            if (m_inputQuery(i, level)) {
                driveInput(static_cast<uint8_t>(1U << i), level); // 缓存，避免每次读都问
            }
        }
        if (pinLevel(i) == 1) {
            value |= 1U << i;
        }
    }
    return value;
}

void GPIO_Port::syncDinToGuest(uc_engine *engine) {
    const uint32_t din = synthesizeDin(false);
    plantValueToGuest(engine, m_base_address + GPIOx_DIN_OFFSET, 4, din);
}

// 把寄存器初值(含按引脚合成的 DIN)种进 guest 内存, 固件的第一次读就能拿到正确值
void GPIO_Port::plantInitialValues(uc_engine *uc) {
    if (uc == nullptr) {
        return;
    }
    for (const auto &[offset, reg] : m_registers) {
        const uint32_t value =
                (offset == GPIOx_DIN_OFFSET) ? synthesizeDin(false) : reg->read();
        plantValueToGuest(uc, m_base_address + offset, 4, value);
    }
}

void GPIO_Port::driveInput(uint8_t pin, int level) {
    for (int i = 0; i < 8; ++i) {
        if ((pin & (1U << i)) == 0) {
            continue;
        }
        if (level < 0) {
            m_extDriven[i] = false;
            m_extLevel[i] = 0;
        } else {
            m_extDriven[i] = true;
            m_extLevel[i] = static_cast<uint8_t>(level & 1);
        }
    }
}

// 输出引脚电平变化日志（固件写 DOUT -> 外部可见）
void GPIO_Port::logOutputChanges(const bool *oldDout) {
    for (int i = 0; i < 8; ++i) {
        if (!m_output[i] || oldDout[i] == m_dout[i]) {
            continue;
        }
        std::cout << "[GPIO" << m_port_name << "] P" << i << " -> "
                << (m_dout[i] ? "H" : "L") << std::endl;
    }
}

// 处理写入操作
bool GPIO_Port::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
    const auto offset = static_cast<uint32_t>(address - m_base_address);

    if (!m_registers.contains(offset)) {
        std::cerr << "   [" << getName() << " W: 0x" << std::hex << std::setw(2) << std::setfill('0')
                << offset << "] 警告: 访问未注册寄存器!" << std::endl;
        return true;
    }

    const bool oldDout[8] = {
        m_dout[0], m_dout[1], m_dout[2], m_dout[3], m_dout[4], m_dout[5], m_dout[6], m_dout[7]
    };

    // 按访问宽度合并写入（固件可能用 8/16 位访问）
    if (size == 4) {
        m_registers[offset]->write(static_cast<uint32_t>(value));
    } else if (size == 1 || size == 2) {
        const uint32_t mask = (1U << (size * 8)) - 1U;
        const uint32_t merged =
                (m_registers[offset]->read() & ~mask) | (static_cast<uint32_t>(value) & mask);
        m_registers[offset]->write(merged);
    } else {
        std::cerr << "   [" << getName() << " W: 0x" << std::hex << offset
                << "] 警告: 不支持的访问宽度 " << std::dec << size << std::endl;
        return true;
    }

    refreshPinConfig();
    if (offset == GPIOx_DOUT_OFFSET) {
        logOutputChanges(oldDout); // 「输出时输出」
    }
    // 输出/配置变化会改变输入回读值，同步给固件
    syncDinToGuest(uc);
    return true;
}

// 处理读取操作：DIN 在读取时动态合成
bool GPIO_Port::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    const auto offset = static_cast<uint32_t>(address - m_base_address);

    if (!m_registers.contains(offset)) {
        std::cerr << "   [" << getName() << " R: 0x" << std::hex << offset
                << "] 警告: 访问未注册寄存器!" << std::endl;
        *read_value = 0;
        return true;
    }

    uint32_t value = 0;
    if (offset == GPIOx_DIN_OFFSET) {
        value = synthesizeDin(true); // 「读时拦截」：外部注入/用户输入在此生效
        if (value != m_lastDinLogged) {
            m_lastDinLogged = value;
            std::cout << "[GPIO" << m_port_name << "] DIN 读 -> 0x" << std::hex << std::setw(2)
                    << std::setfill('0') << value << std::dec << std::endl;
        }
    } else {
        value = regValue(offset);
    }

    *read_value = static_cast<int64_t>(value);
    // 本次读已结束，把模型状态种回内存，让后续读（含读-改-写）看到正确的寄存器内容
    plantValueToGuest(uc, address, size, value);
    return true;
}
