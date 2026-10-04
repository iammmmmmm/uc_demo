// SysTickModel.cc
#include "SysTickModel.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

// 注入中断时需要保存/恢复的寄存器, 与 Cortex-M 硬件异常压栈的内容一致:
// r0-r3, r12, lr, xpsr。r4-r11 由被调函数(中断服务函数)自行保存, 符合 AAPCS。
// PC 单独处理 (见 injectSysTick 的 interruptedPc)。
const int kSavedRegs[] = {
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R12, UC_ARM_REG_LR, UC_ARM_REG_CPSR,
};
constexpr size_t kSavedRegCount = sizeof(kSavedRegs) / sizeof(kSavedRegs[0]);

std::string hex(uint64_t value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << value;
    return oss.str();
}

}  // namespace

SysTickModel *SysTickModel::s_active = nullptr;

SysTickModel::SysTickModel(uc_engine *uc,
                           uint32_t vectorBase,
                           uint32_t trampolineAddr,
                           std::initializer_list<std::pair<uint64_t, uint64_t>> codeRanges)
    : m_uc(uc), m_vectorBase(vectorBase), m_trampolineAddr(trampolineAddr) {
    m_codeRanges.assign(codeRanges.begin(), codeRanges.end());
}

bool SysTickModel::install(std::string &error) {
    s_active = this;
    // 1. 写跳板: 一条 B . (0xE7FE), 只作为 until 目标, 不会被真正执行
    const uint16_t branchSelf = 0xE7FE;
    uc_err err = uc_mem_write(m_uc, m_trampolineAddr, &branchSelf, sizeof(branchSelf));
    if (err != UC_ERR_OK) {
        error = "无法写入 SysTick 中断跳板到 " + hex(m_trampolineAddr) + ": " +
                uc_strerror(err);
        return false;
    }

    // 2. 读 SysTick 向量 (异常号 15 -> 偏移 0x3C)
    uint32_t handler = 0;
    err = uc_mem_read(m_uc, m_vectorBase + kVectorOffset, &handler, sizeof(handler));
    if (err != UC_ERR_OK) {
        error = "无法读取 SysTick 向量 " +
                hex(m_vectorBase + kVectorOffset) + ": " + uc_strerror(err);
        return false;
    }
    // 向量为 0 表示该固件没用 SysTick 中断: 仍然计数, 但不注入, 不算错误
    m_handler = handler & ~1u;

    // 3. 缓存 SysTick 寄存器初值 (映射后为 0, 即未使能)
    m_ctrl = readReg(0x00);
    m_load = readReg(0x04) & 0xFFFFFFu;
    m_val = readReg(0x08) & 0xFFFFFFu;

    // 4. 真异常入口模式: 准备 EXC_RETURN 落点。
    //    handler 末尾的 `bx lr`(LR = 0xFFFFFFF9) 会把 PC 送到 0xFFFFFFF8, 那里放一条 B .
    //    并挂上指令 Hook —— 于是我们能在"异常返回"这一步停下, 手动弹栈恢复现场。
    std::vector<std::pair<uint64_t, uint64_t>> ranges = m_codeRanges;
    if (m_mode == Mode::Exception) {
        const uc_err mapErr = uc_mem_map(m_uc, kExcReturnPage, kExcReturnPageSize, UC_PROT_ALL);
        if (mapErr != UC_ERR_OK && mapErr != UC_ERR_MAP) { // 已经映射过就忽略
            error = "映射 EXC_RETURN 跳板页失败: " + std::string(uc_strerror(mapErr));
            return false;
        }
        const uint16_t branchSelf = 0xE7FE;
        err = uc_mem_write(m_uc, kExcReturnTrampoline, &branchSelf, sizeof(branchSelf));
        if (err != UC_ERR_OK) {
            error = "写入 EXC_RETURN 跳板失败: " + std::string(uc_strerror(err));
            return false;
        }
        ranges.emplace_back(kExcReturnTrampoline, kExcReturnTrampoline + 1);
    }

    // 5. 指令级 Hook: 推进 SysTick 计数并请求注入
    for (const auto &[start, end] : ranges) {
        uc_hook hook = 0;
        err = uc_hook_add(m_uc,
                          &hook,
                          UC_HOOK_CODE,
                          reinterpret_cast<void *>(&SysTickModel::codeHook),
                          this,
                          start,
                          end);
        if (err != UC_ERR_OK) {
            error = "uc_hook_add (SysTick CODE) 失败: " + std::string(uc_strerror(err));
            return false;
        }
    }

    // 5. 内存写 Hook: 同步固件对 CTRL/LOAD/VAL 的读写改
    uc_hook memHook = 0;
    err = uc_hook_add(m_uc,
                      &memHook,
                      UC_HOOK_MEM_WRITE,
                      reinterpret_cast<void *>(&SysTickModel::memWriteHook),
                      this,
                      static_cast<uint64_t>(kBaseAddress),
                      static_cast<uint64_t>(kBaseAddress) + 0x10 - 1);
    if (err != UC_ERR_OK) {
        error = "uc_hook_add (SysTick MEM_WRITE) 失败: " + std::string(uc_strerror(err));
        return false;
    }

    // 6. SRAM 写 Hook: 捕获 APM_Delay() 写下的延时计数变量(__delayCnt)地址。
    //    有了它就能在"不调用固件 ISR"的方式下直接递减该计数, 不依赖向量表内容。
    uc_hook sramHook = 0;
    err = uc_hook_add(m_uc,
                      &sramHook,
                      UC_HOOK_MEM_WRITE,
                      reinterpret_cast<void *>(&SysTickModel::sramWriteHook),
                      this,
                      static_cast<uint64_t>(kSramStart),
                      static_cast<uint64_t>(kSramStart) + kSramSize - 1);
    if (err != UC_ERR_OK) {
        error = "uc_hook_add (SRAM MEM_WRITE) 失败: " + std::string(uc_strerror(err));
        return false;
    }

    return true;
}

