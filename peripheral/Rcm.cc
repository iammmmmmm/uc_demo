#include "Rcm.h"
#include <iomanip>
#include <iostream>

#include "Register/RcmRegisters.h"
#include "Register/Register.h"
#include "../main.h"
// 实现 CMU 构造函数
RCM::RCM() {
  // 调用初始化函数设置复位值
  initialize_registers();
}

// 初始化寄存器复位值
void RCM::initialize_registers() {
  // RCM_ICC (0x00) 复位值: 0x00000001 (HIRCEN = 1)
  m_registers[RCM_ICC_OFFSET] = std::make_unique<Rcm::RCM_ICC_Register>(RCM_ICC_OFFSET);;
  // RCM_ECC (0x04) 复位值: 0x00000000
  m_registers[RCM_ECC_OFFSET] = std::make_unique<Rcm::RCM_ECC_Register>(RCM_ECC_OFFSET);
  // RCM_MCS (0x0C) 复位值: 0x000000E1
  m_registers[RCM_MCS_OFFSET] = std::make_unique<Rcm::RCM_MCS_Register>(RCM_MCS_OFFSET);
  // RCM_MCC (0x10) 复位值: 0x000000E1
  m_registers[RCM_MCC_OFFSET] = std::make_unique<Rcm::RCM_MCC_Register>(RCM_MCC_OFFSET);
  // RCM_CSC (0x14) 复位值: 0x000000XX (初始化为 0x00000000)
  m_registers[RCM_CSC_OFFSET] = std::make_unique<Rcm::RCM_CSC_Register>(RCM_CSC_OFFSET);
  // RCM_CLKDIV (0x18) 复位值: 0x00000018
  m_registers[RCM_CLKDIV_OFFSET] = std::make_unique<Rcm::RCM_CLKDIV_Register>(RCM_CLKDIV_OFFSET);
  // RCM_APBEN1 (0x1C) 复位值: 0x000000FF
  m_registers[RCM_APBEN1_OFFSET] = std::make_unique<Rcm::RCM_APBEN1_Register>(RCM_APBEN1_OFFSET);
  // RCM_CSS (0x20) 复位值: 0x00000000
  m_registers[RCM_CSS_OFFSET] = std::make_unique<Rcm::RCM_CSS_Register>(RCM_CSS_OFFSET);
  // RCM_COC (0x24) 复位值: 0x00000000
  m_registers[RCM_COC_OFFSET] = std::make_unique<Rcm::RCM_COC_Register>(RCM_COC_OFFSET);
  // RCM_APBEN2 (0x28) 复位值: 0x000000FF
  m_registers[RCM_APBEN2_OFFSET] = std::make_unique<Rcm::RCM_APBEN2_Register>(RCM_APBEN2_OFFSET);
  // RCM_HIRCTRIM (0x30) 复位值: 0x00000000
  m_registers[RCM_HIRCTRIM_OFFSET] = std::make_unique<Rcm::RCM_HIRCTRIM_Register>(RCM_HIRCTRIM_OFFSET);
  // RCM_RSTSTS (0x38) 复位值: 0x000000XX (初始化为 0x00000000)
  m_registers[RCM_RSTSTS_OFFSET] = std::make_unique<Rcm::RCM_RSTSTS_Register>(RCM_RSTSTS_OFFSET);
  // RCM_APBEN3 (0x3C) 复位值: 0x00000007
  m_registers[RCM_APBEN3_OFFSET] = std::make_unique<Rcm::RCM_APBEN3_Register>(RCM_APBEN3_OFFSET);
}


// 处理写入操作
bool RCM::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
  if (size != 4) {
    // 仅支持 32 位写入
    std::cerr << "   [CMU W] 警告: 非 32 位写入操作被忽略. 地址: 0x" << std::hex << address << std::endl;
    return true;
  }

  const auto offset = static_cast<uint32_t>(address - RCM_BASE);
  const auto new_value = static_cast<uint32_t>(value);

  // 存储新值到内部状态
  m_registers[offset]->write(new_value);
  switch (offset) {
    case RCM_MCS_OFFSET: {
    }
    break;
    default:
      break;
  }
