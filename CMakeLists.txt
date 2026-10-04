cmake_minimum_required(VERSION 4.0)
project(uc_demo C CXX ASM)

set(CMAKE_CXX_STANDARD 20)

set(UNICORN_ROOT_DIR "E:\\Projects\\qemu_for_apm32\\unicorn")


add_library(unicorn_static STATIC IMPORTED)

set_target_properties(unicorn_static PROPERTIES
        IMPORTED_LOCATION "${UNICORN_ROOT_DIR}/lib/libunicorn.a"
)

target_include_directories(unicorn_static INTERFACE
        "${UNICORN_ROOT_DIR}/include"
)

add_executable(uc_demo
        main.cpp
        main.h
        FirmwareImage.cc
        FirmwareImage.h
        SysTickModel.cc
        SysTickModel.h
        peripheral/peripheral_factory.h
        peripheral/GPIO_Port.cc
        peripheral/GPIO_Port.h
        peripheral/peripheral_factory.cc
        peripheral/Rcm.cc
        peripheral/Rcm.h
        peripheral/Flash.cc
        peripheral/Flash.h
        peripheral/Register/Register.h
        peripheral/Register/RcmRegisters.h
        peripheral/Register/Register.cc
        peripheral/Register/FlashRegisters.h
        peripheral/Register/GPIORegister.h
        peripheral/Register/BuzzerRegister.h
        peripheral/Buzzer.cc
        peripheral/Buzzer.h
        peripheral/Eint.cc
        peripheral/Eint.h
        peripheral/Register/EnitRegisters.h
        peripheral/Register/I2cRegisters.h
        peripheral/I2C.cc
        peripheral/I2C.h
        peripheral/Register/SpiRegisters.h
        peripheral/Spi.cc
        peripheral/Spi.h
        peripheral/Register/IwdtRegisters.h
        peripheral/Register/WwdtRegisters.h
        peripheral/Iwdt.cc
        peripheral/Iwdt.h
        peripheral/Wwdt.cc
        peripheral/Wwdt.h
        peripheral/Register/UsartRegisters.h
        peripheral/Usart.cc
        peripheral/Usart.h
)
# --- 添加头文件搜索路径 ---
target_include_directories(uc_demo PRIVATE
        # 1. 项目根目录 (用于 main.h)
        ${CMAKE_CURRENT_SOURCE_DIR}
        # 2. peripheral 目录 (用于 peripheral_factory.h, Rcm.h, Flash.h 等)
        ${CMAKE_CURRENT_SOURCE_DIR}/peripheral
        # 3. peripheral/Register 目录 (用于 Register.h, RcmRegisters.h, BuzzerRegister.h 等)
        ${CMAKE_CURRENT_SOURCE_DIR}/peripheral/Register
)
target_link_libraries(uc_demo PRIVATE unicorn_static)

