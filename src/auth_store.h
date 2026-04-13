#ifndef AUTH_STORE_H
#define AUTH_STORE_H

#include <stdint.h>
#include <stdbool.h>

#define UID_SIZE 4

void auth_store_init(void);
bool auth_store_add_uid(const uint8_t uid[UID_SIZE]);
bool auth_store_check_uid(const uint8_t uid[UID_SIZE]);
uint8_t auth_store_get_count(void);
#endif
