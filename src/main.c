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

#define MAX_CARDS 10
#define MAX_ATTEMPTS 3

uint8_t authorized_uids[MAX_CARDS][4] = {
    {0xDE, 0xAD, 0xBE, 0xEF},
    {0x08, 0x4F, 0x50, 0x36},
    {0x67, 0x3F, 0xCA, 0x31}
};

int num_cards = 3;



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
void pwm_tamper_alarm(){
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


int main() {
    stdio_init_all();
    setup_pwm();
    pwm_reset();

    // Initialize the display and show the idle screen
    display_init();
    display_idle();

    MFRC522Ptr_t mfrc = MFRC522_Init();
    PCD_Init(mfrc, SPI_PORT);
    printf("RFID initialized\n");
    
    int failed_attempts = 0; 
    
    while (true) {
        if (PICC_IsNewCardPresent(mfrc)) {
            if (PICC_ReadCardSerial(mfrc)) {

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
                    sleep_ms(2000); 
                } 
                else {
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
                
                pwm_reset();
                display_idle(); 
            }
        }
        sleep_ms(50);
    }
}
