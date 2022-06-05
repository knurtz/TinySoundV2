/* 
 * TinySound v2 board file for Pico SDK
 *
 * Based on pico.h by Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_TINYSOUND_H
#define _BOARDS_TINYSOUND_H

// For board detection
#define TINYSOUND_V2

// --- UART ---
#define PICO_DEFAULT_UART 1
#define PICO_DEFAULT_UART_TX_PIN 4
#define PICO_DEFAULT_UART_RX_PIN 5
#define PICO_STDIO_DEFAULT_CRLF 0

// no PICO_DEFAULT_LED_PIN ---

// no PICO_DEFAULT_WS2812_PIN

// no I2C, no SPI

// --- FLASH ---

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)     // reserve 1M for actual program, rest of 16M for mass storage
#endif

#ifndef PICO_RP2040_B2_SUPPORTED
#define PICO_RP2040_B2_SUPPORTED 1
#endif


#endif  // _BOARDS_TINYSOUND_H
