# uc_demo 工程现状与剩余工作

> 目标固件源码见 [`FIRMWARE.md`](FIRMWARE.md)。本文只写"做了什么、还差什么、怎么验证"。

---

## 1. 目标（已冻结）

让目标固件（IR + 薄膜键盘 + PS/2 从机）在没有硬件的情况下跑起来，只做三件事：

1. **USART 输出**：固件 `printf` 的字符出现在 stdout；
2. **GPIO 读时注入**：用户给的电平就是固件读到的 `DIN`；
3. **按指令数推进的时钟**：1 指令 = 1 时钟，驱动 SysTick。

不做：真实异常压栈 / `EXC_RETURN`、SysTick 以外的中断源、USART 接收、GPIO 中断、其余外设行为化、QEMU 迁移。

---

## 2. 已完成

| 能力 | 位置 | 说明 |
|---|---|---|
| USART 字符输出 | `peripheral/Usart.cc` | 接受 8/16 位写（`USART_TxData8` 是 8 位写 `DATA_B.DATA`，原先 `size != 4` 直接丢弃）；写 `DATA` 低 8 位即输出到 stdout；`STS` 保持复位值 `0xC0`，固件的 `TXBE` 轮询立刻通过 |
| GPIO 读时注入 | `peripheral/GPIO_Port.cc/.h` | `DIN` 读取时动态合成：**外部注入 > 输出回读 DOUT > 上拉=1 > 浮空**；`driveInput()` 注入/释放；浮空输入被读时提示用户在终端输入电平；合成值写回 guest 内存 |
| GPIO 输出可见 | 同上 | 写 `DOUT` 时按引脚打印跳变：`[GPIOB] P5 -> H` |
| 外设读写分发 | `main.cpp` | **默认开启**（`--no-hooks` 关闭）；GPIO/USART 的输入输出都依赖它 |
| 寄存器级日志 | `main.h` / `Usart.cc` | 改为运行时开关，`--periph-log` 打开（默认关，避免淹没固件打印） |
| 命令行 | `main.cpp` | `--gpio A3=1,C7=0`、`--periph-log`、`--no-hooks`、`--gpio-prompt`(默认关)、`--no-systick`、`--clock-faithful`、`--systick-isr` |

---

## 3. 时钟：已接线（待实测）

`SysTickModel`（1 指令 = 1 时钟；切片执行；把 `SysTick_Handler` 当子程序注入，LR 指向 SRAM 跳板）
已加入 `CMakeLists.txt`，并接到 `main.cpp`：默认开启，`--no-systick` 关闭，`--trampoline` 可换跳板地址。
向量表基址自动选 ROM(`0x00000000`)/Flash(`0x08000000`)。

接线时修掉两个会让它完全不工作的 bug：

1. **`VAL == 0` 不重新装载**（`SysTickModel.cc`）：固件 `APM_Delay()` 每次都写 `SysTick->VAL = 0`，
   原逻辑遇到 `VAL == 0` 直接返回 → 计数器永远停在 0，一次中断都不会来。已改为按硬件语义
   在下一个时钟从 `LOAD` 重新装载。
2. **外设复位值没有种进 guest 内存**（新增 `PeripheralDevice::plantInitialValues()`，RCM/GPIO/USART 实现）：
   Unicorn 的读 Hook 在读取完成之后才触发、**改不了本次读到的值**，固件读的始终是映射 RAM。
   原先 RAM 全 0 → `RCM_MCS` 读成 0 → `RCM_GetMasterClockFreq()` 返回 0 →
   `SysTick_Config(0)` 返回失败 → 固件卡在 `APM_DelayInit()` 的 `while(1)`（最终 PC `0x0800127A`，且此后不再有任何外设访问，正是这个特征）。

3. **不再注入固件 ISR**：注入方式要求向量表里的 `SysTick_Handler` 确实"经 LR 返回"，而实测
   `0x3C` 处拿到的是启动文件里 `.weak` 绑定的 `Default_Handler`（`b .` 死循环），注入必然失败。
   默认改为**直接做 ISR 唯一的那件事**：`APM_Delay()` 的特征序列是
   `CTRL 清 ENABLE → VAL = 0 → CTRL 置 ENABLE → 写 __delayCnt`，于是在 `VAL = 0` 之后捕获
   紧接着的 4 字节 SRAM 写（就是 `__delayCnt` 的地址），每个节拍把它减 1。
   不依赖向量表，也不要求处理器经 LR 返回；`--systick-isr` 可切回注入方式。

