#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/spi.h"
#include "pins.h"
#include "hardware/pwm.h"
#include "auth_store.h"

void setup_spi(){
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    spi_init(SPI_PORT, 1000000);
    spi_set_format(SPI_PORT, 8, 0, 0, SPI_MSB_FIRST);
}

void setup_pwm() {
    gpio_set_function(PIN_LED_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(PIN_LED_RED, GPIO_FUNC_PWM);
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);

    uint green = pwm_gpio_to_slice_num(PIN_LED_GREEN);
    uint red = pwm_gpio_to_slice_num(PIN_LED_RED);
    uint buzzer = pwm_gpio_to_slice_num(PIN_BUZZER);

    pwm_set_wrap(green, 255);
    pwm_set_wrap(red, 255);
    pwm_set_wrap(buzzer, 255);

    pwm_set_enabled(green, true);
    pwm_set_enabled(red, true);
    pwm_set_enabled(buzzer, true);
}
void pwm_fail(){
    pwm_set_gpio_level(PIN_LED_GREEN, 0);
    pwm_set_gpio_level(PIN_LED_RED, 255); 
    pwm_set_gpio_level(PIN_BUZZER, 255);
}
void pwm_success(){
    pwm_set_gpio_level(PIN_LED_RED, 0);
    pwm_set_gpio_level(PIN_BUZZER, 0); 
    pwm_set_gpio_level(PIN_LED_GREEN, 255); 
}
void pwm_reset(){
    pwm_set_gpio_level(PIN_LED_RED, 0); 
    pwm_set_gpio_level(PIN_LED_GREEN, 0);
    pwm_set_gpio_level(PIN_BUZZER, 0); 
}

uint8_t read_rfid(uint8_t reg){
    uint8_t tx[2];
    uint8_t rx[2];

    tx[1]=0x00;
    tx[0] = ((reg << 1) & 0x7E) | 0x80;

    gpio_put(PIN_CS, 0); //allows spi to talk to the rfid
    
    spi_write_read_blocking(SPI_PORT, tx, rx, 2); //writes tx (src) value to RFID and the rx (dest) value gets the value
    gpio_put(PIN_CS, 1); //stops spi from talk to the rfid
    return rx[1];
}

void rfid_write(uint8_t reg, uint8_t value){
    //RFID will allow the chip to start working

    uint8_t tx[2];
    uint8_t rx[2];

    tx[1] = value;
    tx[0] = ((reg << 1) & 0x7E);
    
    gpio_put(PIN_CS, 0); //allows spi to talk to the rfid
    spi_write_read_blocking(SPI_PORT, tx, rx, 2); //writes tx (src) value to RFID and the rx (dest) value gets the value
    gpio_put(PIN_CS, 1); //stops spi from talk to the rfid
}

void init_rfid(){
    rfid_write(0x01, 0x0F);
    sleep_ms(50);
    rfid_write(0x2A, 0x8D);
    rfid_write(0x2B, 0x3E);
    rfid_write(0x2D, 30);
    rfid_write(0x2C, 0);

    uint8_t temp = read_rfid(0x14);
    rfid_write(0x14, temp | 0x03);
}

bool rfid_get_uid(uint8_t uid[4]) {

    uint8_t irq;
    int timeout;

    rfid_write(0x04, 0x7F); //IRQ register
    rfid_write(0x01, 0x00); //Command Register
    rfid_write(0x0A, 0x80); //FIFO Level Register
    
    
    rfid_write(0x0D, 0x07); //Bit Framing Register, awaiting a 7bit message
    rfid_write(0x09, 0x26); //FIFO Data Register, REQA queued
    rfid_write(0x01, 0x0C); //Command Register,  communicate

    
    //first one waits for REQA (0x26) to respond
    timeout = 1000;
    do {
        irq = read_rfid(0x04);
        timeout--;
    } while (!(irq & 0x30) && timeout);
    rfid_write(0x0D, 0x00);
    if (timeout == 0) return false;
    if (read_rfid(0x06) & 0x1B) return false; //Error register check

    //communication, anticollision
    rfid_write(0x01, 0x00); //Command Register, to stop whatever it is doing

    //PICC_CMD_SEL_CL1 = 0x93,
    //Anti collision/Select, Cascade Level 1
    rfid_write(0x09, 0x93); //FIFO Data Register
    rfid_write(0x09, 0x20); //FIFO Data Register
    rfid_write(0x01, 0x0C); //Command Register, to communcate the data

    //second one waits for UID
    do {
        irq = read_rfid(0x04);
        timeout--;
    } while (!(irq & 0x30) && timeout);

    if (timeout == 0) return false;
    if (read_rfid(0x06) & 0x1B) return false; //Error register check

    uint8_t len = read_rfid(0x0A);
    if (len < 5) return false;

    for (int i = 0; i < 4; i++) {
        uid[i] = read_rfid(0x09);
    }
    return true;
}

int main() {
    stdio_init_all();
    setup_spi();
    setup_pwm();
    init_rfid();
    auth_store_init();
    display_init();
    display_idle();
    uint8_t uid[4];

    //uint8_t x = read_rfid(0x0);
    for (;;) {
        if(rfid_get_uid(uid))
        {
            if(auth_store_check_uid(uid)){
                display_granted();
                pwm_success();
            }
            else{
                pwm_fail();
                display_denied();
            }
            
        }
        sleep_ms(200);
        pwm_reset();

    }
}