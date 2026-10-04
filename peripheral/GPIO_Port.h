// GPIO_Port.h
#pragma once
#include "peripheral_factory.h"
#include "Register/GPIORegister.h"

#include <functional>

// ----------------------------------------------------
// 1. GPIO 模块的基地址
// ----------------------------------------------------
#define GPIOA_BASE          ((uint32_t)0x40000000)
#define GPIOB_BASE          ((uint32_t)0x40000400)
#define GPIOC_BASE          ((uint32_t)0x40000800)
#define GPIOD_BASE          ((uint32_t)0x40000C00)
#define GPIO_PORT_SIZE      0x400 // 每个端口的地址范围大小

class Register;

/**
 * @brief 模拟单个 GPIO 端口（如 GPIOA, GPIOB, GPIOC, GPIOD）
 *
 * 输入侧（DIN）在**读取时**动态合成，规则与真实引脚一致：
 *   外部注入 > 输出回读 DOUT > 上拉输入=1 > 浮空(问用户/默认 0)
 * 用户可通过 driveInput() 在任意时刻注入/释放某引脚电平，
 * 也可通过 setInputQuery() 挂"固件读到未注入的浮空输入时提示用户"的钩子。
 *
 * 注意（Unicorn 限制）：读 Hook 触发时本次读取已经完成，无法改写本次返回值，
 * 因此合成出的 DIN 会同时**种回 guest 内存**，让固件下一次读（通常紧跟其后）拿到正确值。
 */
class GPIO_Port : public PeripheralDevice {
  private:
    const uint64_t m_base_address;
    const std::string m_port_name; // 例如 "A" 或 "B"

    // 使用 Map 存储寄存器状态，Key: 偏移地址，Value: Register 实例
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;

    // 引脚状态（每端口 8 位，由寄存器值刷新而来）
    bool m_output[8] = {};   // MODE:   true = 输出模式
    bool m_pullUp[8] = {};   // CTRL1:  输入时 true = 上拉
    bool m_dout[8] = {};     // DOUT:   输出数据位
    bool m_extDriven[8] = {}; // 外部是否已注入电平
    uint8_t m_extLevel[8] = {}; // 外部注入电平 0/1

    // 浮空输入读取时的用户查询钩子：返回 true 表示已给出电平
    std::function<bool(int pin_index, int &level)> m_inputQuery;

    // 上一次打印过的 DIN 值（只在变化时打印，避免固件轮询刷屏）
    uint32_t m_lastDinLogged = 0xFFFFFFFFU;

    // 初始化寄存器复位值
    void initialize_registers();

    // 从寄存器模型刷新引脚状态（MODE / CTRL1 / DOUT）
    void refreshPinConfig();

    // 读寄存器当前值（不存在返回 0）
    uint32_t regValue(uint32_t offset);

    // 引脚当前有效电平（外部注入 > 输出回读 > 上拉=1 > 浮空=0）
    [[nodiscard]] int pinLevel(int pin_index) const;

    // 合成 DIN 值；allowPrompt=true 时对"浮空且未注入"的引脚询问用户
    uint32_t synthesizeDin(bool allowPrompt);

    // 输出引脚电平变化日志（人类可读）
    void logOutputChanges(const bool *oldDout);

  public:
    /**
     * @brief 构造函数
     * @param base_addr 端口的基地址 (如 GPIOA_BASE)
     * @param port_name 端口名称 (如 "A")
     */
    GPIO_Port(uint64_t base_addr, std::string port_name);

    // 实现 PeripheralDevice 接口
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    void plantInitialValues(uc_engine *uc) override;
    uint64_t getBaseAddress() override { return m_base_address; }
    std::string getName() override { return "GPIO_Port_" + m_port_name; }

    // —— 用户输入注入接口 ——
    /**
     * @brief 注入/释放某引脚的外部电平（模拟外部器件驱动该引脚）
     * @param pin 单引脚位掩码（如 GPIO_PIN_4）
     * @param level 0/1 表示注入电平；<0 表示释放，回到默认规则
     */
    void driveInput(uint8_t pin, int level);

    /** @brief 挂"浮空输入读取时询问用户"的钩子（返回 true 表示已给出电平） */
    void setInputQuery(std::function<bool(int pin_index, int &level)> query) {
        m_inputQuery = std::move(query);
    }

    /** @brief 当前引脚有效电平（外部观察用） */
    [[nodiscard]] int level(int pin_index) const { return pinLevel(pin_index); }

    /** @brief 把当前引脚状态对应的 DIN 值种回 guest 内存（启动注入后调用） */
    void syncDinToGuest(uc_engine *engine);
};
