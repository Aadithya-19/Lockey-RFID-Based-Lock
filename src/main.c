#include "pico/stdlib.h"


void out_spi(){}


int main() {
    stdio_init_all();

    for (;;) {
        printf("Hello world!\n");
        sleep_ms(1000);
    }
}