#if IS_DEBUG
  std::cout << "   [CMU W: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

  // 根据偏移量添加中文注释
  switch (offset) {
    case RCM_ICC_OFFSET: std::cout << "写入 RCM_ICC (内部时钟控制) 寄存器";
      break;
    case RCM_ECC_OFFSET: std::cout << "写入 RCM_ECC (外部时钟控制) 寄存器";
      break;
    case RCM_MCS_OFFSET: std::cout << "写入 RCM_MCS (主时钟状态) 寄存器";
      break; // 注意：此寄存器通常是只读的
    case RCM_MCC_OFFSET: std::cout << "写入 RCM_MCC (主时钟配置) 寄存器";
      break;
    case RCM_CSC_OFFSET: std::cout << "写入 RCM_CSC (时钟切换控制) 寄存器";
      break;
    case RCM_CLKDIV_OFFSET: std::cout << "写入 RCM_CLKDIV (时钟预分频) 寄存器";
      break;
    case RCM_APBEN1_OFFSET: std::cout << "写入 RCM_APBEN1 (APB时钟使能1) 寄存器";
      break;
    case RCM_CSS_OFFSET: std::cout << "写入 RCM_CSS (时钟保护系统) 寄存器";
      break;
    case RCM_COC_OFFSET: std::cout << "写入 RCM_COC (时钟输出控制) 寄存器";
      break;
    case RCM_APBEN2_OFFSET: std::cout << "写入 RCM_APBEN2 (APB时钟使能2) 寄存器";
      break;
    case RCM_HIRCTRIM_OFFSET: std::cout << "写入 RCM_HIRCTRIM (内部高速时钟调整) 寄存器";
      break;
    case RCM_RSTSTS_OFFSET: std::cout << "写入 RCM_RSTSTS (复位状态) 寄存器";
      break; // 注意：此寄存器通常是只读/清除的
    case RCM_APBEN3_OFFSET: std::cout << "写入 RCM_APBEN3 (APB时钟使能3) 寄存器";
      break;
    default: std::cout << "写入 未知/保留 RCM 寄存器";
      break;
  }

  std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << new_value << std::endl;
  std::fflush(stdout);
#endif

  return true;
}

// 处理读取操作
bool RCM::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
  if (size != 4) {
    std::cerr << "   [CMU R] 警告: 非 32 位读取操作被忽略. 地址: 0x" << std::hex << address << std::endl;
    *read_value = 0xDEADBEEF;
    return true;
  }

  auto offset = static_cast<uint32_t>(address - RCM_BASE);
  uint32_t stored_value = 0;

  if (m_registers.contains(offset)) {
    stored_value = m_registers[offset]->read();
  } else {
    std::cerr << "   [CMU R: 0x" << std::hex << offset << "] 警告: 访问未注册寄存器!" << std::endl;
    *read_value = 0;
    return true;
  }


  // --- 关键逻辑：模拟状态标志 ---
  switch (offset) {
    case RCM_ICC_OFFSET:
      if (stored_value & 0x1) {
        stored_value |= (1 << 1); // 模拟设置 HIRC Ready Flag (HIRCRF=1)
      }
      break;
    case RCM_MCS_OFFSET:
      m_registers[RCM_ICC_OFFSET];
      break;
    default:
      break;
  }


  *read_value = static_cast<int64_t>(stored_value);

#if IS_DEBUG
  std::cout << "   [CMU R: 0x" << std::hex << std::setw(2) << std::setfill('0') << offset << "] ";

  // 根据偏移量添加中文注释
  switch (offset) {
    case RCM_ICC_OFFSET: std::cout << "读取 RCM_ICC (内部时钟控制) 寄存器 (包含HIRC就绪标志模拟)";
      break;
    case RCM_ECC_OFFSET: std::cout << "读取 RCM_ECC (外部时钟控制) 寄存器";
      break;
    case RCM_MCS_OFFSET: std::cout << "读取 RCM_MCS (主时钟状态) 寄存器";
      break;
    case RCM_MCC_OFFSET: std::cout << "读取 RCM_MCC (主时钟配置) 寄存器";
      break;
    case RCM_CSC_OFFSET: std::cout << "读取 RCM_CSC (时钟切换控制) 寄存器";
      break;
    case RCM_CLKDIV_OFFSET: std::cout << "读取 RCM_CLKDIV (时钟预分频) 寄存器";
      break;
    case RCM_APBEN1_OFFSET: std::cout << "读取 RCM_APBEN1 (APB时钟使能1) 寄存器";
      break;
    case RCM_CSS_OFFSET: std::cout << "读取 RCM_CSS (时钟保护系统) 寄存器";
      break;
    case RCM_COC_OFFSET: std::cout << "读取 RCM_COC (时钟输出控制) 寄存器";
      break;
    case RCM_APBEN2_OFFSET: std::cout << "读取 RCM_APBEN2 (APB时钟使能2) 寄存器";
      break;
    case RCM_HIRCTRIM_OFFSET: std::cout << "读取 RCM_HIRCTRIM (内部高速时钟调整) 寄存器";
      break;
    case RCM_RSTSTS_OFFSET: std::cout << "读取 RCM_RSTSTS (复位状态) 寄存器";
      break;
    case RCM_APBEN3_OFFSET: std::cout << "读取 RCM_APBEN3 (APB时钟使能3) 寄存器";
      break;
    default: std::cout << "读取 未知/保留 RCM 寄存器";
      break;
  }


  std::cout << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << *read_value << std::endl;
  std::fflush(stdout);
#endif

  return true;
}