> **三种执行方式**（`SysTickModel::Mode`）：
> - `Direct`（默认）：不算异常，直接做 `__delayCnt--`。最快最稳，但 ISR 里若还有别的动作会被漏掉。
> - `Subroutine`（`--systick-isr`）：把向量里的 handler 当子程序调用，LR 指向 SRAM 跳板；不做压栈，
>   要求 handler 经 LR 返回。
> - `Exception`（`--systick-exc`）：**真异常入口** —— 手动把 `{r0-r3,r12,lr,pc,xpsr}` 压到 MSP、
>   `LR = 0xFFFFFFF9`、`PC = 向量`、`xPSR.IPSR = 15`，handler 当普通代码执行；它 `bx lr` 时 PC 落到
>   `0xFFFFFFF8`，那里由我们映射的一页内存接住（放一条 `B .` + 指令 Hook），然后弹栈恢复被中断的指令。
>   中断处理期间再到点 → 挂起、返回后补触发（尾链）。未实现：优先级/PRIMASK、PSP 线程、惰性压栈。
>   - **实测限制（重要）**：机制本身已验证正确（被中断 PC / SP / 压栈位置 / handler / 返回都精确），
>     但**无法用于跑完整流程**：这份固件的延时是"1 µs = 1 次 SysTick 中断"，`Delay_ms(300)` 需要
>     **30 万次**中断；而 Unicorn 里每次中断都要"停切片 → 回宿主压栈 → 再进 handler → 回宿主弹栈"，
>     实测固件每 **20 万条指令**才前进到下一次中断（0.5 秒只走了 20 个周期）。
>     ⇒ 真异常入口只适合短跑观察 ISR 行为；要"真且快"只能换 QEMU（NVIC/SysTick 在 CPU 模型内部，
>     中断不返回宿主）。

> **速率**：固件把 SysTick 配成 `48MHz / 1MHz = 48 拍/微秒`（`LOAD = 47`）。时间忠实的话
> `Delay_ms(300)` 要跑 **1440 万条指令**，而带指令 Hook 的模拟器只有约 **100 万条/秒** → 15 秒才过一句。
> 所以**默认忽略 `LOAD`，每条指令算一拍**（1 微秒 ≈ 1 条指令），demo 才跑得动；
> `--clock-faithful` 可切回"按 LOAD 计时"的忠实速率。长延时仍需 30 万次节拍
> （1 节拍递减 1 个 `__delayCnt`），但每次节拍只是一次内存写（不在延时中时连内存都不碰）。

---

## 4. 怎么跑、看什么

```
uc_demo firmware/firmware.bin                 # 默认就带外设分发
uc_demo firmware/firmware.bin --gpio C7=0     # 把矩阵键盘行 PC7 拉低 = 有键按下
uc_demo firmware/firmware.bin --periph-log    # 需要看寄存器读写时
```

| 想验证 | 预期看到 |
|---|---|
| USART 输出 | 初始化日志之后出现 `printfInit完成。` |
| GPIO 输出 | `[GPIOB] P4 -> H` / `[GPIOB] P5 -> H` 这类跳变行 |
| GPIO 输入 | 注入后固件判定有键按下 → `按下按键：0xXX`；`DIN 读 -> 0x..` 变化行 |
| 时钟 | `uc_demo -n 50000000` | 应出现 `启动完成。`（`PS2_BOOT()` 里的 `Delay_ms(300)` 能返回了） |

---

## 5. 改这个模拟器必须知道的两个 Unicorn 坑

1. **读 Hook 是事后触发的**：`UC_HOOK_MEM_READ` 回调执行时本次读取已经完成，改 `read_value` 没用。所以注入值必须**写回 guest 内存**（`plantToGuest()`），让下一次读拿到正确值 —— 固件的输入轮询都在循环里，够用。
2. **外设区是普通 RAM**：固件大量使用读-改-写（`GPIO_Config` 读 `CTRL1/CTRL2/MODE`、`USART_Config` 读 `CTRL1/CTRL3/CTRL2`）。固件自己的写会让 RAM 与寄存器模型天然一致；但**模型特有的值**（如 `STS` 的复位 `0xC0`）必须主动种回，否则固件会轮询到超时。

---

## 6. 已知限制

- USART 只发不收，不建模波特率与位时序；
- GPIO 不做中断、不做边沿检测、不模拟 PS/2 位时序；矩阵键盘"行拉低"足以让固件认为有键，但键位取决于当时的列扫描状态（可能落在第一列）；
- PPB 区域仍是普通 RAM；时钟接入后由 `SysTickModel` 单独维护 `CTRL/LOAD/VAL`；
- 其余外设（RCM/FMC/I2C/SPI/TMR/EINT/看门狗/BUZZER/ADC/WUPT）仍只记录访问，不建模行为；
- 没有自动化测试，验证靠跑一次看输出。
