#pragma once
#include "Register.h"

// =========================================================================
// Flash 控制器寄存器定义
// =========================================================================

// 3.7.1 控制寄存器1 (FLASH_CTRL1)
// 偏移地址：0x00 复位值：0x0000 0030
class Reg_FLASH_CTRL1 : public Register {
public:
    // 偏移地址 0x00
    explicit Reg_FLASH_CTRL1(uint32_t offset)
      : Register(offset, "FLASH_CTRL1 (控制寄存器1)") {
        // 寄存器复位值
        m_current_value = 0x00000030;

        // Bit 2:0 LATENCY
        m_fields.push_back({
            "LATENCY (等待状态配置)", 0, 3,
            {
                {0, "000：0个等待周期，系统时钟 ≤ 24MHz"},
                {1, "001：1个等待周期，24MHz < 系统时钟 ≤ 48MHz"}
            }
        });

        // Bit 3 HCAEN
        m_fields.push_back({
            "HCAEN (使能半周期访问)", 3, 1,
            {
                {0, "0：禁止"},
                {1, "1：允许"}
            }
        });

        // Bit 4 PBEN
        m_fields.push_back({
            "PBEN (使能预取缓存区)", 4, 1,
            {
                {0, "0：禁用"},
                {1, "1：使能"}
            }
        });

        // Bit 5 PBSF
        m_fields.push_back({
            "PBSF (预取缓存区状态标志)", 5, 1,
            {
                {0, "0：处于关闭状态"},
                {1, "1：处于打开状态"}
            }
        });

        // 忽略 31:6 的保留位
    }
};

// 3.7.2 关键字寄存器1 (FLASH_KEY)
// 偏移地址：0x04 复位值：0xXXXX XXXX (无法确定复位值，使用 0x00000000)
class Reg_FLASH_KEY : public Register {
public:
    // 偏移地址 0x04
    explicit Reg_FLASH_KEY(uint32_t offset)
      : Register(offset, "FLASH_KEY (关键字寄存器1)") {
        // 寄存器复位值 (数据手册显示为 X，使用默认 0)
        m_current_value = 0x00000000;

        // Bit 31:0 KEY
        m_fields.push_back({
            "KEY (FMC 关键字)", 0, 32,
            {} // 写入关键字用于解锁 FMC
        });
    }
};

// 3.7.3 选项字节关键字寄存器 (FLASH_OBKEY)
// 偏移地址：0x08 复位值：0x03FF FFFC
class Reg_FLASH_OBKEY : public Register {
public:
    // 偏移地址 0x08
    explicit Reg_FLASH_OBKEY(uint32_t offset)
      : Register(offset, "FLASH_OBKEY (选项字节关键字寄存器)") {
        // 寄存器复位值
        m_current_value = 0x03FFFFFC;

        // Bit 31:0 KEY
        m_fields.push_back({
            "KEY (选项字节关键字)", 0, 32,
            {} // 写入关键字可以解除选项字节写操作的锁定
        });
    }
};

// 3.7.4 状态寄存器 (FLASH_STS)
// 偏移地址：0x0C 复位值：0x0000 0000
class Reg_FLASH_STS : public Register {
public:
    // 偏移地址 0x0C
    explicit Reg_FLASH_STS(uint32_t offset)
      : Register(offset, "FLASH_STS (状态寄存器)") {
        // 寄存器复位值
        m_current_value = 0x00000000;

        // Bit 0 BUSYF
        m_fields.push_back({
            "BUSYF (忙碌标志)", 0, 1,
            {
                {0, "0：空闲"},
                {1, "1：正在进行闪存操作"}
            }
        });

        // Bit 2 PEF
        m_fields.push_back({
            "PEF (编程错误标志)", 2, 1,
            {
                {0, "0：无错误"},
                {1, "1：编程错误 (地址编辑前的值非 0xFFFF)"}
            }
        });

        // Bit 4 WPEF
        m_fields.push_back({
            "WPEF (写保护错误标志)", 4, 1,
            {
                {0, "0：无错误"},
                {1, "1：写保护错误 (编程了写保护地址)"}
            }
        });

        // Bit 5 OCF
        m_fields.push_back({
            "OCF (操作完成标志)", 5, 1,
            {
                {0, "0：操作未完成"},
                {1, "1：操作完成"}
            }
        });

        // 忽略 31:6 的保留位
    }
};

