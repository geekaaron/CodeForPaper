#ifndef GLOBAL_H
#define GLOBAL_H

#include "json.h"
#include "nonce.h"
#include "crypto.h"
#include "message.h"

extern uint8_t ra_nonces[][NONCE_SIZE];
extern size_t ra_nonces_count;
extern uint8_t msg_nonces[][NONCE_SIZE];
extern size_t msg_nonces_count;

extern int node_id;
extern int reputation;
extern Node nodes[MAX_NODES];

extern EVP_PKEY *tee_key;
extern EC_KEY *local_privkey;

extern uint8_t *buffer;
extern uint8_t params_hash[HASH_SIZE];

extern uint8_t local_pubkey_x[EC_PUBKEY_SIZE];
extern uint8_t local_pubkey_y[EC_PUBKEY_SIZE];

#endif