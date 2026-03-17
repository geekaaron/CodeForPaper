#ifndef NONCE_H
#define NONCE_H

#define NONCE_COUNT 4
#define NONCE_SIZE 16

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

int is_nonce_used(const uint8_t nonces[][NONCE_SIZE], size_t nonces_count, const uint8_t nonce[NONCE_SIZE]);

#endif