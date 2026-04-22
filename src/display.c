#include "display.h"
#include "font.h"
#include "pins.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"



static void send_command(uint8_t cmd) {
    gpio_put(PIN_LCD_DC, 0);
    gpio_put(PIN_LCD_CSn, 0);
    spi_write_blocking(LCD_SPI_PORT, &cmd, 1);
    gpio_put(PIN_LCD_CSn, 1);
}

static void send_data(uint8_t data) {
    gpio_put(PIN_LCD_DC, 1);
    gpio_put(PIN_LCD_CSn, 0);
    spi_write_blocking(LCD_SPI_PORT, &data, 1);
    gpio_put(PIN_LCD_CSn, 1);
}


static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    send_command(0x2A);
    send_data(x0 >> 8); send_data(x0 & 0xFF);
    send_data(x1 >> 8); send_data(x1 & 0xFF);

    send_command(0x2B);
    send_data(y0 >> 8); send_data(y0 & 0xFF);
    send_data(y1 >> 8); send_data(y1 & 0xFF);

    send_command(0x2C);
}
void display_init(void) {
    spi_init(LCD_SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_TX, GPIO_FUNC_SPI);

    gpio_init(PIN_LCD_DC);    gpio_set_dir(PIN_LCD_DC, GPIO_OUT);
    gpio_init(PIN_LCD_RESET); gpio_set_dir(PIN_LCD_RESET, GPIO_OUT);
    gpio_init(PIN_LCD_CSn);   gpio_set_dir(PIN_LCD_CSn, GPIO_OUT);
    gpio_put(PIN_LCD_CSn, 1); 

    gpio_put(PIN_LCD_RESET, 0); sleep_ms(10);
    gpio_put(PIN_LCD_RESET, 1); sleep_ms(120);

    send_command(0x11);
    sleep_ms(120);

    send_command(0x3A);
    send_data(0x55);

    send_command(0x36);
    send_data(0x08);

    send_command(0x29);
}

void display_fill(uint16_t color) {
    set_window(0, 0, 239, 319);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    gpio_put(PIN_LCD_DC, 1);
    gpio_put(PIN_LCD_CSn, 0);
    for (int i = 0; i < 240 * 320; i++) {
        spi_write_blocking(LCD_SPI_PORT, &hi, 1);
        spi_write_blocking(LCD_SPI_PORT, &lo, 1);
    }
    gpio_put(PIN_LCD_CSn, 1);
}

static void draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    int idx = -1;
    for (int i = 0; i < sizeof(font) / sizeof(font[0]); i++) {
        if (font[i].letter == c) { idx = i; break; }
    }
    if (idx == -1) return;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            uint16_t pixel = (font[idx].code[row][col] == '#') ? color : bg;
            
            // Draw a block of pixels for each font pixel to scale it up
            set_window(x + (col * size), y + (row * size), x + (col * size) + size - 1, y + (row * size) + size - 1);
            gpio_put(PIN_LCD_DC, 1);
            gpio_put(PIN_LCD_CSn, 0);
            for (int p = 0; p < size * size; p++) {
                uint8_t hi = pixel >> 8;
                uint8_t lo = pixel & 0xFF;
                spi_write_blocking(LCD_SPI_PORT, &hi, 1);
                spi_write_blocking(LCD_SPI_PORT, &lo, 1);
            }
            gpio_put(PIN_LCD_CSn, 1);
        }
    }
}

// Scaled string drawing
static void draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(x + i * (6 * size), y, str[i], color, bg, size);
    }
}

// Updated screens using scale factor of 3 and adjusted X/Y coordinates to center them
void display_granted(void) {
    display_fill(COLOR_GREEN);
    draw_string(20, 150, "ACCESS GRANTED", COLOR_WHITE, COLOR_GREEN, 3);
}

void display_denied(void) {
    display_fill(COLOR_RED);
    draw_string(30, 150, "ACCESS DENIED", COLOR_WHITE, COLOR_RED, 3);
}
void display_tamper(void) {
    display_fill(COLOR_RED);
    draw_string(10, 140, "TAMPER DETECTED", COLOR_WHITE, COLOR_RED, 3);
    draw_string(40, 180, "SYSTEM LOCKED", COLOR_WHITE, COLOR_RED, 2);
}
void display_stored(void) {
    display_fill(COLOR_GREEN);
    draw_string(30, 140, "CARD STORED", COLOR_WHITE, COLOR_GREEN, 3);
    draw_string(60, 180, "SUCCESS", COLOR_WHITE, COLOR_GREEN, 2);
}

void display_idle(void) {
    display_fill(COLOR_BLACK);
    draw_string(50, 150, "SCAN CARD", COLOR_WHITE, COLOR_BLACK, 3);
}