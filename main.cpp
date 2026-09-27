#include "main.h" // 包含宏定义和函数声明

#include "FirmwareImage.h"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Eint.h"
#include "I2c.h"
#include "Iwdt.h"
#include "Spi.h"
#include "Usart.h"
#include "Wwdt.h"
#include "peripheral/Buzzer.h"
#include "peripheral/Flash.h"
#include "peripheral/GPIO_Port.h"
#include "peripheral/Rcm.h"
#include "peripheral/Register/Register.h"
#include "peripheral_factory.h"

uc_engine *uc;
uc_err err;

namespace {

// -----------------------------------------------------------------------------
// 退出码
// -----------------------------------------------------------------------------
enum ExitCode {
  EXIT_OK = 0,         // 模拟执行完成
  EXIT_USAGE = 1,      // 命令行参数错误
  EXIT_FIRMWARE = 2,   // 固件文件无法读取 / 无法加载
  EXIT_UC = 3,         // Unicorn 初始化或内存映射失败
  EXIT_EMU = 4,        // 模拟执行过程中出错
};

// -----------------------------------------------------------------------------
// 命令行选项
// -----------------------------------------------------------------------------
struct Options {
  std::string firmware;       // 固件路径; 为空时按默认候选路径查找
  ImageFormat format = ImageFormat::Auto;
  uint32_t binBase = FLASH_START;  // .bin 的加载基地址
  uint64_t maxInsns = 1000000;     // 最大执行指令数 (0 = 不限制)
  uint64_t timeoutUs = 0;          // 模拟超时, 微秒 (0 = 不限制)
  uint64_t until = 0;              // 执行到该地址时停止 (0 = 不限制)
  bool trace = false;              // 指令级 Hook
  // BLX 目标监视点 (仅在 --trace 下生效), 默认保留原有固件的两个分析点
  std::vector<uint32_t> watchBlx{FLASH_START + 0x1814, FLASH_START + 0x1830};
  bool periphHooks = false;        // 外设读写 Hook
  bool mirror = true;              // Flash / ROM 双向镜像写入
  bool quiet = false;              // 精简输出
};

// 未指定固件时的默认查找名 (会依次在 exe 目录与工作目录附近查找)
constexpr const char *DEFAULT_FIRMWARE = "firmware/firmware.bin";

// 供 Hook 回调使用的运行时状态
struct TraceState {
  const Options *opt = nullptr;
  unsigned long long instructions = 0;
};
TraceState g_trace;

// -----------------------------------------------------------------------------
// 输出小工具: 避免 std::hex 状态泄漏到后续输出
// -----------------------------------------------------------------------------
std::string hex(uint64_t value, int width = 8) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase << std::setw(width) << std::setfill('0') << value;
  return oss.str();
}

// 判断地址所属区域 (用于日志与错误提示)
std::string describeAddress(uint32_t addr) {
  if (addr >= ROM_START && addr < ROM_START + ROM_SIZE) {
    return "ROM/系统存储区 (0x00000000-0x00007FFF)";
  }
  if (addr >= SRAM_START && addr < SRAM_START + SRAM_SIZE) {
    return "SRAM (0x20000000-0x20000FFF)";
  }
  if (addr >= PERIPHERAL_START && addr <= PERIPHERAL_END) {
    return "外设区域 (0x40000000-0x400114FF)";
  }
  if (addr >= PPB_START && addr < PPB_START + PPB_SIZE) {
    return "内核私有外设 (0xE0000000-0xE00FFFFF)";
  }
  if (addr >= FLASH_START && addr < FLASH_START + FLASH_SIZE) {
    return "Flash 代码区 (0x08000000-0x08007FFF)";
  }
  return "未知/未映射地址";
}

// 已映射的内存区域, 用于写入前校验
struct MemoryRegion {
  uint32_t start;
  uint32_t size;
  const char *name;
};
constexpr MemoryRegion kRegions[] = {
    {FLASH_START, FLASH_SIZE, "Flash"},
    {SRAM_START, SRAM_SIZE, "SRAM"},
    {ROM_START, ROM_SIZE, "ROM/系统存储区"},
};

// 地址区间是否完整落在已映射区域内
const char *containingRegion(uint32_t address, uint64_t size) {
  for (const MemoryRegion &region : kRegions) {
    if (address >= region.start &&
        static_cast<uint64_t>(address) + size <=
            static_cast<uint64_t>(region.start) + region.size) {
      return region.name;
    }
  }
  return nullptr;
}