// 3.7.5 控制寄存器2 (FLASH_CTRL2)
// 偏移地址：0x10 复位值：0x0000 0080
class Reg_FLASH_CTRL2 : public Register {
public:
    // 偏移地址 0x10
    explicit Reg_FLASH_CTRL2(uint32_t offset)
      : Register(offset, "FLASH_CTRL2 (控制寄存器2)") {
        // 寄存器复位值
        m_current_value = 0x00000080;

        // Bit 0 PG
        m_fields.push_back({
            "PG (编程)", 0, 1,
            {
                {0, "0：禁止编程"},
                {1, "1：进行 Flash 编程操作"}
            }
        });

        // Bit 1 PAGEERA
        m_fields.push_back({
            "PAGEERA (页擦除)", 1, 1,
            {
                {0, "0：禁止页擦除"},
                {1, "1：进行页擦除"}
            }
        });

        // Bit 2 MASSERA
        m_fields.push_back({
            "MASSERA (整片擦除)", 2, 1,
            {
                {0, "0：禁止整片擦除"},
                {1, "1：进行整片擦除"}
            }
        });

        // Bit 4 OBP
        m_fields.push_back({
            "OBP (编程选项字节)", 4, 1,
            {
                {0, "0：禁止选项字节编程"},
                {1, "1：进行选项字节编程操作"}
            }
        });

        // Bit 5 OBE
        m_fields.push_back({
            "OBE (擦除选项字节)", 5, 1,
            {
                {0, "0：禁止选项字节擦除"},
                {1, "1：进行选项字节擦除操作"}
            }
        });

        // Bit 6 STA
        m_fields.push_back({
            "STA (开始进行擦除操作)", 6, 1,
            {
                {0, "0：空闲"},
                {1, "1：软件置位，开始擦除"}
            }
        });

        // Bit 7 LOCK
        m_fields.push_back({
            "LOCK (锁定)", 7, 1,
            {
                {0, "0：未锁定"},
                {1, "1：锁定 Flash 和 CTRL2 寄存器"}
            }
        });

        // Bit 9 OBWEN
        m_fields.push_back({
            "OBWEN (使能选项字节写操作)", 9, 1,
            {
                {0, "0：禁止选项字节写入"},
                {1, "1：使能选项字节写入"}
            }
        });

        // Bit 10 ERRIE
        m_fields.push_back({
            "ERRIE (使能错误中断)", 10, 1,
            {
                {0, "0：禁止中断"},
                {1, "1：使能中断 (PEF=1 或 WPEF=1 时)"}
            }
        });

        // Bit 12 OCIE
        m_fields.push_back({
            "OCIE (使能操作完成中断)", 12, 1,
            {
                {0, "0：操作完成中断禁用"},
                {1, "1：操作完成中断使能 (OCF=1 时)"}
            }
        });

        // 忽略 31:13 的保留位
    }
};

// 3.7.6 地址寄存器 (FLASH_ADDR)
// 偏移地址：0x14 复位值：0x0000 0000
class Reg_FLASH_ADDR : public Register {
public:
    // 偏移地址 0x14
    explicit Reg_FLASH_ADDR(uint32_t offset)
      : Register(offset, "FLASH_ADDR (地址寄存器)") {
        // 寄存器复位值
        m_current_value = 0x00000000;

        // Bit 31:0 ADDR
        m_fields.push_back({
            "ADDR (Flash 地址)", 0, 32,
            {} // 编程操作时写入要编程的地址，页擦除时写入要擦除的页
        });
    }
};

// 3.7.7 选项字节控制/状态寄存器 (FLASH_OBCS)
// 偏移地址：0x1C 复位值：0x03FF FFFC
class Reg_FLASH_OBCS : public Register {
public:
    // 偏移地址 0x1C
    explicit Reg_FLASH_OBCS(uint32_t offset)
      : Register(offset, "FLASH_OBCS (选项字节控制/状态寄存器)") {
        // 寄存器复位值
        m_current_value = 0x03FFFFFC;

        // Bit 0 OBE
        m_fields.push_back({
            "OBE (选项字节错误)", 0, 1,
            {
                {0, "0：选项字节和补码匹配"},
                {1, "1：选项字节和补码不匹配 (强制写入 0xFF)"}
            }
        });

        // Bit 1 READPROT
        m_fields.push_back({
            "READPROT (读保护)", 1, 1,
            {
                {0, "0：读保护无效"},
                {1, "1：闪存处于读保护状态"}
            }
        });

        // Bit 2 WWDTSW
        m_fields.push_back({
            "WWDTSW (切换窗口看门狗)", 2, 1,
            {
                {0, "0：硬件激活窗口看门狗"},
                {1, "1：软件激活窗口看门狗"}
            }
        });

        // Bit 3 WWDTRST
        m_fields.push_back({
            "WWDTRST (复位窗口看门狗)", 3, 1,
            {
                {0, "0：HALT 模式下产生复位"},
                {1, "1：HALT 模式下不产生复位"}
            }
        });

        // Bit 4 IWDTSW
        m_fields.push_back({
            "IWDTSW (切换独立看门狗)", 4, 1,
            {
                {0, "0：硬件激活独立看门狗"},
                {1, "1：软件激活独立看门狗"}
            }
        });

        // Bit 5 LIRCEN
        m_fields.push_back({
            "LIRCEN (使能 LIRC)", 5, 1,
            {
                {0, "0：LIRC 可作为 CPU 时钟源"},
                {1, "1：LIRC 不可作为 CPU 时钟源"}
            }
        });

        // Bit 6 HIRCTRIM
        m_fields.push_back({
            "HIRCTRIM (调试 HIRC)", 6, 1,
            {
                {0, "0：4 位调整值"},
                {1, "1：3 位调整值"}
            }
        });

        // Bit 17:10 DATA0
        m_fields.push_back({
            "DATA0", 10, 8,
            {}
        });

        // Bit 25:18 DATA1
        m_fields.push_back({
            "DATA1", 18, 8,
            {}
        });

        // 忽略 31:26 的保留位
    }
};

