// SysTickModel.h
#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include <unicorn/unicorn.h>

/**
 * @brief Cortex-M0+ SysTick 时基 + SysTick 异常注入。
 *
 * 为什么需要它: 固件用 `bsp_delay.c` 做延时 —— `APM_Delay()` 把计数写入 `__delayCnt`
 * 后 `while (__delayCnt > 0);` 空转, 而 `__delayCnt` **只由 SysTick 中断服务函数
 * (`SysTick_Handler` -> `APM_DelayIsr()`) 递减**。而 PPB 区域在本模拟器里只是普通
 * RAM, 不会产生任何中断, 所以 `Delay_us()/Delay_ms()` 会永久空转, 固件根本走不到
 * 主循环, 更谈不上读取 GPIO 输入。
 *
 * 工作方式:
 *   1. 在 Flash/ROM 上挂一个指令级 Hook, 按 "1 条指令 = 1 个时钟" 推进 SysTick 的
 *      递减计数 (VAL), 并对固件读写 CTRL/LOAD/VAL 的内存访问做同步;
 *   2. VAL 减到 0 时置 CTRL.COUNTFLAG、把 VAL 重载为 LOAD; 若 CTRL.TICKINT 置位,
 *      Hook 返回 false 中断本轮模拟;
 *   3. `run()` 检测到该情况后注入中断: 保存现场(r0-r12/SP/LR/PC/CPSR), 把
 *      `SysTick_Handler` 当作子程序执行一次, 再恢复现场。
 *
 * 注入用的是 `uc_emu_start` 的 `until` 参数: 令 LR 指向 SRAM 里的一条 `B .` 跳板,
 * 处理器按常规方式返回(`bx lr` 或 `pop {..., pc}`)时一跳到这里模拟就停下。因此
 * **不需要实现真实的异常压栈与 EXC_RETURN**, 只要求处理器经 LR 返回 —— 本固件的
 * `SysTick_Handler` 满足该条件; 不满足时会报错停下, 而不是悄悄跑错。
 */
class SysTickModel {
  public:
    // SysTick 位于 PPB 区域
    static constexpr uint32_t kBaseAddress = 0xE000E010;
    // SRAM 范围: 用来捕获 APM_Delay() 写下的延时计数变量地址
    static constexpr uint32_t kSramStart = 0x20000000;
    static constexpr uint32_t kSramSize = 0x1000;
    // 寄存器偏移
    static constexpr uint32_t kCtrlOffset = 0x00;
    static constexpr uint32_t kLoadOffset = 0x04;
    static constexpr uint32_t kValOffset = 0x08;
    // CTRL 位定义
    static constexpr uint32_t kCtrlEnable = 1u << 0;
    static constexpr uint32_t kCtrlTickInt = 1u << 1;
    static constexpr uint32_t kCtrlClkSource = 1u << 2;
    static constexpr uint32_t kCtrlCountFlag = 1u << 16;
    // 异常号 15 (SysTick) 在向量表中的偏移
    static constexpr uint32_t kVectorOffset = 0x3C;

    /**
     * @param uc 已初始化并映射好内存的 Unicorn 实例
     * @param vectorBase 向量表基地址 (SysTick 向量位于 vectorBase + 0x3C)
     * @param trampolineAddr 注入用跳板地址, 需落在已映射且可执行的区域(如 SRAM)
     * @param codeRanges 需要挂载指令级 Hook 的代码区间 [start, end] 列表
     */
    SysTickModel(uc_engine *uc,
                 uint32_t vectorBase,
                 uint32_t trampolineAddr,
                 std::initializer_list<std::pair<uint64_t, uint64_t>> codeRanges);

    /**
     * @brief 安装 Hook、准备跳板并解析 SysTick 向量。
     * @param error 失败原因
     */
    bool install(std::string &error);

    /**
     * @brief 代替 uc_emu_start 推进模拟: 按需注入 SysTick 中断。
     *
     * 语义与 uc_emu_start 一致; 区别是需要注入中断时会自动拆成多轮执行。
     * @param maxInsns 0 表示不限制
     * @param timeoutUs 0 表示不限制
     */
    uc_err run(uint64_t startPc, uint64_t until, uint64_t timeoutUs, uint64_t maxInsns);

    /**
     * @brief 时钟到点时怎么"执行"中断服务函数。
     */
    enum class Mode {
        Direct,     // 默认: 不调用 ISR, 直接做它唯一干的事 (__delayCnt--), 最快最稳
        Subroutine, // 把 SysTick_Handler 当子程序调用(LR 指向 SRAM 跳板), 要求它经 LR 返回
        Exception,  // 真异常入口: 手动压栈 + EXC_RETURN 跳板, handler 当普通代码执行
    };
    void setMode(Mode mode) { m_mode = mode; }
    [[nodiscard]] Mode mode() const { return m_mode; }
    [[nodiscard]] uint64_t exceptions() const { return m_exceptions; }

    /**
     * @brief 供 main.cpp 的"未映射访问" Hook 调用。
     *
     * handler 的 `bx lr`(LR = EXC_RETURN) 会跳到 0xFFFFFFF8。那一页并不保证可取指
     * (实测 Unicorn 会在这里报未映射访问), 所以在这里认领它: 认定这是"异常返回",
     * 静默停下本轮模拟, 由 run() 弹栈恢复现场。
     * 返回 true 表示已认领(调用方不要报错)。
     */
    bool handleUnmappedFetch(uint64_t address);

    // 当前活动的时钟模型 (供 main.cpp 的 Hook 查询)
    static SysTickModel *active() { return s_active; }
    static SysTickModel *s_active;