// 计算 Flash <-> ROM 的镜像地址 (固件常以 0x00000000 链接,
// 而向量表/HardFault 重定向按 ROM 地址访问, 因此两块保持一致)
bool mirrorAddress(uint32_t address, uint64_t size, uint32_t &mirrored) {
  const uint32_t romEnd = ROM_START + ROM_SIZE;
  const uint32_t flashEnd = FLASH_START + FLASH_SIZE;
  if (address >= ROM_START && static_cast<uint64_t>(address) + size <= romEnd) {
    mirrored = FLASH_START + (address - ROM_START);
    return true;
  }
  if (address >= FLASH_START && static_cast<uint64_t>(address) + size <= flashEnd) {
    mirrored = ROM_START + (address - FLASH_START);
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// 命令行解析
// -----------------------------------------------------------------------------
enum class ParseResult { Ok, ExitSuccess, Error };

void printVersion() {
  std::cout << "uc_demo " << UC_DEMO_VERSION << " - APM32F003 (Cortex-M0+) 固件模拟器\n";
}

void printHelp(const char *argv0) {
  printVersion();
  std::cout <<
      "\n用法: " << argv0 << " [选项] [固件文件]\n"
      "\n位置参数:\n"
      "  <固件文件>                 要启动的固件, 等价于 -f\n"
      "\n固件选项:\n"
      "  -f, --firmware <文件>      指定要启动的固件文件\n"
      "                             支持 .bin / .hex / .elf, 未指定时依次查找\n"
      "                             ./firmware/firmware.bin 与 exe 同级的 firmware/\n"
      "      --format <bin|hex|elf> 强制指定格式 (默认按扩展名与文件头自动识别)\n"
      "      --base <地址>          .bin 的加载基地址 (默认 0x08000000)\n"
      "      --no-mirror            不把固件同时镜像写入 Flash 与 ROM\n"
      "\n执行控制:\n"
      "  -n, --max-insns <数量>     最大执行指令数, 0 表示不限制 (默认 1000000)\n"
      "      --timeout <微秒>       模拟超时时间, 0 表示不限制 (默认 0)\n"
      "      --until <地址>         执行到该地址时停止 (默认不限制)\n"
      "\n调试选项:\n"
      "  -t, --trace                开启指令级 Hook (逐条打印 PC, 输出量极大)\n"
      "      --watch-blx <地址表>   监视 BLX 目标值, 逗号分隔; 传 none 清空\n"
      "                             (默认 0x08001814,0x08001830, 需配合 -t)\n"
      "      --hooks                启用外设读写 Hook (默认关闭)\n"
      "  -q, --quiet                精简输出 (外设注册表/Hook 提示/逐条轨迹)\n"
      "  -v, --version              显示版本号\n"
      "  -h, --help                 显示本帮助\n"
      "\n示例:\n"
      "  " << argv0 << " firmware/firmware.hex\n"
      "  " << argv0 << " -f my.bin --base 0x08000000\n"
      "  " << argv0 << " -f firmware.elf --hooks --until 0x08001A00\n"
      "  " << argv0 << " -f firmware.bin -t --watch-blx 0x08001814\n"
      "\n退出码: 0 成功, 1 参数错误, 2 固件错误, 3 Unicorn 初始化失败, 4 模拟执行失败\n";
}

bool parseUint64(const std::string &text, uint64_t &out) {
  if (text.empty()) {
    return false;
  }
  char *end = nullptr;
  errno = 0;
  const unsigned long long value = std::strtoull(text.c_str(), &end, 0); // 支持 0x 前缀
  if (errno != 0 || end == text.c_str() || *end != '\0') {
    return false;
  }
  out = value;
  return true;
}

bool parseAddr(const std::string &text, uint32_t &out) {
  uint64_t value = 0;
  if (!parseUint64(text, value) || value > 0xFFFFFFFFull) {
    return false;
  }
  out = static_cast<uint32_t>(value);
  return true;
}

// 解析 "0x08001814,0x08001830" 形式的地址表; 传入 none 表示清空
bool parseAddrList(const std::string &text, std::vector<uint32_t> &out, std::string &error) {
  out.clear();
  if (text == "none" || text == "NONE" || text.empty()) {
    return true;
  }
  std::istringstream iss(text);
  std::string item;
  while (std::getline(iss, item, ',')) {
    if (item.empty()) {
      continue;
    }
    uint32_t addr = 0;
    if (!parseAddr(item, addr)) {
      error = "无法解析地址: " + item;
      return false;
    }
    out.push_back(addr);
  }
  return true;
}

// 从 argv 取选项值, 支持 "--opt value" 与 "--opt=value" 两种写法
bool takeValue(int argc,
               char **argv,
               int &i,
               const std::string &inlineValue,
               bool hasInline,
               const std::string &optName,
               std::string &out,
               std::string &error) {
  if (hasInline) {
    out = inlineValue;
    return true;
  }
  if (i + 1 >= argc) {
    error = "选项 " + optName + " 缺少参数值";
    return false;
  }
  out = argv[++i];
  return true;
}

ParseResult parseArgs(int argc, char **argv, Options &opt, std::string &error) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    std::string inlineValue;
    bool hasInline = false;
    if (arg.rfind("--", 0) == 0) {
      const auto eq = arg.find('=');
      if (eq != std::string::npos) {
        inlineValue = arg.substr(eq + 1);
        arg = arg.substr(0, eq);
        hasInline = true;
      }
    }

    std::string value;
    auto valueFor = [&](const char *name) {
      return takeValue(argc, argv, i, inlineValue, hasInline, name, value, error);
    };

    if (arg == "-h" || arg == "--help") {
      printHelp(argv[0]);
      return ParseResult::ExitSuccess;
    }
    if (arg == "-v" || arg == "--version") {
      printVersion();
      return ParseResult::ExitSuccess;
    }
    if (arg == "-f" || arg == "--firmware") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      opt.firmware = value;
    } else if (arg == "--format") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      if (value == "bin") {
        opt.format = ImageFormat::Bin;
      } else if (value == "hex") {
        opt.format = ImageFormat::Hex;
      } else if (value == "elf") {
        opt.format = ImageFormat::Elf;
      } else if (value == "auto") {
        opt.format = ImageFormat::Auto;
      } else {
        error = "--format 只支持 bin / hex / elf / auto, 收到: " + value;
        return ParseResult::Error;
      }
    } else if (arg == "--base") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      if (!parseAddr(value, opt.binBase)) {
        error = "--base 地址非法: " + value;
        return ParseResult::Error;
      }
    } else if (arg == "-n" || arg == "--max-insns") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      if (!parseUint64(value, opt.maxInsns)) {
        error = "--max-insns 数量非法: " + value;
        return ParseResult::Error;
      }
    } else if (arg == "--timeout") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      if (!parseUint64(value, opt.timeoutUs)) {
        error = "--timeout 时间非法: " + value;
        return ParseResult::Error;
      }
    } else if (arg == "--until") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      uint32_t addr = 0;
      if (!parseAddr(value, addr)) {
        error = "--until 地址非法: " + value;
        return ParseResult::Error;
      }
      opt.until = addr;
    } else if (arg == "-t" || arg == "--trace") {
      opt.trace = true;
    } else if (arg == "--watch-blx" || arg == "--watch") {
      if (!valueFor(arg.c_str())) {
        return ParseResult::Error;
      }
      if (!parseAddrList(value, opt.watchBlx, error)) {
        return ParseResult::Error;
      }
    } else if (arg == "--hooks") {
      opt.periphHooks = true;
    } else if (arg == "--no-mirror") {
      opt.mirror = false;
    } else if (arg == "-q" || arg == "--quiet") {
      opt.quiet = true;
    } else if (!arg.empty() && arg[0] == '-') {
      error = "未知选项: " + arg + " (使用 --help 查看可用选项)";
      return ParseResult::Error;
    } else {
      // 位置参数: 固件文件
      if (!opt.firmware.empty()) {
        error = "只能指定一个固件文件 (已指定: " + opt.firmware + ", 又收到: " + arg + ")";
        return ParseResult::Error;
      }
      opt.firmware = arg;
    }
  }
  return ParseResult::Ok;
}

