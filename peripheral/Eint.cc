#include "Eint.h"
#include "Register/EnitRegisters.h"
#include "../main.h"

// =========================================================================
// EINT (External Interrupt) 外设实现
// =========================================================================

EINT::EINT() {
  initialize_registers();
}
// 初始化寄存器复位值
void EINT::initialize_registers() {
  // EINT_CTRL1 (0x00) 复位值: 0x00000000
  m_registers[EINT_CTRL1_OFFSET] = std::make_unique<EINT_CTRL1_Register>(EINT_CTRL1_OFFSET);

  // EINT_CTRL2 (0x04) 复位值: 0x00000000
  m_registers[EINT_CTRL2_OFFSET] = std::make_unique<EINT_CTRL2_Register>(EINT_CTRL2_OFFSET);

  // EINT_CLR (0x08) 复位值: 0x00000000
  m_registers[EINT_CLR_OFFSET] = std::make_unique<EINT_CLR_Register>(EINT_CLR_OFFSET);
}


// 处理写入操作
bool EINT::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
  if (size != 4) {
    // 仅支持 32 位写入
    std::cerr << "   [EINT W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
    return true;
  }

  const auto offset = static_cast<uint32_t>(address - EINT_BASE); // 假设 EINT_BASE 已定义
  const auto new_value = static_cast<uint32_t>(value);

  if (m_registers.contains(offset)) {
    // 存储新值到内部状态
    m_registers[offset]->write(new_value);
  } else {
    std::cerr << "   [EINT W: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    return true;
  }

#if IS_DEBUG
  std::cout << "   [EINT W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

  // 根据偏移量添加中文注释
  switch (offset) {
    case EINT_CTRL1_OFFSET: std::cout << "写入 EINT_CTRL1 (外部中断控制1) 寄存器";
      break;
    case EINT_CTRL2_OFFSET: std::cout << "写入 EINT_CTRL2 (外部中断控制2) 寄存器";
      break;
    case EINT_CLR_OFFSET: std::cout << "写入 EINT_CLR (外部中断清除) 寄存器";
      break;
    default: std::cout << "写入 未知/保留 EINT 寄存器";
      break;
  }

  std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << new_value << std::endl;
  std::fflush(stdout);
#endif

  return true;
}

// 处理读取操作
bool EINT::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
  if (size != 4) {
    std::cerr << "   [EINT R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
    *read_value = 0xDEADBEEF; // 假设用于指示错误读取的魔数
    return true;
  }

  auto offset = static_cast<uint32_t>(address - EINT_BASE); // 假设 EINT_BASE 已定义
  uint32_t stored_value = 0;

  if (m_registers.contains(offset)) {
    // 从内部状态读取值
    stored_value = m_registers[offset]->read();
  } else {
    std::cerr << "   [EINT R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    *read_value = 0;
    return true;
  }

  *read_value = static_cast<int64_t>(stored_value);

#if IS_DEBUG
  std::cout << "   [EINT R: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

  // 根据偏移量添加中文注释
  switch (offset) {
    case EINT_CTRL1_OFFSET: std::cout << "读取 EINT_CTRL1 (外部中断控制1) 寄存器";
      break;
    case EINT_CTRL2_OFFSET: std::cout << "读取 EINT_CTRL2 (外部中断控制2) 寄存器";
      break;
    case EINT_CLR_OFFSET: std::cout << "读取 EINT_CLR (外部中断清除) 寄存器";
      break;
    default: std::cout << "读取 未知/保留 EINT 寄存器";
      break;
  }


  std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
  std::fflush(stdout);
#endif

  return true;
}