#pragma once

#include <unicorn/unicorn.h>
// 版本号 (uc_demo --version)
#define UC_DEMO_VERSION "1.1.0"

// Unicorn 架构和模式 (ARM Cortex-M0+ 对应 ARM Thumb 32)
#define ARCH UC_ARCH_ARM
#define MODE UC_MODE_THUMB

// MCU 内存地址映射 (基于 APM32F003)
#define FLASH_START   0x08000000
#define FLASH_SIZE    (32 * 1024)  // 32 KB Flash
#define FLASH_END     (FLASH_START + FLASH_SIZE)
#define SRAM_START    0x20000000
#define SRAM_SIZE     (4 * 1024)   // 4 KB SRAM
#define SYS_MEM_START 0x00020000   // 系统存储区
#define SYS_MEM_SIZE  (1 * 1024)   // 1 KB


// ROM
#define ROM_START     0x00000000
#define ROM_SIZE      (32 * 1024)  // 32 KB
// 外设区域起始地址
#define PERIPHERAL_START 0x40000000
#define PERIPHERAL_END   0x400114FF // 整个外设区域的结束大致范围
#define PERIPHERAL_MAP_SIZE 0x12000

// 映射 Cortex-M 核心私有外设区域 (PPB/System Control Block)
// 通常映射 0xE0000000 到 0xE0100000
#define PPB_START 0xE0000000
// 映射大小 0x100000 (1MB)
#define PPB_SIZE 0x100000

// 定义虚拟 HardFault Handler 的地址和指令
#define VIRTUAL_HF_HANDLER_ADDR  (SRAM_START + 0x400) // 选择 SRAM 中的安全地址
// B . (Branch to self) 指令的 Thumb 编码 (0xE7FE)
// 注意：由于 Cortex-M0+ 只有 16 位和 32 位指令，B . 是 16 位指令
// Thumb 模式下的 16 位指令：B. (Branch) => 0xE7FE (字节序 0xFE, 0xE7)
const unsigned char hardfault_code[] = {0xFE, 0xE7}; // B . 永远循环指令

#define UC_HOOK_MEM_WR (UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE )

#define IS_DEBUG true
// 指令级监视 (UC_HOOK_CODE) 改为运行时开关, 见 uc_demo --help 的 -t/--trace

// 外设寄存器级日志的运行时开关 (默认关闭, 由 --periph-log 打开)
// USART 输出的是固件自己打印的字符, 不能被寄存器日志混在一起
extern bool g_periphLog;

extern uc_engine *uc;