// -----------------------------------------------------------------------------
// Hook 回调函数 A: 监视代码执行 (UC_HOOK_CODE) - 用于追踪执行流
// -----------------------------------------------------------------------------
void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
  auto *state = static_cast<TraceState *>(user_data);
  if (state == nullptr || state->opt == nullptr) {
    return;
  }
  state->instructions++;

  // 逐条轨迹输出量极大, 仅在未开启 --quiet 时打印
  if (!state->opt->quiet) {
    std::cout << ">>> [CODE] 指令计数: " << state->instructions
        << ", 当前PC: " << hex(address) << std::endl;
  }

  // --- 监视关键的 BLX 目标 ---
  bool watched = false;
  for (const uint32_t watch : state->opt->watchBlx) {
    if (address == watch) {
      watched = true;
      break;
    }
  }
  if (!watched) {
    return;
  }

  uint32_t r3_val = 0;
  uc_reg_read(uc, UC_ARM_REG_R3, &r3_val); // 读取 r3 的值
  const uint32_t jump_addr = r3_val & ~1u; // 清除 Thumb 位, 获取纯地址

  std::cout << "\n>>> ⚠️ 关键 BLX R3 监视点命中! PC: " << hex(address) << std::endl;
  std::cout << "    r3 (BLX 目标值): " << hex(r3_val) << std::endl;
  std::cout << "    目标区域: " << describeAddress(jump_addr) << std::endl;
}

