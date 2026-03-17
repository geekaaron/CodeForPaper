#include "nonce.h"

int is_nonce_used(uint8_t nonces[][NONCE_SIZE], size_t nonces_count, uint8_t nonce[NONCE_SIZE]) {
    for (size_t i = 1; i < nonces_count; i++)
        if (!memcmp(nonces[i], nonce, NONCE_SIZE)) return 1;
    return 0;
}