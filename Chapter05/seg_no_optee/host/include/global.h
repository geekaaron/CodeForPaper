#ifndef GLOBAL_H
#define GLOBAL_H

#include "crypto.h"
#include "nonce.h"
#include "game.h"

extern EC_KEY *self_key;
extern EC_KEY *peer_key;

extern uint8_t params_hash[];
extern uint8_t nonces[][NONCE_SIZE];
extern uint32_t nonces_count;
extern ProvidersInfo pinfo;
extern RequestersInfo rinfo;

#endif