// -----------------------------------------------------------------------------
// Hook 回调函数 B: 监视外设读写
// -----------------------------------------------------------------------------
void hook_mem_wr(uc_engine *uc,
                 uc_mem_type type,
                 uint64_t address,
                 int size,
                 int64_t value,
                 void *user_data) {
  auto *state = static_cast<TraceState *>(user_data);
  const bool quiet = state != nullptr && state->opt != nullptr && state->opt->quiet;

  if (address < PERIPHERAL_START) {
    return; // 只关心外设区域
  }

  uint32_t current_pc = 0;
  uc_reg_read(uc, UC_ARM_REG_PC, &current_pc);

  PeripheralDevice *device = PeripheralRegistry::getInstance().findDevice(address);
  if (!quiet) {
    std::cout << "\n>>>  外设" << (type == UC_MEM_WRITE ? "写" : "读") << "监视:" << std::endl;
    std::cout << "    PC地址: " << hex(current_pc) << std::endl;
    std::cout << "    操作地址: " << hex(address) << std::endl;
    std::cout << "    值: " << hex(static_cast<uint64_t>(value), size * 2) << std::endl;
    std::cout << "    数据大小: " << std::dec << size << " 字节" << std::endl;
  }

  if (device == nullptr) {
    if (!quiet) {
      std::cout << "     未注册的设备地址: 无法找到对应的外设驱动！" << std::endl;
    }
    return;
  }
  if (!quiet) {
    std::cout << "     交由设备驱动处理: **" << device->getName() << "** (Base: "
        << hex(device->getBaseAddress()) << ")" << std::endl;
  }

  if (type == UC_MEM_WRITE) {
    device->handle_write(uc, address, size, value);
  } else {
    device->handle_read(uc, address, size, &value);
  }
}

// -----------------------------------------------------------------------------
// Hook 回调函数 C: 监视未映射内存访问 (用于调试 UNMAPPED 错误)
// -----------------------------------------------------------------------------
bool hook_mem_unmapped(uc_engine *uc,
                       uc_mem_type type,
                       uint64_t address,
                       int size,
                       int64_t value,
                       void *user_data) {
  uint32_t current_pc = 0;
  uc_reg_read(uc, UC_ARM_REG_PC, &current_pc);
  std::cerr << "\n>>>  致命错误: 尝试访问未映射内存!" << std::endl;
  std::cerr << "    PC地址: " << hex(current_pc) << std::endl;
  std::cerr << "    访问地址: " << hex(address) << " (" << size << " 字节)" << std::endl;
  std::cerr << "    地址区域: " << describeAddress(static_cast<uint32_t>(address)) << std::endl;

  // 对于无法恢复的错误，返回 false 停止模拟
  return false;
}

// -----------------------------------------------------------------------------
// Hook 设置函数: 注册所有 Hook
// -----------------------------------------------------------------------------
bool setup_hooks(uc_engine *uc, const Options &opt) {
  // 1. 未映射内存访问 Hook (始终注册, 便于定位跑飞)
  uc_hook hh_mem_unmap = 0;
  err = uc_hook_add(uc,
                    &hh_mem_unmap,
                    UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED |
                    UC_HOOK_MEM_FETCH_UNMAPPED,
                    reinterpret_cast<void *>(hook_mem_unmapped),
                    nullptr,
                    1,
                    0); // 从地址 1 开始, 长度 0 表示整个地址空间
  if (err) {
    std::cerr << "uc hook add (UNMAPPED) err: " << err << " (" << uc_strerror(err) << ")"
        << std::endl;
    return false;
  }

  // 2. 外设读写 Hook
  if (opt.periphHooks) {
    uc_hook hh_mem_write = 0;
    err = uc_hook_add(uc,
                      &hh_mem_write,
                      UC_HOOK_MEM_WR,
                      reinterpret_cast<void *>(hook_mem_wr),
                      &g_trace,
                      PERIPHERAL_START,
                      PERIPHERAL_END);
    if (err) {
      std::cerr << "uc hook add (PERIPHERAL) err: " << err << " (" << uc_strerror(err) << ")"
          << std::endl;
      return false;
    }
  }

  // 3. 指令级 Hook
  // 固件会通过 BLX 跳到 ROM 镜像 (0x0000_xxxx) 中执行, 只挂 Flash 区间会漏掉这些指令,
  // 也会让指令计数与 Unicorn 的实际执行数对不上, 因此两个区域都要挂钩。
  if (opt.trace) {
    const uint64_t ranges[][2] = {
        {FLASH_START, FLASH_END - 1},
        {ROM_START, ROM_START + ROM_SIZE - 1},
    };
    for (const auto &range : ranges) {
      uc_hook hh_code = 0;
      err = uc_hook_add(uc,
                        &hh_code,
                        UC_HOOK_CODE,
                        reinterpret_cast<void *>(hook_code),
                        &g_trace,
                        range[0],
                        range[1]);
      if (err) {
        std::cerr << "uc hook add (CODE) err: " << err << " (" << uc_strerror(err) << ")"
            << std::endl;
        return false;
      }
    }
  }

  if (!opt.quiet) {
    std::cout << "\n--- Hook 设置成功: "
        << (opt.periphHooks ? "外设读写" : "未启用外设读写") << ", "
        << (opt.trace ? "指令级追踪已开启" : "指令级追踪未开启")
        << ", 未映射访问监视已开启 ---\n" << std::endl;
  }
  return true;
}

