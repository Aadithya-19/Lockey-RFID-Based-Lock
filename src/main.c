#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/spi.h"
#include "pins.h"
#include "hardware/pwm.h"
#include "auth_store.h"
#include "pico/stdlib.h"
#include "mfrc522.h"
#include "display.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define EEPROM_ADDR 0x50
#define MAX_CARDS 10
#define MAX_ATTEMPTS 3

uint8_t authorized_uids[MAX_CARDS][4] = {
    {0xDE, 0xAD, 0xBE, 0xEF},
    {0x08, 0x4F, 0x50, 0x36},
    {0x67, 0x3F, 0xCA, 0x31}
};

int num_cards = 3;

uint pwm_slice;

void setup_servo() {
    gpio_set_function(PIN_SERVO, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(PIN_SERVO);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 64.0f);
    pwm_config_set_wrap(&config, 46875);
    pwm_init(pwm_slice, &config, true);
    pwm_set_chan_level(pwm_slice, pwm_gpio_to_channel(PIN_SERVO), 3281);
     // start locked
}

void lock() {
    pwm_set_chan_level(pwm_slice, pwm_gpio_to_channel(PIN_SERVO), 3281);
    printf("Locked\n");
}

void unlock() {
    pwm_set_chan_level(pwm_slice, pwm_gpio_to_channel(PIN_SERVO), 5481);
    printf("Unlocked\n");
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
    pwm_set_clkdiv(buzzer, 125.0f); 
    pwm_set_wrap(buzzer, 500); 

    pwm_set_enabled(green, true);
    pwm_set_enabled(red, true);
    pwm_set_enabled(buzzer, true);
}

void pwm_fail(){
    pwm_set_gpio_level(PIN_LED_GREEN, 0);
    pwm_set_gpio_level(PIN_LED_RED, 255); 
    
    pwm_set_gpio_level(PIN_BUZZER, 250); 
}

void pwm_tamper_alarm(){
    pwm_set_gpio_level(PIN_LED_GREEN, 0);
    pwm_set_gpio_level(PIN_LED_RED, 255); 
    
    pwm_set_gpio_level(PIN_BUZZER, 250); 
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

void load_uids() 
{
    for(int i = 0; i < num_cards; i++) {
        uint8_t mem_addr_buf[2];
        uint16_t mem_addr = i * 4; 

        mem_addr_buf[0] = (mem_addr >> 8) & 0xFF; 
        mem_addr_buf[1] = mem_addr & 0xFF;        

        i2c_write_blocking(I2C_PORT, EEPROM_ADDR, mem_addr_buf, 2, true); 
        i2c_read_blocking(I2C_PORT, EEPROM_ADDR, authorized_uids[i], 4, false);
    }
}


int main() {
    stdio_init_all();
    setup_pwm();
    setup_servo();
    pwm_reset();

    i2c_init(I2C_PORT, 100000);
    gpio_set_function(STORE_SDA, GPIO_FUNC_I2C);
    gpio_set_function(STORE_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(STORE_SDA);
    gpio_pull_up(STORE_SCL);
    gpio_init(PIN_STORE);
    gpio_set_dir(PIN_STORE, GPIO_IN);
    gpio_pull_down(PIN_STORE);

    gpio_init(PIN_SWITCH);
    gpio_set_dir(PIN_SWITCH, GPIO_IN);
    gpio_pull_down(PIN_SWITCH);

    load_uids();
    display_init();
    display_idle();

    MFRC522Ptr_t mfrc = MFRC522_Init();
    PCD_Init(mfrc, SPI_PORT);
    printf("RFID initialized\n");

    int failed_attempts = 0;
    bool locked = true;

    while (true) {

        // check switch rlock if door is closed
        if (gpio_get(PIN_SWITCH) == 1 && !locked) {
            lock();
            locked = true;
            pwm_reset();
            display_idle();
        }

        if (PICC_IsNewCardPresent(mfrc)) {
            if (PICC_ReadCardSerial(mfrc)) {

                // enrollment mode  switch held down while scanning
                if (gpio_get(PIN_STORE) == 1) {
                    printf("ENROLLMENT MODE: Saving Card...\n");
                    if (num_cards < MAX_CARDS) {
                        memcpy(authorized_uids[num_cards], mfrc->uid.uidByte, 4);
                        uint8_t buffer[6];
                        uint16_t mem_addr = num_cards * 4;
                        buffer[0] = (mem_addr >> 8) & 0xFF;
                        buffer[1] = mem_addr & 0xFF;
                        memcpy(&buffer[2], mfrc->uid.uidByte, 4);
                        i2c_write_blocking(I2C_PORT, EEPROM_ADDR, buffer, 6, false);
                        sleep_ms(5);
                        num_cards++;
                        pwm_success();
                        display_scanning();
                        display_stored();
                        printf("Card successfully saved to EEPROM!\n");
                        sleep_ms(2000);
                    } else {
                        printf("EEPROM is full!\n");
                        pwm_fail();
                        display_scanning();
                        display_denied();
                        sleep_ms(2000);
                    }
                }
                // authentication mode
                else {
                    display_scanning();
                    printf("AUTHENTICATING...\n");
                    load_uids();

                    int access_granted = 0;
                    for (int i = 0; i < num_cards; i++) {
                        if (memcmp(mfrc->uid.uidByte, authorized_uids[i], mfrc->uid.size) == 0) {
                            access_granted = 1;
                            break;
                        }
                    }

                    if (access_granted == 1) {
                        failed_attempts = 0;
                        pwm_success();
                        display_granted();
                        printf("Access Granted\n");
                        unlock();
                        locked = false;
                        sleep_ms(2000);
                    } else {
                        failed_attempts++;
                        if (failed_attempts >= MAX_ATTEMPTS) {
                            pwm_tamper_alarm();
                            display_tamper();
                            printf("TAMPER WARNING! System Locked.\n");
                            sleep_ms(5000);
                        } else {
                            pwm_fail();
                            display_denied();
                            printf("Access Denied. Attempt %d of %d\n", failed_attempts, MAX_ATTEMPTS);
                            sleep_ms(2000);
                        }
                    }
                }

                pwm_reset();
                display_idle();
                PICC_HaltA(mfrc);
            }
        }
        sleep_ms(50);
    }
}