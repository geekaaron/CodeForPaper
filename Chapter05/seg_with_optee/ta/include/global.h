#ifndef GLOBAL_H
#define GLOBAL_H

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <nonce.h>
#include <crypto.h>
#include <game.h>

extern uint8_t params_hash[];
extern uint8_t nonces[][NONCE_SIZE];
extern uint32_t nonces_count;
extern TEE_ObjectHandle key_handle;
extern TEE_ObjectHandle peer_key_handle ;
extern TEE_OperationHandle sign_op_handle;
extern TEE_OperationHandle peer_verify_op_handle;
extern ProvidersInfo pinfo;
extern RequestersInfo rinfo;

#endif