// -----------------------------------------------------------------------------
// 虚拟 HardFault Handler: 把向量表 0x0C 指向 SRAM 中的 B . 死循环
// -----------------------------------------------------------------------------
bool setup_VIRTUAL_HF_HANDLER(uc_engine *uc, bool quiet) {
  // 1. 写入 B . 指令到 SRAM
  err = uc_mem_write(uc,
                     VIRTUAL_HF_HANDLER_ADDR,
                     (const void *) hardfault_code,
                     sizeof(hardfault_code));
  if (err) {
    std::cerr << "\n>>> ❌ 致命错误: 无法写入 HardFault Handler 代码到 SRAM ("
        << hex(VIRTUAL_HF_HANDLER_ADDR) << ")! 错误码: " << err << " (" << uc_strerror(err) << ")"
        << std::endl;
    return false;
  }

  // 2. 将 VIRTUAL_HF_HANDLER_ADDR 写入 HardFault 向量 (0x0C)
  // 注意：必须设置最低位为 1 (Thumb 模式)
  const uint32_t handler_vector = static_cast<uint32_t>(VIRTUAL_HF_HANDLER_ADDR) | 1;
  err = uc_mem_write(uc, ROM_START + 0x0C, &handler_vector, sizeof(handler_vector));
  if (err) {
    std::cerr << "\n>>> ❌ 致命错误: 无法将 HardFault 向量 (" << hex(ROM_START + 0x0C)
        << ") 重定向到 " << hex(handler_vector) << "! 错误码: " << err << " ("
        << uc_strerror(err) << ")" << std::endl;
    return false;
  }

  if (!quiet) {
    std::cout << "---  HardFault 向量重定向成功: 0x0C -> " << hex(handler_vector)
        << " (SRAM Handler) ---" << std::endl;
  }
  return true;
}

// -----------------------------------------------------------------------------
// 外设注册
// -----------------------------------------------------------------------------
void registerPeripherals(bool quiet) {
  if (!quiet) {
    std::cout << "--- 注册外设驱动 ---" << std::endl;
  }

  auto &registry = PeripheralRegistry::getInstance();
  registry.registerDevice(std::make_unique<Flash>());  // 0x40011000
  registry.registerDevice(std::make_unique<RCM>());    // 0x40010000
  registry.registerDevice(std::make_unique<I2c>());    // 0x4000 3000
  registry.registerDevice(std::make_unique<Spi>());    // 0x4000 2C00
  registry.registerDevice(std::make_unique<Buzzer>()); // 0x4000 2800
  registry.registerDevice(std::make_unique<Iwdt>());   // 0x4000 2000
  registry.registerDevice(std::make_unique<Wwdt>());   // 0x4000 1C00
  registry.registerDevice(std::make_unique<EINT>());   // 0x4000 1800
  registry.registerDevice(std::make_unique<GPIO_Port>(GPIOD_BASE, "D"));
  registry.registerDevice(std::make_unique<GPIO_Port>(GPIOC_BASE, "C"));
  registry.registerDevice(std::make_unique<GPIO_Port>(GPIOB_BASE, "B"));
  registry.registerDevice(std::make_unique<GPIO_Port>(GPIOA_BASE, "A"));
  registry.registerDevice(std::make_unique<USART>(USART1_BASE, "USART1")); // 0x4000 3400
  registry.registerDevice(std::make_unique<USART>(USART2_BASE, "USART2")); // 0x4000 1400
  registry.registerDevice(std::make_unique<USART>(USART3_BASE, "USART3")); // 0x4000 4800
  registry.registerDevice(std::make_unique<GenericPeripheral>("WUPT", 0x40002400));
  registry.registerDevice(std::make_unique<GenericPeripheral>("ADC", 0x40004400));
  registry.registerDevice(std::make_unique<GenericPeripheral>("TMR4", 0x40004000));
  registry.registerDevice(std::make_unique<GenericPeripheral>("TMR2", 0x40003C00));
  registry.registerDevice(std::make_unique<GenericPeripheral>("TMR1", 0x40003800));
  registry.registerDevice(std::make_unique<GenericPeripheral>("TMR1A", 0x40001000));

  if (!quiet) {
    for (const auto &[base, device] : registry.getDevices()) {
      std::cout << "  - 注册成功: " << device->getName() << " @ " << hex(base) << std::endl;
    }
  }
}