// 3.7.8 写保护寄存器 (FLASH_WRTPROT)
// 偏移地址：0x20 复位值：0xFFFF FFFF
class Reg_FLASH_WRTPROT : public Register {
public:
    // 偏移地址 0x20
    explicit Reg_FLASH_WRTPROT(uint32_t offset)
      : Register(offset, "FLASH_WRTPROT (写保护寄存器)") {
        // 寄存器复位值
        m_current_value = 0xFFFFFFFF;

        // Bit 31:0 WRTPROT
        m_fields.push_back({
            "WRTPROT (写保护)", 0, 32,
            {
                {0, "0：写保护有效"},
                {1, "1：写保护无效"}
            }
        });
    }
};

// 3.7.9 低功耗模式寄存器 (FLASH_LPM)
// 偏移地址：0x24 复位值：0x0000 0000
class Reg_FLASH_LPM : public Register {
public:
    // 偏移地址 0x24
    explicit Reg_FLASH_LPM(uint32_t offset)
      : Register(offset, "FLASH_LPM (低功耗模式寄存器)") {
        // 寄存器复位值
        m_current_value = 0x00000000;

        // Bit 0 HALT
        m_fields.push_back({
            "HALT (Flash 在 halt 模式下掉电)", 0, 1,
            {
                {0, "0：MCU 处于 halt 模式，Flash 处于掉电状态"},
                {1, "1：MCU 处于 halt 模式，Flash 处于工作状态"}
            }
        });

        // Bit 1 AHALT
        m_fields.push_back({
            "AHALT (Flash 在 Active-halt 模式下掉电)", 1, 1,
            {
                {0, "0：MCU 处于 Active-halt 模式，Flash 处于工作状态"},
                {1, "1：MCU 处于 Active-halt 模式，Flash 处于掉电状态"}
            }
        });

        // 忽略 31:2 的保留位
    }
};

// 3.7.10 闪存 tpower_on 寄存器 (FLASH_TPO)
// 偏移地址：0x28 复位值：0x0000 0000
class Reg_FLASH_TPO : public Register {
public:
    // 偏移地址 0x28
    explicit Reg_FLASH_TPO(uint32_t offset)
      : Register(offset, "FLASH_TPO (闪存 tpower_on 寄存器)") {
        // 寄存器复位值
        m_current_value = 0x00000000;

        // Bit 7:0 TPO
        m_fields.push_back({
            "TPO (Flash 处于掉电模式下的启动时间)", 0, 8,
            {} // Flash 从掉电模式启动所需等待时间
        });

        // 忽略 31:8 的保留位
    }
};

// // =========================================================================
// // Flash 模块集合类
// // =========================================================================
// // 如果你想将所有寄存器作为一个整体管理，可以使用这个结构体：
// struct FlashController {
//     Reg_FLASH_CTRL1   CTRL1{0x00};
//     Reg_FLASH_KEY     KEY{0x04};
//     Reg_FLASH_OBKEY   OBKEY{0x08};
//     Reg_FLASH_STS     STS{0x0C};
//     Reg_FLASH_CTRL2   CTRL2{0x10};
//     Reg_FLASH_ADDR    ADDR{0x14};
//     Reg_FLASH_OBCS    OBCS{0x1C}; // 注意，0x14 之后是 0x1C
//     Reg_FLASH_WRTPROT WRTPROT{0x20};
//     Reg_FLASH_LPM     LPM{0x24};
//     Reg_FLASH_TPO     TPO{0x28};
// };