cmake_minimum_required(VERSION 3.20)

# 设置交叉编译工具链
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/arm-gcc-toolchain.cmake)

project(ITEMP C ASM)

# MCU 配置
set(MCU_ARCH cortex-m4)
set(MCU_FLOAT_ABI hard)
set(MCU_FPU fpv4-sp-d16)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F407ZGTx_FLASH.ld)

# 编译选项
set(MCU_FLAGS "-mcpu=${MCU_ARCH} -mthumb -mfpu=${MCU_FPU} -mfloat-abi=${MCU_FLOAT_ABI}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_FLAGS} -Wall -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MCU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MCU_FLAGS} -T${LINKER_SCRIPT} --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage -u _printf_float")

# 预处理宏
add_definitions(
    -DSTM32F40_41xxx
    -DUSE_STDPERIPH_DRIVER
)

# 头文件路径
include_directories(
    CORE
    USER
    SYSTEM/delay
    SYSTEM/sys
    SYSTEM/usart
    HARDWARE/LED
    HARDWARE/LCD
    HARDWARE/ADC
    HARDWARE/DAC
    FWLIB/inc
)

# 启动文件
set(STARTUP_SRC CORE/startup_stm32f407xx.s)

# 用户源文件
set(USER_SRC
    USER/control_output.c
    USER/main.c
    USER/temp_conversion.c
    USER/stm32f4xx_it.c
    USER/system_stm32f4xx.c
)

# 系统源文件
set(SYSTEM_SRC
    SYSTEM/delay/delay.c
    SYSTEM/sys/sys.c
    SYSTEM/usart/usart.c
)

# 硬件驱动源文件
set(HARDWARE_SRC
    HARDWARE/ADC/adc.c
    HARDWARE/DAC/dac.c
    HARDWARE/LCD/lcd.c
    HARDWARE/LED/led.c
)

# STM32固件库源文件（仅编译实际需要的模块）
set(FWLIB_SRC
    FWLIB/src/misc.c
    FWLIB/src/stm32f4xx_adc.c
    FWLIB/src/stm32f4xx_dac.c
    FWLIB/src/stm32f4xx_dma.c
    FWLIB/src/stm32f4xx_exti.c
    FWLIB/src/stm32f4xx_flash.c
    FWLIB/src/stm32f4xx_fsmc.c
    FWLIB/src/stm32f4xx_gpio.c
    FWLIB/src/stm32f4xx_pwr.c
    FWLIB/src/stm32f4xx_rcc.c
    FWLIB/src/stm32f4xx_syscfg.c
    FWLIB/src/stm32f4xx_tim.c
    FWLIB/src/stm32f4xx_usart.c
)

# 构建目标
add_executable(${PROJECT_NAME}.elf
    ${STARTUP_SRC}
    ${USER_SRC}
    ${SYSTEM_SRC}
    ${HARDWARE_SRC}
    ${FWLIB_SRC}
)

# 生成 hex 和 bin 文件
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)