// -----------------------------------------------------------------------------
// 内存映射
// -----------------------------------------------------------------------------
bool mapMemory(uc_engine *uc) {
  struct Mapping {
    uint32_t start;
    uint32_t size;
    uint32_t perms;
    const char *name;
  };
  const Mapping mappings[] = {
      {FLASH_START, FLASH_SIZE, UC_PROT_READ | UC_PROT_EXEC, "Flash"},
      {SRAM_START, SRAM_SIZE, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC, "SRAM"},
      {ROM_START, ROM_SIZE, UC_PROT_READ | UC_PROT_EXEC, "ROM/系统存储区"},
      {PERIPHERAL_START, PERIPHERAL_MAP_SIZE, UC_PROT_READ | UC_PROT_WRITE, "外设区域"},
      {PPB_START, PPB_SIZE, UC_PROT_READ | UC_PROT_WRITE, "Cortex-M 核心私有外设"},
  };

  for (const Mapping &mapping : mappings) {
    err = uc_mem_map(uc, mapping.start, mapping.size, mapping.perms);
    if (err) {
      std::cerr << "映射 " << mapping.name << " (" << hex(mapping.start) << ", "
          << mapping.size << " 字节) 失败. 错误码: " << err << " (" << uc_strerror(err) << ")"
          << std::endl;
      return false;
    }
  }
  return true;
}

// -----------------------------------------------------------------------------
// 把固件写入模拟内存 (含 Flash/ROM 镜像)
// -----------------------------------------------------------------------------
bool writeFirmware(uc_engine *uc, const FirmwareImage &image, const Options &opt) {
  for (const FirmwareChunk &chunk : image.chunks) {
    const uint64_t size = chunk.data.size();
    const uint32_t address = chunk.address;

    if (containingRegion(address, size) == nullptr) {
      std::cerr << ">>> ❌ 固件分段 " << hex(address) << " (" << size
          << " 字节) 落在未映射的内存区域, 无法加载。" << std::endl;
      std::cerr << "    已映射区域: Flash " << hex(FLASH_START) << "-"
          << hex(FLASH_START + FLASH_SIZE - 1) << ", SRAM " << hex(SRAM_START) << "-"
          << hex(SRAM_START + SRAM_SIZE - 1) << ", ROM " << hex(ROM_START) << "-"
          << hex(ROM_START + ROM_SIZE - 1) << std::endl;
      std::cerr << "    请检查 --base 是否与固件的链接地址一致 (默认为 " << hex(FLASH_START)
          << ")。" << std::endl;
      return false;
    }

    err = uc_mem_write(uc, address, chunk.data.data(), size);
    if (err) {
      std::cerr << ">>> ❌ 写入固件到 " << hex(address) << " 失败. 错误码: " << err << " ("
          << uc_strerror(err) << ")" << std::endl;
      return false;
    }

    // 镜像写入: 固件以 0x00000000 链接时, 向量表在 ROM 区, 而启动 PC 取 Flash 镜像
    uint32_t mirrored = 0;
    if (opt.mirror && mirrorAddress(address, size, mirrored)) {
      err = uc_mem_write(uc, mirrored, chunk.data.data(), size);
      if (err) {
        std::cerr << ">>> ❌ 镜像写入固件到 " << hex(mirrored) << " 失败. 错误码: " << err
            << " (" << uc_strerror(err) << ")" << std::endl;
        return false;
      }
    }
  }
  return true;
}

