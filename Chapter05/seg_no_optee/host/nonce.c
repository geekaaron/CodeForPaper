#include "include/nonce.h"
#include <string.h>

uint8_t nonces[NONCE_COUNT][NONCE_SIZE];
size_t nonces_count = 0;

int is_nonce_used(const uint8_t nonces[][NONCE_SIZE], size_t nonces_count, const uint8_t nonce[NONCE_SIZE]) {
    for (size_t i = 0; i < nonces_count; i++) {
        if (!memcmp(nonces[i], nonce, NONCE_SIZE)) return 1;
    }
    return 0;
}
