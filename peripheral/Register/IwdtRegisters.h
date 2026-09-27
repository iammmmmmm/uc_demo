#pragma once
#include "Register.h"
#include <iostream>

namespace IwdtRegisters {

// =========================================================================
// IWDT_KEYWORD (关键字寄存器)
// 偏移地址：0x00，复位值：0x0000 0000
// 这是一个特殊的只写（W）寄存器，读取时返回 0。
// =========================================================================
class IWDT_KEYWORD_Register : public Register {
public:
    explicit IWDT_KEYWORD_Register(uint32_t offset)
        : Register(offset, "IWDT_KEYWORD (关键字寄存器)", 0x00000000) {
        // Bit 7:0: KEYWORD (允许访问 IWDT 寄存器键值)
        m_fields.push_back({
            "KEYWORD (Access Key Value)", 0, 8,
            {
                {0x55, "0x55: 允许访问 PSC/CNTRLD 寄存器 (假设写入 0x5555/0x55)"},
                {0xAA, "0xAA: 重装载计数器 (喂狗) (假设写入 0xAAAA/0xAA)"},
                {0xCC, "0xCC: 启动看门狗 (假设写入 0xCCCC/0xCC)"}
            }
        });

        // 注意：实际写入键值是 16 位的 0x5555, 0xAAAA, 0xCCCC。
        // 这里为了兼容 8 位位域，只列出低 8 位。
    }

    // 重写 read 方法：读出值为 0x0000
//     uint32_t read() override {
//         // 确保读出的值为 0x0000
//         uint32_t read_val = 0x00000000;
// #if Register::IS_DEBUG
//         debug_interpret_read(read_val);
// #endif
//         return read_val;
//     }

//     // 重写 write 方法：处理特殊键值，并提供调试信息
//     void write(uint32_t new_value) override {
//         // IWDT_KEYWORD 的写入是 16 位或 32 位。这里只关注低 16 位的值。
//         uint16_t key = (uint16_t)new_value;
//
//         switch (key) {
//             case 0x5555:
//                 // 模拟副作用：允许访问
//                 // 在模拟器中，这通常是设置一个内部标志
//                 m_current_value = 0x5555;
//                 break;
//             case 0xAAAA:
//                 // 模拟副作用：重装载计数器（喂狗）
//                 m_current_value = 0xAAAA;
//                 break;
//             case 0xCCCC:
//                 // 模拟副作用：启动看门狗
//                 m_current_value = 0xCCCC;
//                 break;
//             default:
//                 m_current_value = new_value; // 存储完整值供调试
//                 break;
//         }
//
// #if Register::IS_DEBUG
//         // 仅对低 16 位进行位域解析
//         debug_interpret_write(new_value & 0xFFFF);
// #endif
//     }
};

// =========================================================================
// IWDT_PSC (预分频寄存器)
// 偏移地址：0x04，复位值：0x0000 0006
// =========================================================================
class IWDT_PSC_Register : public Register {
public:
    explicit IWDT_PSC_Register(uint32_t offset)
        : Register(offset, "IWDT_PSC (预分频寄存器)", 0x00000006) {
        // Bit 2:0: PSC (计数器时钟预分频系数)
        m_fields.push_back({
            "PSC (Counter Clock Prescaler Factor)", 0, 3,
            {
                {0b000, "000: 4 分频"},
                {0b001, "001: 8 分频"},
                {0b010, "010: 16 分频"},
                {0b011, "011: 32 分频"},
                {0b100, "100: 64 分频"},
                {0b101, "101: 128 分频"},
                {0b110, "110: 256 分频"},
                {0b111, "111: 保留"}
            }
        });
    }
};

// =========================================================================
// IWDT_CNTRLD (计数器重装载寄存器)
// 偏移地址：0x08，复位值：0x0000 00FF
// =========================================================================
class IWDT_CNTRLD_Register : public Register {
public:
    explicit IWDT_CNTRLD_Register(uint32_t offset)
        : Register(offset, "IWDT_CNTRLD (计数器重装载寄存器)", 0x000000FF) {
        // Bit 7:0: CNTRLD (设置看门狗计数器重装载值)
        m_fields.push_back({
            "CNTRLD (Watchdog Counter Reload Value Setup)", 0, 8,
            {} // R/W 字段，直接显示值
        });
    }
};

} // namespace IwdtRegisters