uint32_t SysTickModel::readReg(uint32_t offset) const {
    uint32_t value = 0;
    if (uc_mem_read(m_uc, kBaseAddress + offset, &value, sizeof(value)) != UC_ERR_OK) {
        return 0;
    }
    return value;
}

void SysTickModel::writeReg(uint32_t offset, uint32_t value) {
    uc_mem_write(m_uc, kBaseAddress + offset, &value, sizeof(value));
}

// ---------------------------------------------------------------------------
// 指令级 Hook: 每执行一条指令算作 1 个时钟, 推进 SysTick
//
// 返回 false 会让 uc_emu_start 立即停下(该指令尚未执行), 由 run() 负责注入中断。
// ---------------------------------------------------------------------------
bool SysTickModel::codeHook(uc_engine *uc, uint64_t address, uint32_t size, void *userData) {
    auto *self = static_cast<SysTickModel *>(userData);
    if (self == nullptr) {
        return true;
    }
    self->m_instructions++;

    // 看门狗: 进了异常却长时间不返回 —— 把现场报出来, 而不是干等到超时
    if (self->m_exceptionDepth > 0 &&
        self->m_instructions - self->m_exceptionEnteredAt > kExceptionWarnInstructions) {
        if (!self->m_exceptionWedged) {
            self->m_exceptionWedged = true;
            uint32_t pc = 0, sp = 0, lr = 0, ipsr = 0;
            uc_reg_read(uc, UC_ARM_REG_PC, &pc);
            uc_reg_read(uc, UC_ARM_REG_SP, &sp);
            uc_reg_read(uc, UC_ARM_REG_LR, &lr);
            uc_reg_read(uc, UC_ARM_REG_CPSR, &ipsr);
            self->m_lastError = "中断服务函数未返回(看门狗): PC=" + hex(pc) + " SP=" + hex(sp) +
                " LR=" + hex(lr) + " xPSR=" + hex(ipsr) + " CTRL=" + hex(self->readReg(0x00)) +
                " 入口=" + hex(self->m_handler) + " 异常次数=" + std::to_string(self->m_exceptions);
        }
        return false; // 停下, 由 run() 报错退出
    }

    // 异常返回落点: `bx lr` 且 LR = EXC_RETURN 时 PC 会跳到这里 -> 停下来弹栈
    if (address == kExcReturnTrampoline) {
        if (self->m_exceptionDepth > 0) {
            self->m_pendingExcReturn = true;
            return false;
        }
        return true; // 不是我们的异常(固件乱跳到这儿), 让它自己撞上那条 B .
    }

    // 注入期间只计数, 不再触发新中断, 否则会在执行中断服务函数时把自己打断
    if (self->m_injecting) {
        return true;
    }
    if ((self->m_ctrl & kCtrlEnable) == 0) {
        return true;
    }

    if (self->m_honorLoad) {
        // 忠实模式: 按固件设置的 LOAD 计满一拍
        if (self->m_load == 0) {
            return true;
        }
        // 固件写 VAL = 0 (APM_Delay() 每次都这么做) 表示重新计时:
        // 硬件会在下一个时钟从 LOAD 重新装载, 这里必须补上, 否则计数器永远停在 0。
        if (self->m_val == 0) {
            self->m_val = self->m_load & 0xFFFFFFu;
            self->writeReg(0x08, self->m_val);
            return true;
        }
        if (--self->m_val != 0) {
            return true;
        }
    }
    // 默认演示速率: 每条指令就是一拍 (忽略 LOAD), 否则 48 拍/微秒会让长延时要跑上千万条指令

    // --- SysTick 到点 ---
    self->m_val = self->m_load & 0xFFFFFFu;
    self->m_ctrl |= kCtrlCountFlag;
    self->writeReg(0x00, self->m_ctrl); // COUNTFLAG 供固件查询
    self->writeReg(0x08, self->m_val);
    self->m_systickInterrupts++; // 节拍计数

    if ((self->m_ctrl & kCtrlTickInt) == 0) {
        return true; // 没开中断, 只计数
    }

    switch (self->m_mode) {
        case Mode::Direct:
            // 不调用固件 ISR, 直接做它唯一的那件事 (__delayCnt--)
            self->tickDelayCounter();
            return true;

        case Mode::Subroutine:
            if (self->m_handler == 0) {
                return true;
            }
            self->m_pending = true;
            return false; // 请求注入

        case Mode::Exception:
            if (self->m_handler == 0) {
                return true;
            }
            if (self->m_exceptionDepth > 0) {
                // 正在执行上一个中断: 挂起, 等它返回后再触发 (相当于尾链)
                self->m_exceptionPended = true;
                return true;
            }
            self->m_pendingException = true;
            return false; // 先停下, 由 run() 压栈并跳到 handler
    }
    return true;
}

