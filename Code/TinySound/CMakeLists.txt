cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Set custom board file for TinySound v2
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/board")
set(PICO_BOARD tinysound_v2)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(TinySound C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executables
add_executable(TinySound 
    TinySound.c 
    TS_flash.c
    TS_shell.c
)

pico_set_program_name(TinySound "TinySound")
pico_set_program_version(TinySound "0.1")

# Enable a summary of used memory at the end of linking process
target_link_options(TinySound PUBLIC "-Wl,--print-memory-usage")
add_custom_command(TARGET TinySound
    POST_BUILD
    COMMENT "Code size:"
    COMMAND arm-none-eabi-size TinySound.elf
    VERBATIM
)

# Add the standard library to the build
target_link_libraries(TinySound 
    pico_stdlib
)

# Add any user requested libraries
target_link_libraries(TinySound
    hardware_dma
    hardware_pio
    hardware_irq
    hardware_flash
    hardware_sync
)

pico_add_extra_outputs(TinySound)