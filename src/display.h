#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_fill(uint16_t color);
void display_granted(void);
void display_denied(void);
void display_idle(void);

#endif