// -----------------------------------------------------------------------------
// 从向量表 / ELF 入口计算 SP 与 PC
// -----------------------------------------------------------------------------
bool setupVectors(uc_engine *uc, const FirmwareImage &image, bool quiet, uint64_t &startPc) {
  // 向量表位于镜像最低地址处: [0] = 初始 SP, [4] = 复位入口
  const uint32_t vectorBase = image.load_base;
  if (containingRegion(vectorBase, 8) == nullptr) {
    std::cerr << ">>> ❌ 向量表地址 " << hex(vectorBase) << " 落在未映射区域。" << std::endl;
    return false;
  }

  uint32_t sp_val = 0;
  err = uc_mem_read(uc, vectorBase, &sp_val, sizeof(sp_val));
  if (err) {
    std::cerr << "读取 SP 初始值失败. 错误码: " << err << " (" << uc_strerror(err) << ")"
        << std::endl;
    return false;
  }

  uint32_t pc_raw = 0;
  err = uc_mem_read(uc, vectorBase + 4, &pc_raw, sizeof(pc_raw));
  if (err) {
    std::cerr << "读取 PC 初始值失败. 错误码: " << err << " (" << uc_strerror(err) << ")"
        << std::endl;
    return false;
  }
  // ELF 头里的入口地址更可靠, 优先使用
  if (image.has_entry) {
    pc_raw = image.entry;
  }

  err = uc_reg_write(uc, UC_ARM_REG_SP, &sp_val);
  if (err) {
    std::cerr << "设置 SP 失败. 错误码: " << err << " (" << uc_strerror(err) << ")" << std::endl;
    return false;
  }

  // 以 0x00000000 链接的镜像需要加上 Flash 基址, 从 Flash 镜像执行 (与原行为一致)
  uint64_t start_pc = pc_raw;
  if (!(pc_raw >= FLASH_START && pc_raw < FLASH_START + FLASH_SIZE)) {
    start_pc = static_cast<uint64_t>(FLASH_START) + pc_raw;
  }
  start_pc = (start_pc & ~1ull) | 1ull; // 强制置位 Thumb 位

  if (!quiet) {
    std::cout << "初始 SP 设置为: " << hex(sp_val) << std::endl;
    std::cout << "初始 PC 设置为: " << hex(start_pc) << " (Thumb)" << std::endl;
  }

  // SP 落在 SRAM 之外通常是向量表读错或固件不匹配, 给出提示但不中断
  if (sp_val < SRAM_START + sizeof(uint32_t) || sp_val > SRAM_START + SRAM_SIZE) {
    std::cerr << "⚠️  警告: 初始 SP " << hex(sp_val) << " 不在 SRAM ("
        << hex(SRAM_START) << "-" << hex(SRAM_START + SRAM_SIZE)
        << ") 范围内, 请确认固件与 --base 是否匹配。" << std::endl;
  }

  // 供 uc_emu_start 使用
  startPc = start_pc;
  return true;
}

}  // namespace

