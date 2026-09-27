#include "peripheral_factory.h"
#include <iostream>

// 确保链接器能找到 vtable 和这些函数的定义

bool PeripheralDevice::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
  std::cerr << "[WARN] 未处理的写入操作到基类虚函数: 0x" << std::hex << address << std::endl;
  return true; // 默认允许继续模拟
}

bool PeripheralDevice::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
  std::cerr << "[WARN] 未处理的读取操作到基类虚函数: 0x" << std::hex << address << std::endl;
  if (read_value) {
    *read_value = 0; // 默认返回 0
  }
  return true; // 默认允许继续模拟
}

uint64_t PeripheralDevice::getBaseAddress() {
  std::cerr << "[WARN] 调用了基类 getBaseAddress()" << std::endl;
  return 0;
}

std::string PeripheralDevice::getName() {
  return "Unknown PeripheralDevice";
}
// ==========================================================
// GenericPeripheral 实现
// ==========================================================
GenericPeripheral::GenericPeripheral(const std::string& name, uint64_t base_addr)
    : m_name(name), m_base_addr(base_addr) {}

bool GenericPeripheral::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
  uint64_t offset = address - m_base_addr;
  std::cout << "[Generic] " << m_name << " Write: Offset=0x" << std::hex << offset
            << ", Val=0x" << value << std::dec << std::endl;
  return true;
}

bool GenericPeripheral::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
  uint64_t offset = address - m_base_addr;
  *read_value = 0; // 默认返回0
  std::cout << "[Generic] " << m_name << " Read: Offset=0x" << std::hex << offset << std::dec << std::endl;
  return true;
}

uint64_t GenericPeripheral::getBaseAddress() { return m_base_addr; }
std::string GenericPeripheral::getName() { return m_name; }