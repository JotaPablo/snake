# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(snake C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(snake snake.c 
                     lib/neopixel.c 
                     lib/ssd1306.c 
                     lib/buzzer.c)

pico_set_program_name(snake "snake")
pico_set_program_version(snake "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(snake 1)
pico_enable_stdio_usb(snake 1)

# Add the standard library to the build
target_link_libraries(snake
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_pwm
        hardware_pio
        pico_bootrom)

# Add the standard include files to the build
target_include_directories(snake PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(snake 
        
        )

pico_generate_pio_header(snake ${CMAKE_CURRENT_LIST_DIR}/lib/ws2818b.pio)


pico_add_extra_outputs(snake)