// -----------------------------------------------------------------------------
// 主函数
// -----------------------------------------------------------------------------
int main(int argc, char **argv) {
  Options opt;
  std::string message;

  switch (parseArgs(argc, argv, opt, message)) {
    case ParseResult::ExitSuccess:
      return EXIT_OK;
    case ParseResult::Error:
      std::cerr << "参数错误: " << message << std::endl;
      std::cerr << "使用 --help 查看用法。" << std::endl;
      return EXIT_USAGE;
    case ParseResult::Ok:
      break;
  }

  // ----------------------
  // 0. 定位并加载固件 (先于 Unicorn 初始化, 尽早失败)
  // ----------------------
  const std::string requested = opt.firmware.empty() ? DEFAULT_FIRMWARE : opt.firmware;
  std::vector<std::string> tried;
  const std::string path = resolveFirmwarePath(requested, &tried);
  if (path.empty()) {
    std::cerr << "错误：无法找到固件文件 '" << requested << "'。" << std::endl;
    std::cerr << "已尝试以下路径:" << std::endl;
    for (const std::string &candidate : tried) {
      std::cerr << "  - " << candidate << std::endl;
    }
    std::cerr << "请用 -f/--firmware 指定固件路径。" << std::endl;
    return EXIT_FIRMWARE;
  }

  FirmwareImage image;
  if (!loadFirmware(path, opt.format, opt.binBase, image, message)) {
    std::cerr << "错误：加载固件失败 - " << message << std::endl;
    return EXIT_FIRMWARE;
  }

  if (image.total_bytes > FLASH_SIZE) {
    std::cerr << "错误：固件大小 " << image.total_bytes << " 字节超过 Flash 容量 " << FLASH_SIZE
        << " 字节。" << std::endl;
    return EXIT_FIRMWARE;
  }

  if (!opt.quiet) {
    printVersion();
    std::cout << "[固件] " << image.path << " (格式: " << image.format << ", "
        << image.total_bytes << " 字节, " << image.chunks.size() << " 个分段)" << std::endl;
    for (const FirmwareChunk &chunk : image.chunks) {
      std::cout << "       " << hex(chunk.address) << " - "
          << hex(chunk.address + chunk.data.size()) << " (" << chunk.data.size() << " 字节)"
          << std::endl;
    }
    std::cout << "[执行] 指令上限 " << std::dec << opt.maxInsns << ", 超时 " << opt.timeoutUs
        << " us, 停止地址 " << (opt.until != 0 ? hex(opt.until) : std::string("无"))
        << ", 外设 Hook " << (opt.periphHooks ? "开" : "关") << std::endl;
  }

  // ----------------------
  // 1. 初始化 Unicorn 并映射内存
  // ----------------------
  err = uc_open(ARCH, MODE, &uc);
  if (err) {
    std::cerr << "初始化 Unicorn Engine 失败. 错误码: " << err << " (" << uc_strerror(err) << ")"
        << std::endl;
    return EXIT_UC;
  }

  if (!mapMemory(uc)) {
    uc_close(uc);
    return EXIT_UC;
  }

  // ----------------------
  // 2. 固件文件写入
  // ----------------------
  if (!writeFirmware(uc, image, opt)) {
    uc_close(uc);
    return EXIT_FIRMWARE;
  }

  // ----------------------
  // 3. 读取启动向量和寄存器设置
  // ----------------------
  uint64_t start_pc = 0;
  if (!setupVectors(uc, image, opt.quiet, start_pc)) {
    uc_close(uc);
    return EXIT_FIRMWARE;
  }

  // ----------------------
  // 4.1 实例化和注册外设
  // ----------------------
  registerPeripherals(opt.quiet);

  // ----------------------
  // 4.2 设置 HardFault 重定向与 Hook
  // ----------------------
  if (!setup_VIRTUAL_HF_HANDLER(uc, opt.quiet)) {
    uc_close(uc);
    return EXIT_UC;
  }

  g_trace.opt = &opt;
  g_trace.instructions = 0;
  if (!setup_hooks(uc, opt)) {
    uc_close(uc);
    return EXIT_UC;
  }

  // ----------------------
  // 5. 启动模拟
  // ----------------------
  if (!opt.quiet) {
    std::cout << "--- 启动模拟执行 ---" << std::endl;
  }

  const auto start = std::chrono::high_resolution_clock::now();
  err = uc_emu_start(uc,
                     start_pc,
                     opt.until,
                     opt.timeoutUs,
                     static_cast<size_t>(opt.maxInsns));
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  std::cout << "执行时间: " << std::dec << duration.count() << " ns" << std::endl;
  if (opt.trace) {
    std::cout << "执行指令数: " << g_trace.instructions << std::endl;
  }

  int exitCode = EXIT_OK;
  if (err) {
    uint32_t final_pc = 0, final_lr = 0, final_sp = 0;
    uc_reg_read(uc, UC_ARM_REG_PC, &final_pc);
    uc_reg_read(uc, UC_ARM_REG_LR, &final_lr);
    uc_reg_read(uc, UC_ARM_REG_SP, &final_sp);
    std::cerr << "\n>>>  模拟执行失败!" << std::endl;
    std::cerr << "    PC地址: " << hex(final_pc) << " (终止点)" << std::endl;
    std::cerr << "    LR地址: " << hex(final_lr) << " (返回地址)" << std::endl;
    std::cerr << "    SP地址: " << hex(final_sp) << std::endl;
    std::cerr << "    错误码: " << std::dec << err << " (" << uc_strerror(err) << ")" << std::endl;
    exitCode = EXIT_EMU;
  } else {
    uint32_t final_pc = 0;
    uc_reg_read(uc, UC_ARM_REG_PC, &final_pc);
    // 区分几种正常停止的原因, 便于判断固件是否真的跑完
    // (Unicorn 对停止原因统一返回 UC_ERR_OK, 这里按 PC / 耗时 / 指令数反推)
    const double elapsedUs = static_cast<double>(duration.count()) / 1000.0;
    std::string reason;
    if (opt.until != 0 && (final_pc & ~1u) == (opt.until & ~1u)) {
      reason = "命中停止地址 " + hex(opt.until);
    } else if (opt.timeoutUs != 0 && elapsedUs >= static_cast<double>(opt.timeoutUs)) {
      reason = "达到模拟超时 " + std::to_string(opt.timeoutUs) + " us";
    } else if (opt.maxInsns != 0 &&
               (!opt.trace || g_trace.instructions >= opt.maxInsns)) {
      reason = "达到指令上限 " + std::to_string(opt.maxInsns) + " (可能尚未执行完)";
    } else {
      reason = "固件执行结束";
    }
    std::cout << "\n模拟执行结束: " << reason << std::endl;
    std::cout << "    最终 PC: " << hex(final_pc) << std::endl;
  }

  // ----------------------
  // 6. 清理
  // ----------------------
  uc_close(uc);
  return exitCode;
}
