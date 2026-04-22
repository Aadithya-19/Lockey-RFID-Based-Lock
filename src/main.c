#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/spi.h"
#include "pins.h"
#include "hardware/pwm.h"
#include "auth_store.h"
#include "pico/stdlib.h"
#include "mfrc522.h"

#define MAX_CARDS 10

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
    pwm_fail();
    pwm_success();
    pwm_reset();

    MFRC522Ptr_t mfrc = MFRC522_Init();
    PCD_Init(mfrc, SPI_PORT);
    printf("RFID initialized\n");
    uint8_t temp [4];
    
    while (true) {
        if (PICC_IsNewCardPresent(mfrc)) {
            if (PICC_ReadCardSerial(mfrc)) {

                printf("UID: ");
                for (int i = 0; i < mfrc->uid.size; i++) {
                    printf("%02X ", mfrc->uid.uidByte[i]);
                    temp [i] = mfrc->uid.uidByte[i];
                   
                }
                printf("\n");
                int suc = 0;
                for (int i = 0;  i < num_cards; i++){
                    pwm_reset();
                    for (int j = 0; j < mfrc->uid.size; j++){
                        if (temp [j] == authorized_uids[i][j]){
                            suc++;
                        }
                        else{ 
                            break;
                        }
                    }
                }
                if (suc==4){
                    pwm_success();
                    printf("Access Granted");
                }
                else{
                    pwm_fail();
                    printf("Access Denied");
                }
            }
            
        }
        sleep_ms(200);
        pwm_reset();

    }
}