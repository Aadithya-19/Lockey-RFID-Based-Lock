#include "auth_store.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define STORE_ADDR 0x50
#define STORE_I2C i2c0
#define SDA_PIN 4
#define SCL_PIN 5
#define COUNT_ADDR 0x000
#define UID_START 0x001

#define write_delay 5

void auth_store_init(void) {
    i2c_init(STORE_I2C, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

uint8_t auth_store_get_count(void)
{
    uint8_t count;
    uint8_t addr[2] = {0x00, 0x00};
    i2c_write_blocking(STORE_I2C, STORE_ADDR, addr, 2, true);
    i2c_read_blocking(STORE_I2C, STORE_ADDR, &count, 1, false);
    if (count == 0xFF){
        count = 0;
    }
    return count;
}
bool auth_store_add_uid(const uint8_t uid[UID_SIZE])
{
    uint8_t count = auth_store_get_count();
    uint16_t write_addr = UID_START + (count * UID_SIZE);
    uint8_t packet[6];
    packet[0] = (write_addr >> 8) & 0xFF;
    packet[1] = write_addr  & 0xFF;
    for( int i = 0; i< UID_SIZE; i++ )
    {
        packet[2 + i] = uid[i];
    }
    i2c_write_blocking(STORE_I2C, STORE_ADDR,packet, 6, false);
    sleep_ms(write_delay);
    count++;
    uint8_t count_packet[3] = {0x00, 0x00, count};
    i2c_write_blocking(STORE_I2C, STORE_ADDR, count_packet, 3, false);
    sleep_ms(write_delay);
    return true;
}

bool auth_store_check_uid(const uint8_t uid[UID_SIZE]){
    uint8_t count = auth_store_get_count();
    if(count == 0) {
        return false;
    }
    uint8_t stored[UID_SIZE];

    for(int i = 0; i < count; i++){
        uint16_t read_addr = UID_START + (i * UID_SIZE);
        uint8_t addr[2] = {(read_addr >> 8) & 0xFF, read_addr & 0xFF};
        i2c_write_blocking(STORE_I2C, STORE_ADDR, addr, 2, true);
        i2c_read_blocking(STORE_I2C, STORE_ADDR, stored, UID_SIZE, false);

        bool match = true;
        for(int j = 0; j < UID_SIZE; j++)
        {
            if(stored[j] != uid[j])
            {
                match = false;
                break;
            }
        }
        if(match){
            return true;
        }
    }
    return false;
}