cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Set custom board file for TinySound v2
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/board")
set(PICO_BOARD tinysound_v2)

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)
# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(TinySound C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executables
add_executable(TinySound 
    # Main
    TinySound.c

    # TinySound specific
    TS_shell.c
    TS_fat.c
    TS_audio.c

    # USB / Mass storage
    usb/usb_callbacks.c
    usb/usb_descriptors.c
    usb/msc_disk.c
    usb/flash_functions.c

    # FatFS
    ff15/source/diskio.c
    ff15/source/ffunicode.c
    ff15/source/ff.c
)

target_include_directories(TinySound PRIVATE
    .
    usb
    ff15/source
)

pico_set_linker_script(TinySound ${CMAKE_CURRENT_SOURCE_DIR}/custom_linker_script.ld)

pico_set_program_name(TinySound "TinySound")
pico_set_program_version(TinySound "0.1")

# Add the standard library to the build
target_link_libraries(TinySound 
    pico_stdlib
    pico_bootsel_via_double_reset
    
    hardware_dma
    hardware_pio
    hardware_irq
    hardware_flash
    hardware_sync

    tinyusb_device
)

pico_generate_pio_header(TinySound ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio)

pico_enable_stdio_usb(TinySound 0)
pico_enable_stdio_uart(TinySound 1)

target_compile_definitions(TinySound PRIVATE
    # bootloader activity LED
    PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=28
)

pico_add_extra_outputs(TinySound)

# Enable a summary of used memory at the end of linking process
target_link_options(TinySound PUBLIC "-Wl,--print-memory-usage")
# target_link_options(TinySound PUBLIC "-Wl,--no-warn-rwx-segment")
add_custom_command(TARGET TinySound
    POST_BUILD
    COMMENT "Code size:"
    COMMAND arm-none-eabi-size TinySound.elf
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/modify_uf2.py ${CMAKE_CURRENT_SOURCE_DIR}/build/TinySound.uf2 0x10100000
    VERBATIM
)