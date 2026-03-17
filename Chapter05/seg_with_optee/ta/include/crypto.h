#ifndef ECDSA_H
#define ECDSA_H

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

TEE_Result ecdsa_keygen(uint32_t param_types, TEE_Param params[4]);
TEE_Result build_key_handle(uint32_t param_types, TEE_Param params[4]);
TEE_Result verify_signature(uint32_t param_types, TEE_Param params[4]);
TEE_Result ecdh_derive_key(uint32_t param_types, TEE_Param params[4]);
TEE_Result aes_decrypt(uint32_t param_types, TEE_Param params[4]);

#endif