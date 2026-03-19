# LCD Display with Raspberry Pi Pico

> **Bare-metal HD44780 LCD driver written from scratch in C — no external library, pure GPIO control.**

[![Language](https://img.shields.io/badge/Language-C%20%28C11%29-blue?style=flat-square&logo=c)](https://en.cppreference.com/w/c/11)
[![SDK](https://img.shields.io/badge/SDK-Pico%20SDK-brightgreen?style=flat-square)](https://github.com/raspberrypi/pico-sdk)
[![Build](https://img.shields.io/badge/Build-CMake-orange?style=flat-square&logo=cmake)](https://cmake.org/)
[![Hardware](https://img.shields.io/badge/MCU-RP2040-red?style=flat-square)](https://www.raspberrypi.com/products/raspberry-pi-pico/)
[![LCD](https://img.shields.io/badge/Display-HD44780%2016×2-yellowgreen?style=flat-square)](#hardware)
[![Author](https://img.shields.io/badge/Author-Senthil%20Vel%20K-9cf?style=flat-square)](https://senthilvel-k.github.io/)

---

**[🌐 Project Demo Site](https://senthilvel-k.github.io/lcd-rpi-pico/)** &nbsp;·&nbsp; **[👤 Portfolio](https://senthilvel-k.github.io/)** &nbsp;·&nbsp; **[📄 Full Report (PDF)](./docs/LCD_Report.pdf)**

---

## Overview

This project implements a complete 16×2 LCD display driver for the **Raspberry Pi Pico** using its RP2040 microcontroller — written entirely in bare-metal C with the Pico SDK. The HD44780 parallel interface is operated in **4-bit mode** over 6 GPIO pins, with all timing, nibble splitting, enable pulse generation, and initialization sequencing implemented from scratch.

**Line 1** displays a static string. **Line 2** dynamically cycles through characters A–Z in a 1-second loop, demonstrating real-time display refresh without an OS or scheduler.

```
┌──────────────────────┐
│ Senthil Vel          │   ← Line 1 — static text
│ A                    │   ← Line 2 — cycles A → Z → A ...
└──────────────────────┘
```

---

## Repository Structure

```
lcd-rpi-pico/
├── src/
│   └── lcd.c              # Full LCD driver + main application
├── CMakeLists.txt          # Build configuration (Pico SDK)
├── docs/
│   └── LCD_Report.pdf     # Project report (Visteon, Oct 2023)
├── images/
│   └── hardware.jpg       # Breadboard photo
└── README.md
```

---

## Hardware

| LCD Pin | Signal | Raspberry Pi Pico GPIO |
|---------|--------|------------------------|
| RS | Register Select | GPIO 0 |
| EN | Enable | GPIO 1 |
| D4 | Data bit 4 | GPIO 2 |
| D5 | Data bit 5 | GPIO 3 |
| D6 | Data bit 6 | GPIO 4 |
| D7 | Data bit 7 | GPIO 5 |
| V0 | Contrast | Potentiometer wiper |
| VDD | Power | 3.3 V / 5 V |
| VSS | Ground | GND |
| A  | Backlight + | 3.3 V (via resistor) |
| K  | Backlight − | GND |

> **Note:** RW pin is tied to GND (write-only mode). Only 6 GPIO pins are needed.

**Parts list:**
- Raspberry Pi Pico (RP2040)
- HD44780-compatible 16×2 LCD module
- 10 kΩ potentiometer (contrast)
- Jumper wires + breadboard

---

## How It Works

### 4-bit Parallel Protocol (HD44780)

The LCD communicates over a parallel bus. In 4-bit mode, every byte is sent as two sequential **nibbles** (4 bits each) on the D4–D7 lines. After each nibble, the **EN pin is pulsed HIGH→LOW** to latch the data into the LCD's internal register.

```
Sending 'A' (ASCII 0x41 = 0100 0001):

  RS  ─────────────────────────────  HIGH (data mode)
  EN  ──┐   ┌──────────┐   ┌──────
        └───┘          └───┘
  D7-D4   [0100]         [0001]
            ↑ upper nibble  ↑ lower nibble
```

The **RS pin** selects the destination register:
- `RS = 0` → **Command register** (clear screen, set cursor position, display on/off)
- `RS = 1` → **Data register** (write ASCII character to DDRAM → display)

### Initialization Sequence

The HD44780 requires a specific boot sequence after power-up:

```c
lcd_send_byte(0x33, 0);  // Initialize (8-bit mode start)
lcd_send_byte(0x32, 0);  // Switch to 4-bit mode
lcd_send_byte(0x28, 0);  // 2-line display, 5×8 dot font
lcd_send_byte(0x0C, 0);  // Display ON, cursor OFF, blink OFF
lcd_send_byte(0x01, 0);  // Clear display, reset cursor
sleep_ms(2);             // Allow LCD to process clear
```

### Cursor Addressing (DDRAM)

The LCD's internal memory (DDRAM) maps to screen positions:

| Row | DDRAM Base | Example: Column 3 |
|-----|------------|-------------------|
| 0 (top) | `0x80` | `0x80 + 3 = 0x83` |
| 1 (bottom) | `0xC0` | `0xC0 + 3 = 0xC3` |

```c
void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t position = 0x80;
    if (row == 1) position += 0x40;  // offset to row 1
    position += col;
    lcd_send_byte(position, 0);      // send as command (RS=0)
}
```

---

## Code

### `lcd_send_byte()` — Core driver function

```c
static void lcd_send_byte(uint8_t value, uint8_t mode) {
    gpio_put(LCD_RS_PIN, mode);           // RS: 0=cmd, 1=data

    // Send upper nibble (bits 7..4)
    gpio_put(LCD_D4_PIN, (value >> 4) & 1);
    gpio_put(LCD_D5_PIN, (value >> 5) & 1);
    gpio_put(LCD_D6_PIN, (value >> 6) & 1);
    gpio_put(LCD_D7_PIN, (value >> 7) & 1);
    gpio_put(LCD_EN_PIN, 1); sleep_us(1); gpio_put(LCD_EN_PIN, 0);

    // Send lower nibble (bits 3..0)
    gpio_put(LCD_D4_PIN, value & 1);
    gpio_put(LCD_D5_PIN, (value >> 1) & 1);
    gpio_put(LCD_D6_PIN, (value >> 2) & 1);
    gpio_put(LCD_D7_PIN, (value >> 3) & 1);
    gpio_put(LCD_EN_PIN, 1); sleep_us(1); gpio_put(LCD_EN_PIN, 0);

    sleep_ms(2);  // command settle time
}
```

### API Summary

| Function | Description |
|----------|-------------|
| `lcd_init()` | Initialize GPIO pins + run HD44780 boot sequence |
| `lcd_clear()` | Send 0x01 command — wipe DDRAM, home cursor |
| `lcd_set_cursor(row, col)` | Set cursor to row 0–1, column 0–15 |
| `lcd_print(const char *str)` | Print null-terminated string character by character |
| `lcd_send_byte(value, mode)` | Send single byte as command (mode=0) or data (mode=1) |

---

## Build & Flash

### Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) installed
- CMake ≥ 3.13
- ARM GCC toolchain (`arm-none-eabi-gcc`)

### Build

```bash
git clone https://github.com/senthilvel-k/lcd-rpi-pico.git
cd lcd-rpi-pico
mkdir build && cd build
cmake ..
make -j4
```

This generates `lcd.uf2` in the `build/` directory.

### Flash

1. Hold the **BOOTSEL** button on the Pico while connecting USB
2. It mounts as a mass storage device (`RPI-RP2`)
3. Copy `lcd.uf2` to the drive — Pico reboots and runs automatically

```bash
cp build/lcd.uf2 /media/$USER/RPI-RP2/
```

---

## What I Learned

- **HD44780 protocol internals** — initialization timing constraints, why the 0x33→0x32 sequence is needed, command vs data modes
- **4-bit nibble mode** — how to split a byte and send it in two clocked bursts, saving 4 GPIO lines
- **DDRAM addressing** — how the LCD maps its internal 80-byte memory to screen coordinates
- **GPIO driver fundamentals** — using `gpio_init`, `gpio_set_dir`, `gpio_put` from the Pico SDK at a low level
- **Timing sensitivity** — the EN pulse must be ≥1 µs wide; commands need ≥2 ms settle time or the display misbehaves silently

---

## Context

This was built as a **personal learning project** during my time at Visteon Corporation, where I work on production automotive ECU firmware (AUTOSAR, ISO 26262, CAN, Ethernet). The LCD project was a deliberate step back to first principles — building a driver from scratch to reinforce low-level GPIO and parallel bus fundamentals outside the structured AUTOSAR stack.

> See my full portfolio and production ECU work: **[senthilvel-k.github.io](https://senthilvel-k.github.io/)**

---

## License

MIT License — free to use, modify, and learn from.

---

*Senthil Vel K · skumara4 · Visteon Corporation · October 2023*
