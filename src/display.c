#include "display.h"
#include "pins.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pic/stdlib.h"

static void send_command(uint8_t cmd)