    // Cortex-M 异常返回魔数 (线程模式 / 使用 MSP)
    static constexpr uint32_t kExcReturnThreadMsp = 0xFFFFFFF9u;
    // LR = EXC_RETURN 时, `bx lr` 会把 PC 送到这里 (EXC_RETURN & ~1)
    static constexpr uint32_t kExcReturnTrampoline = 0xFFFFFFF8u;
    // 给上面这个落点准备的一页内存 (里面放一条 B .)
    static constexpr uint32_t kExcReturnPage = 0xFFFFF000u;
    static constexpr uint32_t kExcReturnPageSize = 0x1000u;

    /**
     * @brief 是否按固件设置的 LOAD 重载值计时。
     *
     * false(默认): 每条指令算一拍, 忽略 LOAD。固件把 SysTick 配成 48MHz/1MHz=48 拍一微秒,
     *   忠实计时的话 `Delay_ms(300)` 要跑 1440 万条指令(带指令 Hook 时约 15 秒), demo 不可用。
     * true: 按 LOAD 计满一拍, 时间忠实但慢得多。
     */
    void setHonorLoad(bool honor) { m_honorLoad = honor; }

    // 统计 (供 --trace / 报告使用)
    [[nodiscard]] uint64_t instructions() const { return m_instructions; }
    [[nodiscard]] uint64_t interrupts() const { return m_systickInterrupts; }
    [[nodiscard]] bool handlerAvailable() const { return m_handler != 0; }
    [[nodiscard]] uint32_t handlerAddress() const { return m_handler; }
    // 捕获到的延时计数变量地址 (0 = 还没学到)
    [[nodiscard]] uint32_t delayCounterAddress() const { return m_delayCntAddr; }
    // run() 因注入失败而中止时的原因
    [[nodiscard]] const std::string &lastError() const { return m_lastError; }

    // 单轮模拟最多执行的指令数, 用来限制中断注入的响应延迟
    static constexpr uint64_t kSliceInstructions = 200000;
    // 中断服务函数被视为子程序执行时的指令上限, 防止其永不返回时卡死
    static constexpr uint64_t kHandlerInstructionLimit = 10000;
    // 写完 SysTick->VAL=0 之后, 多少条指令内的 4 字节 SRAM 写才算延时计数器
    static constexpr uint64_t kDelayCaptureWindow = 16;
    // 中断里跑了这么多条指令还没返回, 判定为卡死并报出当前现场
    static constexpr uint64_t kExceptionWarnInstructions = 200000;

  private:
    static bool codeHook(uc_engine *uc, uint64_t address, uint32_t size, void *userData);
    static void memWriteHook(uc_engine *uc,
                             uc_mem_type type,
                             uint64_t address,
                             int size,
                             int64_t value,
                             void *userData);
    static void sramWriteHook(uc_engine *uc,
                              uc_mem_type type,
                              uint64_t address,
                              int size,
                              int64_t value,
                              void *userData);

    // 把 SysTick_Handler 当子程序执行一次; 出错返回 false
    bool injectSysTick(std::string &error);

    // 真异常入口: 压栈 + 置 EXC_RETURN, 让 handler 当普通代码跑
    bool enterException(std::string &error);
    // 异常返回: 从栈帧恢复现场 (PC 回到被中断的指令)
    bool exitException(std::string &error);

    // 直接做 ISR 的效果: 延时计数 -1
    void tickDelayCounter();

    // SysTick 寄存器读写 (CTRL/LOAD/VAL 兼顾固件的读-改-写)
    [[nodiscard]] uint32_t readReg(uint32_t offset) const;
    void writeReg(uint32_t offset, uint32_t value);

    uc_engine *m_uc = nullptr;
    const uint32_t m_vectorBase;
    const uint32_t m_trampolineAddr;
    uint32_t m_handler = 0;
    std::vector<std::pair<uint64_t, uint64_t>> m_codeRanges;

    // SysTick 状态
    uint32_t m_ctrl = 0;
    uint32_t m_load = 0;
    uint32_t m_val = 0;

    // 时钟推进方式与延时计数变量
    Mode m_mode = Mode::Direct;
    bool m_honorLoad = false;       // true = 按固件设置的 LOAD 计满一拍 (时间忠实但很慢)
    bool m_armDelayCapture = false; // 固件刚写过 VAL=0, 下一个 4 字节 SRAM 写就是延时计数
    bool m_sawCtrlDisable = false;  // 刚见过 CTRL 里 ENABLE 被清零 (APM_Delay 的特征序列)
    uint64_t m_armAtInstruction = 0; // 上面那次 VAL=0 发生在第几条指令 (用来排除远距离的无关写)
    uint32_t m_delayCntAddr = 0;    // 捕获到的延时计数变量地址
    uint32_t m_delayCntValue = 0;   // 它的缓存值: 固件写入时更新, 节拍时递减

    bool m_pending = false;   // Hook 请求注入 SysTick 中断 (Subroutine 模式)
    bool m_injecting = false; // 正在执行中断服务函数, 期间不再触发新的注入
    bool m_pendingException = false;  // Hook 请求进入异常 (Exception 模式)
    bool m_pendingExcReturn = false;  // 已到达 EXC_RETURN 跳板, 需要弹栈
    uint32_t m_exceptionDepth = 0;    // 异常嵌套深度 (>0 表示正在执行中断服务函数)
    bool m_exceptionPended = false;   // 处理中又到点: 等返回后补触发 (尾链)
    uint64_t m_exceptionEnteredAt = 0; // 最近一次进异常的指令序号 (看门狗用)
    bool m_exceptionWedged = false;    // 中断里卡死了
    uint64_t m_unmappedInException = 0; // 异常处理中遇到的未映射访问次数 (诊断用)

    uint64_t m_instructions = 0;
    uint64_t m_systickInterrupts = 0;
    uint64_t m_exceptions = 0;
    std::string m_lastError;
};
