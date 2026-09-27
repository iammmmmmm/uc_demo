# uc_demo · APM32F003 固件模拟器

基于 [Unicorn Engine](https://www.unicorn-engine.org/) 的 APM32F003(ARM Cortex-M0+)固件模拟器。
在 PC 上加载真实固件、复现复位启动流程、逐条追踪执行流,并把对外设寄存器的访问分发到 C++ 写的设备模型中,
用于**在没有硬件的情况下分析固件行为**。

---

## 1. 功能概览

| 能力 | 说明 |
|---|---|
| 固件加载 | `.bin` / `.hex`(Intel HEX) / `.elf`(ELF32) 三种格式,自动识别 |
| 启动复位 | 从向量表取出初始 SP 与复位 PC,按 Cortex-M 规则进入 Thumb 模式 |
| 执行追踪 | `-t` 开启指令级 Hook,可监视指定 BLX 指令的跳转目标 |
| 外设建模 | 21 个外设按「寄存器 + 位域」建模,`--hooks` 启用读写分发 |
| 内存防护 | 固件写入前校验目标区域,运行期捕获未映射访问并打印 PC |
| 跑飞兜底 | 虚拟 HardFault Handler(`B .` 死循环),异常不会静默跑飞 |
| 执行控制 | 指令数上限、模拟超时、运行到指定地址停止 |

---

## 2. 快速开始

```bash
# 不指定参数时自动查找 firmware/firmware.bin
uc_demo

# 指定固件,格式按扩展名自动识别
uc_demo firmware/firmware.hex
uc_demo firmware/firmware.elf

# 启用外设读写分发,并把固件跑到 0x08001A00 为止
uc_demo -f firmware/firmware.bin --hooks --until 0x08001A00

# 查看全部参数
uc_demo --help
```

典型输出:

```
uc_demo 1.1.0 - APM32F003 (Cortex-M0+) 固件模拟器
[固件] firmware/firmware.bin (格式: bin, 7096 字节, 1 个分段)
       0x08000000 - 0x08001BB8 (7096 字节)
[执行] 指令上限 1000000, 超时 0 us, 停止地址 无, 外设 Hook 关
--- 注册外设驱动 ---
  - 注册成功: GPIO_Port_A @ 0x40000000
  ...
---  HardFault 向量重定向成功: 0x0C -> 0x20000401 (SRAM Handler) ---
--- 启动模拟执行 ---
执行时间: 31472000 ns

模拟执行结束: 达到指令上限 1000000 (可能尚未执行完)
    最终 PC: 0x0800127A
```

---

## 3. 命令行参数

```
uc_demo [选项] [固件文件]
```

| 参数 | 说明 |
|---|---|
| `<固件文件>` | 位置参数,等价于 `-f` |
| `-f, --firmware <文件>` | 指定固件文件 |
| `--format <bin\|hex\|elf\|auto>` | 强制指定格式,默认 `auto`(按扩展名 + 文件头魔数识别) |
| `--base <地址>` | `.bin` 的加载基地址,默认 `0x08000000` |
| `--no-mirror` | 关闭 Flash ↔ ROM 双向镜像写入(排障用,见 §4.3) |
| `-n, --max-insns <数量>` | 最大执行指令数,`0` 表示不限制,默认 `1000000` |
| `--timeout <微秒>` | 模拟超时时间,`0` 表示不限制 |
| `--until <地址>` | 执行到该地址时停止,带不带 Thumb 位均可 |
| `-t, --trace` | 开启指令级 Hook,逐条打印 PC(**输出量极大**) |
| `--watch-blx <地址表>` | 监视 BLX 前的 `r3` 值,逗号分隔;传 `none` 清空。需配合 `-t` |
| `--hooks` | 启用外设读写 Hook(默认关闭) |
| `-q, --quiet` | 精简输出:外设注册表、Hook 提示、逐条轨迹都不打印 |
| `-v, --version` / `-h, --help` | 版本 / 帮助 |

选项值支持 `--opt value` 与 `--opt=value` 两种写法;地址与数量均支持 `0x` 前缀。

### 退出码

| 码 | 含义 |
|---|---|
| 0 | 模拟执行完成 |
| 1 | 命令行参数错误 |
| 2 | 固件无法找到 / 无法加载 / 超出容量 |
| 3 | Unicorn 初始化或内存映射失败 |
| 4 | 模拟执行过程中出错(含未映射访问) |

---

## 4. 固件加载

### 4.1 三种格式

| 格式 | 地址来源 | 说明 |
|---|---|---|
| `.bin` | `--base` 参数 | 裸二进制,自身不含地址信息 |
| `.hex` | 文件内记录 | Intel HEX,支持记录类型 `00/01/02/03/04/05`,逐行校验和验证,非法行会报出行号 |
| `.elf` | 程序头 | ELF32 小端,按 `PT_LOAD` 段的 `p_paddr` 加载;入口优先取 ELF 头的 `e_entry` |

三种格式加载同一份固件(md5 相同)会得到完全一致的镜像与最终 PC,可交叉验证。
`--format` 可强制指定格式,用于扩展名与内容不符的场景(此时会给出明确报错)。

### 4.2 路径查找顺序

未指定 `--firmware` 时按默认名 `firmware/firmware.bin` 查找;指定路径打不开时,
依次尝试(绝对路径只尝试其本身):

1. 给定路径本身
2. 可执行文件同级目录
3. `<exe目录>/../firmware/<文件名>`
4. `./firmware/<文件名>`
5. `../firmware/<文件名>`

全部失败时会列出所有尝试过的候选路径。**因此从项目根目录或构建目录运行都能找到固件。**

### 4.3 Flash ↔ ROM 镜像(重要)

默认会把固件同时写入 `0x08000000`(Flash)与 `0x00000000`(ROM),这不是冗余:

- 这类固件通常**以 `0x00000000` 链接**,复位向量的值是 `0x000015B9` 这样的低地址,
  启动时需要加上 Flash 基址才能得到真正的执行地址 `0x080015B9`;
- 固件里会出现 `BLX r3`,而 `r3 = 0x1851` 这类**链接期低地址**,跳转目标落在 ROM 区;
- 复位后从 ROM 区自举的芯片,其向量表被硬件映射到 `0x00000000`。

两块内容必须一致,否则固件一跳转就会执行到全 0 的内存(`--no-mirror` 下可复现,PC 会落到 `0x00000000`)。
此外 HardFault 向量重定向固定写在 `0x0000000C`,同样依赖 ROM 区有内容。

---

## 5. 内存映射

| 区域 | 起始地址 | 大小 | 权限 | 用途 |
|---|---|---|---|---|
| ROM / 系统存储区 | `0x00000000` | 32 KB | R-X | 固件镜像 |
| Flash | `0x08000000` | 32 KB | R-X | 固件镜像 |
| SRAM | `0x20000000` | 4 KB | RWX | 栈、`.data`/`.bss`;虚拟 HardFault Handler 位于 `0x20000400` |
| 外设 | `0x40000000` | 0x12000 | RW- | 所有外设寄存器 |
| Cortex-M 私有外设 (PPB) | `0xE0000000` | 1 MB | RW- | 普通 RAM 映射,未实现 NVIC/SysTick 行为 |

固件写入前会校验分段是否完整落在上述区域内,越界时直接报错并列出可用区域,不会静默截断。
固件总大小超过 Flash 容量(32 KB)时在加载阶段即拒绝。

---

## 6. 运行时结构

`main()` 的执行顺序:

1. **解析参数** → 2. **查找并加载固件**(先于 Unicorn 初始化,尽早失败)
→ 3. `uc_open` + 映射内存 → 4. **写入固件分段**(含镜像)
→ 5. **初始化 SP/PC**(读向量表,ELF 优先用 `e_entry`)→ 6. **注册外设**
→ 7. **虚拟 HardFault 重定向** → 8. **注册 Hook** → 9. `uc_emu_start` → 10. 报告与清理

### Hook 机制

| Hook | 范围 | 作用 |
|---|---|---|
| `UC_HOOK_MEM_WR` | `0x40000000`-`0x400114FF` | 读写外设时按地址在注册表中查设备并调用 `handle_read/handle_write` |
| `UC_HOOK_CODE` | Flash + ROM 两个区间 | `-t` 时才注册;计数并打印 PC,命中 `--watch-blx` 时解析 `r3` 目标 |
| `*_UNMAPPED` | 整个地址空间 | 打印 PC 与访问地址后停止模拟 |

> 代码 Hook 必须同时覆盖 Flash 与 ROM 两个区间:固件经 `BLX` 跳到 ROM 镜像执行的指令
> 也要计入,否则指令计数与 Unicorn 实际执行数会对不上。

### 设备查找

`PeripheralRegistry` 是单例,以**基地址为 key** 存放在 `std::map` 中;
`findDevice(addr)` 返回基地址 `<= addr` 的最近一个设备。因此外设的寄存器偏移是相对其基地址天然解析的。

### 寄存器模型

每个外设内部是 `std::map<偏移, std::unique_ptr<Register>>`,在 `initialize_registers()` 里
用 `Register` / `BitField` 描述寄存器复位值与位域名称,可挂接:

- `BitFieldWriteCallback` — 某个位域被写入时的副作用(如使能时钟、启动转换);
- `RegisterReadCallback` — 读取时修改返回值(如读清状态位)。

配合 `peripheral/*/Register/*Registers.h` 中集中定义的寄存器描述,日志会直接打印位域名,
例如:

```
[CMU W: 0x10] 写入 RCM_MCC (主时钟配置) 寄存器: 0x000000e1
```

---

## 7. 外设支持情况

共注册 21 个设备(基地址为 `map` 的 key,故不重叠):

| 外设 | 基地址 | 实现方式 |
|---|---|---|
| GPIOA / GPIOB / GPIOC / GPIOD | `0x40000000` / `400` / `800` / `C00` | 独立模型(`GPIORegister.h`) |
| RCM(CMU) | `0x40010000` | 独立模型(`RcmRegisters.h`),时钟树相关副作用 |
| FMC(Flash 控制器) | `0x40011000` | 独立模型(`FlashRegisters.h`) |
| USART1 / USART2 / USART3 | `0x40003400` / `0x40001400` / `0x40004800` | 独立模型(`UsartRegisters.h`) |
| I2C | `0x40003000` | 独立模型(`I2cRegisters.h`) |
| SPI | `0x40002C00` | 独立模型(`SpiRegisters.h`) |
| EINT | `0x40001800` | 独立模型(`EnitRegisters.h`) |
| IWDT / WWDT | `0x40002000` / `0x40001C00` | 独立模型,看门狗计数逻辑 |
| BUZZER | `0x40002800` | 独立模型(`BuzzerRegister.h`) |
| WUPT / ADC / TMR1 / TMR2 / TMR4 / TMR1A | `0x40002400` / `4400` / `3800` / `3C00` / `4000` / `1000` | `GenericPeripheral` 占位:只记录访问,不建模行为 |

寄存器级日志由 `main.h` 中的 `IS_DEBUG` 统一开关(`#if IS_DEBUG`)。

---

## 8. 如何新增一个外设

1. 在 `peripheral/Register/` 下新增 `XxxRegisters.h`,用 `Register` / `BitField` 描述寄存器与位域;
2. 在 `peripheral/` 下新增 `Xxx.h` / `Xxx.cc`,继承 `PeripheralDevice`:

```cpp
class Xxx : public PeripheralDevice {
  std::map<uint32_t, std::unique_ptr<Register>> m_registers;
  void initialize_registers();              // 复位值与位域定义
public:
  Xxx();
  bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) override;
  bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) override;
  uint64_t getBaseAddress() override { return XXX_BASE; }
  std::string getName() override { return "XXX"; }
};
```

3. 在 `main.cpp` 的 `registerPeripherals()` 中 `registerDevice(std::make_unique<Xxx>())`;
4. 把新增源文件加入 `CMakeLists.txt` 的 `add_executable(uc_demo ...)` 列表;
5. 用 `--hooks` 运行,确认寄存器访问被正确分发。

---

## 9. 已知限制

- **外设仅到寄存器级**:USART 不产生字符输出、GPIO 不驱动外部器件,只能通过访问日志观察行为;
- **无中断注入**:没有定时器/NVIC 事件源,固件进入 `while(1)` 空转后不会自行结束
  (上面的示例固件就停在 `0x0800127A` 的等待循环),需靠 `-n` / `--timeout` 结束;
- **PPB 区域是普通内存**:读写 `0xE0000000` 起的内存不会触发真实的 NVIC/SysTick 行为;
- **不做外设写入的合法性检查**:未注册的地址只打印「未注册的设备地址」,不报错;
- **初始 SP 越界仅警告**(不在 SRAM 范围内时提示,但继续执行);
- **不支持中断/异常的真实栈帧**:HardFault 被替换为 SRAM 里的 `B .` 死循环,便于定位但不还原压栈过程;
- **仅 Cortex-M0+**(Thumb / ARMv6-M),无浮点、无 64 位固件;
- 每次运行都从复位向量重新开始,没有快照 / 断点回放能力。

---

## 10. 目录结构

```
uc_demo/
├── main.cpp                  # 参数解析、内存映射、固件写入、Hook、主循环
├── main.h                    # 架构/模式、内存布局宏、版本号、IS_DEBUG
├── FirmwareImage.h/.cc       # 固件加载器: bin / Intel HEX / ELF32 + 路径解析
├── CMakeLists.txt
├── firmware/                 # 示例固件 (.bin/.hex/.elf 同一镜像) 与分析资料
└── peripheral/
    ├── peripheral_factory.h/.cc   # PeripheralDevice 基类 + PeripheralRegistry + GenericPeripheral
    ├── GPIO_Port.*  Rcm.*  Flash.*  Usart.*  I2c.*  Spi.*
    ├── Eint.*  Iwdt.*  Wwdt.*  Buzzer.*
    ├── Register/             # Register / BitField 抽象 + 各外设寄存器描述
    └── Timer/                # 预留
```

---

## 11. 构建

依赖:

- CMake ≥ 3.20、支持 C++20 的编译器(Windows 上用 MSYS2 MinGW-w64);
- Unicorn 静态库(`libunicorn.a`),路径在 `CMakeLists.txt` 中通过 `UNICORN_ROOT_DIR` 指定,
  默认写死为 `E:\Projects\qemu_for_apm32\unicorn`,**换机器时需修改**。

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`firmware/` 目录下的 `firmware.bin` / `firmware.hex` / `firmware.elf` 是同一镜像的三种封装,
可直接用于交叉验证加载器。
