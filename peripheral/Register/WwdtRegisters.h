#pragma once
#include "Register.h"
#include <iostream>

namespace WwdtRegisters {

// =========================================================================
// WWDT_CTRL (控制寄存器)
// 偏移地址：0x00，复位值：0x0000 007F
// =========================================================================
class WWDT_CTRL_Register : public Register {
  public:
    explicit WWDT_CTRL_Register(uint32_t offset)
        : Register(offset, "WWDT_CTRL (控制寄存器)", 0x0000007F) {
      // Bit 6:0: CNT (设置计数器数值)
      m_fields.push_back({
          "CNT (Counter Value Setup, 7 bits)", 0, 7,
          {} // R/W 字段，直接显示值
      });

      // Bit 7: WWDTEN (使能窗口看门狗) - R/S (Read/Set, 软件置 1, 硬件复位清除)
      m_fields.push_back({
          "WWDTEN (Window Watchdog Enable)", 7, 1,
          {{0, "0: 禁止"}, {1, "1: 使能"}}
      });
    }

    // // 对于 R/S 位 (WWDTEN)，软件只能置 1，不能清 0。
    // // 这里重写 write 来模拟这个行为。
    // void write(uint32_t new_value)  {
    //   // 提取当前的 WWDTEN 状态 (Bit 7)
    //   uint32_t current_wwdten = (m_current_value >> 7) & 1U;
    //
    //   // 提取新值中的 WWDTEN 位
    //   uint32_t new_wwdten = (new_value >> 7) & 1U;
    //
    //   if (current_wwdten == 1 && new_wwdten == 0) {
    //     // 尝试将 1 写入 0，忽略此操作，WWDTEN 保持 1
    //     new_value = (new_value & ~(1U << 7)) | (1U << 7);
    //   }
    //
    //   Register::write(new_value);
    // }
};

// =========================================================================
// WWDT_WDDATA (窗口看门狗数据寄存器)
// 偏移地址：0x04，复位值：0x0000 007F
// =========================================================================
class WWDT_WDDATA_Register : public Register {
  public:
    explicit WWDT_WDDATA_Register(uint32_t offset)
        : Register(offset, "WWDT_WDDATA (窗口看门狗数据寄存器)", 0x0000007F) {
      // Bit 6:0: WINCNT (设置窗口值)
      m_fields.push_back({
          "WINCNT (Window Value Setup, 7 bits)", 0, 7,
          {} // R/W 字段，直接显示值
      });
    }
};

} // namespace WwdtRegisters