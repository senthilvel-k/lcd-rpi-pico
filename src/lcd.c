/* ===========================================================
 * lcd.c — HD44780 16×2 LCD Driver for Raspberry Pi Pico
 * Author : Senthil Vel K (skumara4)
 * Date   : October 2023
 * Org    : Visteon Corporation
 *
 * Interface : 4-bit parallel (GPIO 0–5)
 * Protocol  : HD44780 compatible
 * =========================================================== */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

/* ── Pin Definitions ──────────────────────────────────────── */
#define LCD_RS_PIN  0   /* Register Select: 0=command, 1=data  */
#define LCD_EN_PIN  1   /* Enable: latch data on falling edge   */
#define LCD_D4_PIN  2   /* Data bit 4 (upper nibble, bit 0)    */
#define LCD_D5_PIN  3   /* Data bit 5 (upper nibble, bit 1)    */
#define LCD_D6_PIN  4   /* Data bit 6 (upper nibble, bit 2)    */
#define LCD_D7_PIN  5   /* Data bit 7 (upper nibble, bit 3)    */

/* ── Forward Declarations ────────────────────────────────── */
static void lcd_send_byte(uint8_t value, uint8_t mode);
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *str);

/* ===========================================================
 * lcd_send_byte()
 *   Sends a single byte to the LCD in 4-bit mode.
 *   The byte is split into two nibbles (upper first, lower second).
 *   Each nibble is clocked into the LCD with an EN pulse.
 *
 *   value : byte to send (command byte or ASCII character)
 *   mode  : 0 = command register (RS low)
 *            1 = data register   (RS high)
 * =========================================================== */
static void lcd_send_byte(uint8_t value, uint8_t mode) {
    gpio_put(LCD_RS_PIN, mode);

    /* ── Upper nibble (bits 7..4) ── */
    gpio_put(LCD_D4_PIN, (value >> 4) & 1);
    gpio_put(LCD_D5_PIN, (value >> 5) & 1);
    gpio_put(LCD_D6_PIN, (value >> 6) & 1);
    gpio_put(LCD_D7_PIN, (value >> 7) & 1);

    /* EN pulse: HIGH for ≥1 µs then LOW to latch */
    gpio_put(LCD_EN_PIN, 1);
    sleep_us(1);
    gpio_put(LCD_EN_PIN, 0);

    /* ── Lower nibble (bits 3..0) ── */
    gpio_put(LCD_D4_PIN, value & 1);
    gpio_put(LCD_D5_PIN, (value >> 1) & 1);
    gpio_put(LCD_D6_PIN, (value >> 2) & 1);
    gpio_put(LCD_D7_PIN, (value >> 3) & 1);

    /* EN pulse */
    gpio_put(LCD_EN_PIN, 1);
    sleep_us(1);
    gpio_put(LCD_EN_PIN, 0);

    /* Command settle time — HD44780 requires ≥ 37 µs (use 2 ms for safety) */
    sleep_ms(2);
}

/* ===========================================================
 * lcd_init()
 *   1. Initializes all 6 GPIO pins as outputs.
 *   2. Runs the HD44780 initialization sequence:
 *        0x33 — begin init (ensures 8-bit mode start)
 *        0x32 — switch to 4-bit mode
 *        0x28 — 2-line display, 5×8 dot font (DL=0, N=1, F=0)
 *        0x0C — display ON, cursor OFF, blink OFF
 *        0x01 — clear display, return cursor home
 * =========================================================== */
void lcd_init(void) {
    uint8_t pins[] = {
        LCD_RS_PIN, LCD_EN_PIN,
        LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN
    };

    for (int i = 0; i < 6; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    /* HD44780 initialization sequence */
    lcd_send_byte(0x33, 0);  /* Initialize                        */
    lcd_send_byte(0x32, 0);  /* Set 4-bit mode                    */
    lcd_send_byte(0x28, 0);  /* 2 lines, 5×8 font                 */
    lcd_send_byte(0x0C, 0);  /* Display on, cursor off, blink off */
    lcd_send_byte(0x01, 0);  /* Clear display                     */
    sleep_ms(2);
}

/* ===========================================================
 * lcd_clear()
 *   Sends command 0x01 — clears DDRAM and returns cursor home.
 * =========================================================== */
void lcd_clear(void) {
    lcd_send_byte(0x01, 0);
    sleep_ms(2);
}

/* ===========================================================
 * lcd_set_cursor()
 *   Sets the DDRAM address to position the cursor.
 *
 *   DDRAM map (16-column display):
 *     Row 0: 0x80 + col  (0x80 .. 0x8F)
 *     Row 1: 0xC0 + col  (0xC0 .. 0xCF)
 *
 *   row : 0 (top line) or 1 (bottom line)
 *   col : 0 – 15
 * =========================================================== */
void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t position = 0x80;
    if (row == 1) position += 0x40;  /* Row 1 offset */
    position += col;
    lcd_send_byte(position, 0);      /* RS=0 → command */
}

/* ===========================================================
 * lcd_print()
 *   Writes a null-terminated string to the LCD at the current
 *   cursor position. Each character is sent as a data byte (RS=1).
 * =========================================================== */
void lcd_print(const char *str) {
    while (*str) {
        lcd_send_byte((uint8_t)*str, 1);
        str++;
    }
}

/* ===========================================================
 * main()
 *   1. Initialize GPIO and LCD.
 *   2. Print "Senthil Vel" on row 0.
 *   3. Loop: update row 1 with characters cycling A → Z.
 * =========================================================== */
int main(void) {
    stdio_init_all();

    lcd_init();
    lcd_clear();

    /* Row 0: static label */
    lcd_set_cursor(0, 0);
    lcd_print("Senthil Vel");

    /* Row 1: live character counter A–Z */
    char counter = 'A';

    while (1) {
        char message[2] = { counter, '\0' };
        lcd_set_cursor(1, 0);
        lcd_print(message);

        sleep_ms(1000);

        /* Advance A → Z → A */
        counter = (counter < 'Z') ? counter + 1 : 'A';
    }

    return 0;
}