// ---------------------------------------------------------------------------
// 真异常入口: 手动做硬件做的事
//   1. 把 {r0-r3, r12, lr, pc, xpsr} 压到当前栈 (SP -= 32)
//   2. LR = EXC_RETURN(0xFFFFFFF9), PC = 向量, xPSR.IPSR = 15 (SysTick)
// 之后 handler 就是一段普通代码; 它 `bx lr` 时会跳到 EXC_RETURN 跳板, 由 exitException() 弹栈。
// 未实现: 优先级/PRIMASK 屏蔽、PSP(线程用 PSP 的 RTOS)、惰性压栈、精确的压栈时序。
// ---------------------------------------------------------------------------
bool SysTickModel::enterException(std::string &error) {
    uint32_t sp = 0, pc = 0, lr = 0, xpsr = 0;
    uint32_t r0 = 0, r1 = 0, r2 = 0, r3 = 0, r12 = 0;
    const int regs[] = {UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_LR, UC_ARM_REG_CPSR,
                        UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
                        UC_ARM_REG_R12};
    uint32_t values[9] = {};
    for (size_t i = 0; i < 9; ++i) {
        if (uc_reg_read(m_uc, regs[i], &values[i]) != UC_ERR_OK) {
            error = "读取现场失败";
            return false;
        }
    }
    sp = values[0];
    pc = values[1];
    lr = values[2];
    xpsr = values[3];
    r0 = values[4];
    r1 = values[5];
    r2 = values[6];
    r3 = values[7];
    r12 = values[8];

    const uint32_t frame[8] = {r0, r1, r2, r3, r12, lr, pc, xpsr};
    const uint32_t newSp = sp - 32;
    if (uc_mem_write(m_uc, newSp, frame, sizeof(frame)) != UC_ERR_OK) {
        error = "压入异常栈帧失败 (SP = " + hex(newSp) + ")";
        return false;
    }
    if (uc_reg_write(m_uc, UC_ARM_REG_SP, &newSp) != UC_ERR_OK) {
        error = "写入 SP 失败";
        return false;
    }
    const uint32_t excReturn = kExcReturnThreadMsp;
    uc_reg_write(m_uc, UC_ARM_REG_LR, &excReturn);

    const uint32_t handlerPc = m_handler | 1u;
    if (uc_reg_write(m_uc, UC_ARM_REG_PC, &handlerPc) != UC_ERR_OK) {
        error = "写入 PC(handler) 失败";
        return false;
    }
    // IPSR = 15 (SysTick); xPSR 其余位保留(含 Thumb 位)
    const uint32_t newXpsr = (xpsr & ~0x1FFu) | 15u;
    uc_reg_write(m_uc, UC_ARM_REG_CPSR, &newXpsr);

    m_exceptionDepth++;
    m_exceptions++;
    m_exceptionEnteredAt = m_instructions;
    m_exceptionWedged = false;
    if (m_exceptions <= 3) {
        std::cerr << "[时钟] 异常进入 #" << m_exceptions << ": 指令#" << m_instructions
                  << " 被中断 PC=" << hex(pc) << " SP=" << hex(sp) << " -> " << hex(newSp)
                  << ", handler=" << hex(m_handler) << ", 累计节拍=" << m_systickInterrupts
                  << std::endl;
    }
    return true;
}

