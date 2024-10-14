
cmake_minimum_required(VERSION 3.12)

set(ENV{PICO_SDK_PATH} "/opt/badtuna/third_party/pico-sdk")
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(badtuna_display_pico)
pico_sdk_init()

add_executable(badtuna_display_pico
iodevice.c
hardware.c
display.c
display_fonts.c
main.c
)

pico_set_program_name(badtuna_display_pico "BadTuna Display (PICO)")
pico_set_program_version(badtuna_display_pico "1.20")

pico_enable_stdio_usb(badtuna_display_pico 1)
pico_enable_stdio_uart(badtuna_display_pico 0)

target_link_libraries(badtuna_display_pico pico_stdlib hardware_i2c hardware_spi hardware_adc)
target_compile_options(badtuna_display_pico PUBLIC -O6 -Wall -Wextra -pipe)

pico_add_extra_outputs(badtuna_display_pico)

install(CODE "execute_process(COMMAND /usr/local/bin/picotool load -f badtuna_display_pico.uf2)")

