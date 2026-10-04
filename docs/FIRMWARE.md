# 目标固件：IR + 薄膜键盘 + PS/2 从机（APM32F003F6U6）

模拟器要伺候的固件。本文记录它的源码位置、引脚、执行流程与观测点，改模拟器前先读这里。

- 固件工程：`E:\Projects\IR_KeyBoard_PS2_APM32F003F6U6`
  （`App/Src/main.c`、`App/Src/init.c`、`App/Src/ps2.c`、`App/Src/delay.c`；`App/Inc/configs.h` 定义引脚）
- 官方 SDK：`E:\Stm32Doc\APM32Sdk\APM32F00x_SDK_V1.4`
  （`Libraries/APM32F00x_StdPeriphDriver`、`Boards/Board_APM32F003_MINI/src/bsp_delay.c`）
- 编译产物：`Project/Cmake/cmake-build-debug-mingw_stm32/firmware.bin`
- 模拟器里用的镜像：`uc_demo/firmware/firmware.bin|hex|elf`（同一份，md5 相同）

## 1. 它在做什么

一块 **PS/2 从机**（对 PC 来说它是一把键盘/一个输入设备）：把本地按键——**薄膜矩阵键盘**和**红外遥控**——翻译成 **PS/2 扫描码**上报给主机；同时用 `USART3` 打印调试信息。

## 2. `main()` 流程

```c
//Clock_init();            // 注释掉了：不开 HXT、不做时钟切换
GPIO_init();               // 全部引脚配置（见下表）
TIM_init();                // TMR1：divider=47, count=65535 —— 给红外解码当计时器
//NVIC_init();             // 空函数
USART_init();              // USART3 @115200, TX+RX
printfInit();              // tinyprintf -> io_putchar -> USART_TxData8
printf("printfInit完成。");  // ← 第一句输出
Delay_TMR1_Init();         // 名字骗人：实际是 APM_DelayInit() -> SysTick_Config()
PS2_BOOT();                // 列全拉低、CLK/DATA 拉高、Delay_ms(300)、上报 0xAA
printf("启动完成。");        // ← 第二句输出
while (1) { PS2_LOOP(); }  // 键盘扫描 + 红外结果 -> PS/2 上报
```

## 3. 引脚（`configs.h`）

| 用途 | 端口 / 引脚 | 模式 | 说明 |
|---|---|---|---|
| PS/2 CLK | **GPIOB5** | `OUT_PP` 推挽输出 | 固件驱动它当时钟，又用 `GPIO_ReadInputBit` 读回 |
| PS/2 DATA | **GPIOB4** | `OUT_PP` 推挽输出 | 同上，双向靠推挽+读回 |
| 键盘列 ×4 | **GPIOD5..D2** | `OUT_PP` 输出 | 扫描时逐列拉低 |
| 键盘行 ×5 | **GPIOC7..C3** | `IN_PU` 上拉输入 | 扫描时读；不为低 = 无键 |
| 红外输入 | **GPIOA3** | `IN_PU` + EINT 下降沿 + NVIC | 中断路径，模拟器未做 |
| 调试串口 | **USART3**（`0x40004800`）+ GPIOB4/B5（`DEBUG_USART` 下配置） | — | `printf` 走这里 |

## 4. 时间基（`bsp_delay.c`）

```c
void APM_DelayInit(APM_Delay_typedef t) {
  if (SysTick_Config(RCM_GetMasterClockFreq() / t)) { while (1); }  // 失败即死循环
}
void APM_Delay(uint32_t cnt) {
  SysTick->CTRL &= ~ENABLE; SysTick->VAL = 0; SysTick->CTRL |= ENABLE;
  __delayCnt = cnt & 0x7FFFFFFF;
  while (__delayCnt > 0);        // 忙等
  ...
}
void SysTick_Handler(void) { APM_DelayIsr(); }   // __delayCnt 只在这里递减
```

- **`__delayCnt` 只由 SysTick 中断递减** → 模拟器不产生 SysTick 中断，任何 `Delay_us/ms/s` 都会永久忙等；
- `SysTick_Handler → APM_DelayIsr()` 是普通 C 函数（经 LR 返回），可以被"当子程序注入"；
- ⚠️ `RCM_GetMasterClockFreq()` 若返回 0，`SysTick_Config(0)` 失败 → 卡在 `APM_DelayInit` 的 `while(1)`。

## 5. 输入路径

**矩阵键盘**（`Key4X5Scan()`）：逐列拉低 → 读 5 行 → 命中行/列查 `mk_key_map[5][4]` → 查 `MKKeyArray` 得扫描码。
注意：**行在 GPIOC、列在 GPIOD，跨端口**；只有"当前被拉低的那一列"对应的行低才是一次真实按键。

**PS/2 从机**（`PS2Send/PS2Receive/FollowCommand`）：

- 11 位帧（1 起始位 + 8 数据 + 1 奇校验 + 1 停止位），`ClockDelay = 60us`；
- `PS2Send` 先等 CLK 空闲（`GetClockPinState() == HIGH`），超时 100 次后放弃；
- `PS2Receive` 在时钟上升沿逐位采样 DATA；
- 主机命令（`0xFF` 复位、`0xF4` 使能、`0xF2` 读 ID…）由 `FollowCommand()` 回应 ACK。

**红外**（`ir_ex_ISR`）：EINT 下降沿里用 TMR1 计数值区分 NEC 的起始/重复/1/0，凑齐 32 位再查 `IRKeyArray`。

## 6. 观测点（拿来看模拟器对不对）

| 输出 | 含义 |
|---|---|
| `printfInit完成。` | `USART_init` + `printf` 路径通了（在第一个 `Delay` 之前） |
| `发送数据超时` | PS/2 CLK 一直不是高电平——固件读到的 CLK 电平不对 |
| `启动完成。` | `PS2_BOOT()` 返回了（含 `Delay_ms(300)`，**必须有时钟**） |
| `开始接收` / `收到指令：xx` | `PS2_LOOP` 认为主机在发命令（CLK=H 且 DATA=L） |
| `按下按键：0xXX` | 键盘扫描或红外命中（`XX` 是查表得到的 PS/2 扫描码） |
| `发送数据：xx` | 固件往主机上报一帧 PS/2 数据 |

## 7. 常见"看起来像模拟器坏了"的现象

| 现象 | 原因 |
|---|---|
| 永远打印 `按下按键：0x6B`（LeftArrow） | 键盘行被读成恒低（DIN 恒 0 或行被静态拉低）→ 每次扫描都在第 0 行第 0 列命中 → `mk_key_map[0][0] = 0` → 查表得 LeftArrow |
| `发送数据超时` 刷屏 | PS/2 CLK 读回不是高电平（`DIN` 没有回读 `DOUT`） |
| 一个字符都打印不出来 | USART 写是 8 位访问（`DATA_B.DATA`），模拟器按 4 字节过滤就会全部丢弃 |
| 打印到 `printfInit完成。` 就停 | 正常：卡在 `PS2_BOOT()` 的 `Delay_ms(300)`，等 SysTick 时钟 |
| 每个字符之间卡很久 | `STS.TXBEF` 被清掉且没恢复 → `io_putchar()` 每次轮询 `USART_TIMEOUT`(3216) 次 |
