// peripheral_factory.h
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <unicorn/unicorn.h>

// ----------------------------------------------------
// 1. 抽象基类 (hookDad)
// ----------------------------------------------------
/**
 * @brief 把外设模型的值种回 guest 内存。
 *
 * 为什么需要: Unicorn 的读 Hook 在读取完成之后才触发, 改不了本次读到的值 ——
 * 固件读到的始终是映射 RAM 里的内容。所以要主动把模型状态写回内存, 让固件后续的读
 * (含 `|=` / `&=` 这类读-改-写) 看到正确的寄存器内容。
 */
inline void plantValueToGuest(uc_engine *uc, uint64_t address, int size, uint32_t value) {
    if (uc == nullptr) {
        return;
    }
    if (size == 4) {
        uc_mem_write(uc, address, &value, 4);
    } else if (size == 1 || size == 2) {
        const uint8_t bytes[2] = {
            static_cast<uint8_t>(value & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
        };
        uc_mem_write(uc, address, bytes, size);
    }
}

class PeripheralDevice {
  public:
    virtual ~PeripheralDevice() = default;

    /**
     * @brief 处理来自模拟固件的内存写入操作
     * @param uc Unicorn Engine 实例
     * @param address 写入的内存地址
     * @param size 写入的字节数
     * @param value 写入的值
     * @return true 表示 Hook 已成功处理并可继续模拟；false 表示应停止模拟（或错误）
     */
    virtual bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value);

    /**
         * @brief 处理来自模拟固件的内存读取操作
         * @param uc Unicorn Engine 实例
         * @param address 读取的内存地址
         * @param size 读取的字节数
         * @param read_value 引用，用于将模拟的返回值写入
         * @return true 表示 Hook 已成功处理并可继续模拟；false 表示应停止模拟
         */
    virtual bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value);
    /**
     * @brief 获取该外设的起始地址，用于查找
     */
    virtual uint64_t getBaseAddress();

    /**
     * @brief 获取外设名称
     */
    virtual std::string getName();

    /**
     * @brief 把本设备的寄存器复位值种进 guest 内存 (启动时调用一次)。
     *
     * 为什么必须这么做: Unicorn 的读 Hook 在读取完成之后才触发, 改不了本次读到的值 ——
     * 固件读到的始终是映射 RAM 里的内容。若不把复位值写进 RAM, 固件会一律读到 0,
     * 例如 RCM_GetMasterClockFreq() 会返回 0, 固件的 APM_DelayInit() 就会因
     * SysTick_Config(0) 失败而卡在 while(1)。
     */
    virtual void plantInitialValues(uc_engine *uc) { (void) uc; }
};

// ----------------------------------------------------
// 2. 集中管理外设的注册表 (查找器)
// ----------------------------------------------------
class PeripheralRegistry {
  public:
    static PeripheralRegistry &getInstance() {
      static PeripheralRegistry instance;
      return instance;
    }

    // 注册外设实例
    void registerDevice(std::unique_ptr<PeripheralDevice> device) {
      const uint64_t base_addr = device->getBaseAddress();
      if (m_devices.contains(base_addr)) {
        std::cerr << "警告: 地址 0x" << std::hex << base_addr << " 上的外设已重复注册!" << std::endl;
        return;
      }
      m_devices[base_addr] = std::move(device);
    }

    // 根据地址查找最匹配的外设（返回基地址小于等于给定地址的最近一个外设）
    [[nodiscard]] PeripheralDevice *findDevice(uint64_t address) const {
      // 查找第一个键大于给定地址的元素

      // 如果找到了，将迭代器后退一个位置，指向基地址小于等于给定地址的元素
      if (auto it = m_devices.upper_bound(address); it != m_devices.begin()) {
        --it;
        // 简单检查：确保地址落在这个设备的地址范围内 (需要更细致的范围检查，这里简化为最近的基地址)
        return it->second.get();
      }
      return nullptr;
    }

    // 获取所有注册的外设
    [[nodiscard]] const std::map<uint64_t, std::unique_ptr<PeripheralDevice> > &getDevices() const {
      return m_devices;
    }

  private:
    PeripheralRegistry(const PeripheralRegistry&) = delete;
    PeripheralRegistry& operator=(const PeripheralRegistry&) = delete;
    PeripheralRegistry() = default;
    // 存储外设：Key是外设的起始地址
    std::map<uint64_t, std::unique_ptr<PeripheralDevice> > m_devices;
};
// ----------------------------------------------------
// 通用外设占位符
// ----------------------------------------------------
class GenericPeripheral : public PeripheralDevice {
  public:
    GenericPeripheral(const std::string& name, uint64_t base_addr);

    // 覆写基类方法
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
    uint64_t getBaseAddress() override;
    std::string getName() override;

  protected:
    std::string m_name;
    uint64_t m_base_addr;
};