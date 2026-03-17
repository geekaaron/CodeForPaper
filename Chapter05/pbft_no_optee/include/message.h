#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#include "nonce.h"
#include "crypto.h"

#define MAX_NODES 39

typedef struct {
    int id;
    int reputation;
} Node;

typedef struct __attribute__((packed)) {
    char type[16];
    int from;
    int reputation;

    uint8_t params_hash[HASH_SIZE];
    uint8_t params_signature[SIGN_SIZE];
    uint8_t params_nonce[NONCE_SIZE];

    uint8_t pubkey_x[EC_PUBKEY_SIZE];
    uint8_t pubkey_y[EC_PUBKEY_SIZE];
} PBFTMessage;

void handle_msg(PBFTMessage *msg);

#endif