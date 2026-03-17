#ifndef ATTESTATION_H
#define ATTESTATION_H

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

TEE_Result get_ra_pubkey(uint32_t param_types, TEE_Param params[4]);
TEE_Result remote_attestation(uint32_t param_types, TEE_Param params[4]);

#endif