// 异常返回: 从栈帧恢复现场, PC 回到被中断的指令
bool SysTickModel::exitException(std::string &error) {
    uint32_t sp = 0;
    if (uc_reg_read(m_uc, UC_ARM_REG_SP, &sp) != UC_ERR_OK) {
        error = "读取 SP 失败";
        return false;
    }
    uint32_t frame[8] = {};
    if (uc_mem_read(m_uc, sp, frame, sizeof(frame)) != UC_ERR_OK) {
        error = "读取异常栈帧失败 (SP = " + hex(sp) + ")";
        return false;
    }
    const uint32_t newSp = sp + 32;
    const uint32_t r0 = frame[0], r1 = frame[1], r2 = frame[2], r3 = frame[3];
    const uint32_t r12 = frame[4], lr = frame[5], resumePc = frame[6], xpsr = frame[7];
    uc_reg_write(m_uc, UC_ARM_REG_R0, &r0);
    uc_reg_write(m_uc, UC_ARM_REG_R1, &r1);
    uc_reg_write(m_uc, UC_ARM_REG_R2, &r2);
    uc_reg_write(m_uc, UC_ARM_REG_R3, &r3);
    uc_reg_write(m_uc, UC_ARM_REG_R12, &r12);
    uc_reg_write(m_uc, UC_ARM_REG_LR, &lr);
    uc_reg_write(m_uc, UC_ARM_REG_SP, &newSp);
    uc_reg_write(m_uc, UC_ARM_REG_CPSR, &xpsr);
    const uint32_t pcWrite = resumePc & ~1u;
    if (uc_reg_write(m_uc, UC_ARM_REG_PC, &pcWrite) != UC_ERR_OK) {
        error = "恢复 PC 失败";
        return false;
    }

    if (m_exceptionDepth > 0) {
        m_exceptionDepth--;
    }
    if (m_exceptions <= 3) {
        std::cerr << "[时钟] 异常返回 #" << m_exceptions << ": 指令#" << m_instructions
                  << " 恢复到 PC=" << hex(resumePc) << " SP=" << hex(newSp)
                  << ", 累计节拍=" << m_systickInterrupts << std::endl;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 未映射访问认领: 异常返回时 `bx lr` 跳到 0xFFFFFFF8, 那一页不一定可取指,
// 于是 Unicorn 会报未映射访问。这里认定它就是"异常返回", 静默停下让 run() 弹栈。
// ---------------------------------------------------------------------------
bool SysTickModel::handleUnmappedFetch(uint64_t address) {
    if (m_exceptionDepth == 0) {
        return false; // 不在异常里, 交给上层按真实错误处理
    }
    m_unmappedInException++;
    if ((address & ~1ull) == static_cast<uint64_t>(kExcReturnTrampoline)) {
        m_pendingExcReturn = true;
        return true; // 认领
    }
    if (m_unmappedInException <= 3) {
        uint32_t pc = 0;
        uc_reg_read(m_uc, UC_ARM_REG_PC, &pc);
        std::cerr << "[时钟] 异常处理中出现未映射访问: 地址 " << hex(address) << " (出错指令 PC "
                  << hex(pc) << ") —— 这不是 EXC_RETURN 落点 " << hex(kExcReturnTrampoline) << std::endl;
    }
    return false;
}

// 直接做中断服务函数的效果: 把固件写下的延时计数减 1。
// 用缓存值判断, 所以"不在延时中"(计数为 0)时节拍几乎不花代价。
void SysTickModel::tickDelayCounter() {
    if (m_delayCntAddr == 0 || m_delayCntValue == 0) {
        return; // 还没学到地址, 或当前没有正在进行的延时
    }
    m_delayCntValue--;
    uc_mem_write(m_uc, m_delayCntAddr, &m_delayCntValue, sizeof(m_delayCntValue));
}

// 固件写 VAL = 0 之后, 紧接着会写 __delayStack(1 字节) 和 __delayCnt(4 字节);
// 因此只在"刚写完 VAL=0 的十几条指令内"、且是 4 字节的 SRAM 写, 才认定为延时计数器。
// (这样能排除 SysTick_Config() 里那次 VAL=0 之后隔得很远的无关全局变量写。)
void SysTickModel::sramWriteHook(uc_engine *uc,
                                 uc_mem_type type,
                                 uint64_t address,
                                 int size,
                                 int64_t value,
                                 void *userData) {
    auto *self = static_cast<SysTickModel *>(userData);
    if (self == nullptr) {
        return;
    }
    // 固件每次开始一次新延时都会重写这个变量, 同步到缓存
    if (self->m_delayCntAddr != 0 && size == 4 &&
        static_cast<uint32_t>(address) == self->m_delayCntAddr) {
        self->m_delayCntValue = static_cast<uint32_t>(value);
    }
    if (!self->m_armDelayCapture) {
        return;
    }
    if (self->m_instructions - self->m_armAtInstruction > kDelayCaptureWindow) {
        self->m_armDelayCapture = false; // 太远了, 不是延时计数
        return;
    }
    if (size != 4) {
        return; // __delayStack 是 1 字节
    }
    self->m_delayCntAddr = static_cast<uint32_t>(address);
    self->m_delayCntValue = static_cast<uint32_t>(value);
    self->m_armDelayCapture = false;
}

// 固件对 SysTick 寄存器的写入 (含 |= / &= 这类读-改-写) 要同步到缓存
void SysTickModel::memWriteHook(uc_engine *uc,
                                uc_mem_type type,
                                uint64_t address,
                                int size,
                                int64_t value,
                                void *userData) {
    auto *self = static_cast<SysTickModel *>(userData);
    if (self == nullptr || size != 4) {
        return;
    }
    const auto offset = static_cast<uint32_t>(address - kBaseAddress);
    switch (offset) {
        case 0x00:
            self->m_ctrl = static_cast<uint32_t>(value);
            // APM_Delay() 的特征序列: CTRL 清 ENABLE -> VAL = 0 -> CTRL 置 ENABLE -> 写延时计数
            // (SysTick_Config() 只写 LOAD -> VAL -> CTRL, 没有"先清 ENABLE", 因此不会误arm)
            self->m_sawCtrlDisable = (self->m_ctrl & kCtrlEnable) == 0;
            break;
        case 0x04:
            self->m_load = static_cast<uint32_t>(value) & 0xFFFFFFu;
            break;
        case 0x08:
            // 固件清零 VAL 表示重新计时; APM_Delay() 在这里之后马上会写延时计数变量
            self->m_val = static_cast<uint32_t>(value) & 0xFFFFFFu;
            self->m_armDelayCapture = self->m_sawCtrlDisable;
            self->m_armAtInstruction = self->m_instructions;
            self->m_sawCtrlDisable = false;
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// 注入一次 SysTick 中断
//
// 做法: 保存现场 -> 让 LR 指向跳板 -> 从处理器入口执行, 到跳板处停下 -> 恢复现场。
// 这样不必模拟硬件异常压栈与 EXC_RETURN, 只要求处理器按常规方式经 LR 返回
// (`bx lr` 或 `pop {..., pc}` 均可)。
// ---------------------------------------------------------------------------
bool SysTickModel::injectSysTick(std::string &error) {
    uint32_t saved[kSavedRegCount] = {};
    for (size_t i = 0; i < kSavedRegCount; ++i) {
        if (uc_reg_read(m_uc, kSavedRegs[i], &saved[i]) != UC_ERR_OK) {
            error = "读取被中断现场失败";
            return false;
        }
    }
    uint32_t interruptedPc = 0;
    if (uc_reg_read(m_uc, UC_ARM_REG_PC, &interruptedPc) != UC_ERR_OK) {
        error = "读取被中断的 PC 失败";
        return false;
    }

    // 处理器入口 (Thumb) 与返回跳板
    const uint32_t entry = m_handler | 1u;
    const uint32_t trampoline = m_trampolineAddr | 1u;

    m_injecting = true;
    uc_reg_write(m_uc, UC_ARM_REG_LR, &trampoline);
    uc_reg_write(m_uc, UC_ARM_REG_PC, &entry);

    const uc_err err =
        uc_emu_start(m_uc, entry, trampoline, 0, kHandlerInstructionLimit);

    m_injecting = false;

    // 恢复现场 (包括被中断的 PC), 无论处理器是否正常返回
    for (size_t i = 0; i < kSavedRegCount; ++i) {
        uc_reg_write(m_uc, kSavedRegs[i], &saved[i]);
    }
    uc_reg_write(m_uc, UC_ARM_REG_PC, &interruptedPc);

    if (err != UC_ERR_OK) {
        error = "执行 SysTick 中断服务函数出错: " + std::string(uc_strerror(err));
        return false;
    }

    uint32_t stoppedAt = 0;
    uc_reg_read(m_uc, UC_ARM_REG_PC, &stoppedAt);
    if ((stoppedAt & ~1u) != m_trampolineAddr) {
        // 处理器没有经 LR 返回 (例如内部改写了 LR、或陷入死循环)
        error = "SysTick 中断服务函数未能在 " +
                std::to_string(kHandlerInstructionLimit) + " 条指令内经 LR 返回 (PC = " +
                hex(stoppedAt) + ")。该注入方式要求处理器按常规方式返回; 若确实不满足, " +
                "需改用真实的异常压栈 / EXC_RETURN 实现";
        return false;
    }

    // 节拍计数已在 codeHook 里统计, 这里不重复计数
    return true;
}

// ---------------------------------------------------------------------------
// 推进模拟: 与 uc_emu_start 语义一致, 但会在 SysTick 到点时注入中断
// ---------------------------------------------------------------------------
uc_err SysTickModel::run(uint64_t startPc, uint64_t until, uint64_t timeoutUs, uint64_t maxInsns) {
    const auto begin = std::chrono::steady_clock::now();
    const auto elapsedUs = [&begin]() -> uint64_t {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - begin)
                .count());
    };

    uint64_t pc = startPc;
    std::string error;

    while (true) {
        // 指令预算: 0 表示不限制
        uint64_t remaining = 0;
        if (maxInsns != 0) {
            if (m_instructions >= maxInsns) {
                return UC_ERR_OK; // 达到指令上限
            }
            remaining = maxInsns - m_instructions;
        }

        // 单轮长度取"剩余预算"与"切片上限"的较小者, 使中断注入足够及时
        uint64_t slice = kSliceInstructions;
        if (remaining != 0 && remaining < slice) {
            slice = remaining;
        }

        uint64_t remainingUs = 0;
        if (timeoutUs != 0) {
            const uint64_t used = elapsedUs();
            if (used >= timeoutUs) {
                return UC_ERR_OK; // 达到模拟超时
            }
            remainingUs = timeoutUs - used;
        }

        const uint64_t before = m_instructions;
        m_pending = false;
        m_pendingException = false;
        m_pendingExcReturn = false;
        const uc_err err = uc_emu_start(m_uc, pc, until, remainingUs, slice);
        const uint64_t executed = m_instructions - before;

        if (m_exceptionWedged) {
            // 看门狗已在 codeHook 里填好 m_lastError
            return UC_ERR_EXCEPTION;
        }
        if (m_pendingExcReturn) {
            // 异常返回: 弹栈恢复现场 (恢复后的 PC 由下面统一读取续跑)
            if (!exitException(error)) {
                m_lastError = error;
                return UC_ERR_EXCEPTION;
            }
            // 尾链: 处理中断期间又到点, 返回后立刻再进一次
            if (m_exceptionPended) {
                m_exceptionPended = false;
                if (!enterException(error)) {
                    m_lastError = error;
                    return UC_ERR_EXCEPTION;
                }
            }
        } else if (m_pendingException) {
            // 真异常入口: 压栈 + 跳到 handler
            if (!enterException(error)) {
                m_lastError = error;
                return UC_ERR_EXCEPTION;
            }
        } else if (m_pending) {
            if (!injectSysTick(error)) {
                m_lastError = error;
                return UC_ERR_EXCEPTION;
            }
        } else if (err != UC_ERR_OK) {
            return err; // 真实错误 (未映射访问 / 指令非法等), 交给上层报告
        } else if (executed < slice) {
            // 本轮提前结束: 命中 until, 或无法继续执行 —— 与 uc_emu_start 的语义一致
            return UC_ERR_OK;
        }

        // 继续下一轮: PC 取当前值 (注入后即为被中断的指令处)
        uint32_t currentPc = 0;
        if (uc_reg_read(m_uc, UC_ARM_REG_PC, &currentPc) != UC_ERR_OK) {
            m_lastError = "读取 PC 失败";
            return UC_ERR_EXCEPTION;
        }
        pc = static_cast<uint64_t>(currentPc) | 1ull;
    }
}
