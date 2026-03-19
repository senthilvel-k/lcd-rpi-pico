cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(lcd-rpi-pico C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(lcd
    src/lcd.c
)

pico_add_extra_outputs(lcd)
target_link_libraries(lcd pico_stdlib)
pico_enable_stdio_usb(lcd  1)
pico_enable_stdio_uart(lcd 0)
