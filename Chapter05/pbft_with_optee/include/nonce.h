#ifndef NONCE_H
#define NONCE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NONCE_COUNT 100000
#define NONCE_SIZE 16

int is_nonce_used(uint8_t nonces[][NONCE_SIZE], size_t nonces_count, uint8_t nonce[NONCE_SIZE]);

#endif