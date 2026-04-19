#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>


// Red=5 bits, Green=6b its, Blue=5 bits
#define COLOR_GREEN  0x07E0  // 00000 111111 00000
#define COLOR_RED    0xF800  // 11111 000000 00000
#define COLOR_BLACK  0x0000
#define COLOR_WHITE  0xFFFF

void display_init(void);
void display_fill(uint16_t color);
void display_granted(void);
void display_denied(void);
void display_idle(void);

#endif