#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/spi.h"
#include "pins.h"
#include "auth_store.h"

void setup_spi(){
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    spi_init(SPI_PORT, 150000);
    spi_set_format(SPI_PORT, 8, 0, 0, SPI_MSB_FIRST);
}

//0x0 = fail
//0x1 = success

uint8_t read_rfid(uint8_t reg){
    uint8_t tx[2];
    uint8_t rx[2];

    tx[1]=0x00;
    tx[0] = (reg << 1) & 0x7E;
    tx[0] |= 0x80;

    gpio_put(PIN_CS, 0);
    
    spi_write_read_blocking(SPI_PORT, tx, rx, 2);
    gpio_put(PIN_CS, 1);
    return 0;
}

//replace this with actual RFID READING FUCNTION OKAY? 
bool rfid_get_uid(uint8_t uid[4])
{
    return false;
}


void setup_leds(){
    gpio_init(PIN_LED_GREEN);
    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_GREEN, GPIO_OUT);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);
    gpio_put(PIN_LED_GREEN, 0);
    gpio_put(PIN_LED_RED, 0);
}


int main() {
    stdio_init_all();
    setup_spi();
    setup_leds();
    auth_store_init();
    display_init();
    display_idle();
    uint8_t uid[4];

    //uint8_t x = read_rfid(0x0);
    for (;;) {
        if(rfid_get_uid(uid))
        {
            if(auth_store_check_uid(uid))
            {
                display_granted();
                gpio_put(PIN_LED_GREEN,1);
                sleep_ms(3000);
                gpio_put(PIN_LED_GREEN,0);
            }
            else{
                display_denied();
                gpio_put(PIN_LED_RED,1);
                sleep_ms(3000);
                gpio_put(PIN_LED_RED,0);
            }
            display_idle();
        }
    }
}
