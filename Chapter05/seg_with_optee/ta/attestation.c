#include <attestation.h>
#include <pta_attestation.h>

#include <nonce.h>
#include <global.h>
#include <seg_with_optee_ta.h>

TEE_Result get_ra_pubkey(uint32_t param_types, TEE_Param params[4])
{
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                               TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                               TEE_PARAM_TYPE_VALUE_OUTPUT,
                                               TEE_PARAM_TYPE_NONE);

    TEE_UUID pta_uuid = PTA_ATTESTATION_UUID;
    TEE_Session pta_sess;
    TEE_Result res;
    uint32_t err_origin;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    res = TEE_OpenTASession(&pta_uuid, 0, 0, NULL, &pta_sess, &err_origin);
    if (res != TEE_SUCCESS) return res;

    // get public key
    DMSG("Invoking TA to get public key.");
    res = TEE_InvokeTACommand(pta_sess, 0,
                              PTA_ATTESTATION_GET_PUBKEY,
                              param_types, params, &err_origin);

    TEE_CloseTASession(pta_sess);

    return res;
}

TEE_Result remote_attestation(uint32_t param_types, TEE_Param params[4])
{
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                               TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                               TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                               TEE_PARAM_TYPE_MEMREF_OUTPUT);
    
    TEE_UUID pta_uuid = PTA_ATTESTATION_UUID;
    TEE_Session pta_sess;
    TEE_Param op[4];
    uint32_t op_param_types;

    TEE_Result res;
    uint32_t err_origin;

    uint8_t ta_uuid[] = {
        0x00, 0xf2, 0xaa, 0xfa,
        0x50, 0x24,
        0xe4, 0x11,
        0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b
    };

    uint8_t nonce[NONCE_SIZE];

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    res = TEE_OpenTASession(&pta_uuid, 0, 0, NULL, &pta_sess, &err_origin);
    if (res != TEE_SUCCESS) return res;

    do {
        TEE_GenerateRandom(nonce, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, nonce));

    TEE_MemMove(nonces[nonces_count++], nonce, NONCE_SIZE);
    TEE_MemMove(params[3].memref.buffer, nonce, NONCE_SIZE);
    params[3].memref.size = NONCE_SIZE;

    // get ta shdr digest & signature
    TEE_MemFill(&op, 0, sizeof(op));
    op_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                     TEE_PARAM_TYPE_MEMREF_INPUT,
                                     TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                     TEE_PARAM_TYPE_NONE);

    op[0].memref.buffer = ta_uuid;
    op[0].memref.size = sizeof(ta_uuid);
    op[1].memref.buffer = nonce;
    op[1].memref.size = sizeof(nonce);
    op[2].memref.buffer = params[0].memref.buffer;
    op[2].memref.size = params[0].memref.size;

    DMSG("Invoking PTA to get ta shdr digest.");
    res = TEE_InvokeTACommand(pta_sess, 0,
                              PTA_ATTESTATION_GET_TA_SHDR_DIGEST,
                              op_param_types, op, &err_origin);
    if (res != TEE_SUCCESS) return res;

    params[0].memref.size = op[2].memref.size;

    // get ta memory digest & signature
    TEE_MemFill(&op, 0, sizeof(op));
    op_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                     TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                     TEE_PARAM_TYPE_NONE,
                                     TEE_PARAM_TYPE_NONE);

    op[0].memref.buffer = nonce;
    op[0].memref.size = sizeof(nonce);
    op[1].memref.buffer = params[1].memref.buffer;
    op[1].memref.size = params[1].memref.size;

    DMSG("Invoking PTA to get ta memory digest.");
    res = TEE_InvokeTACommand(pta_sess, 0,
                              PTA_ATTESTATION_HASH_TA_MEMORY,
                              op_param_types, op, &err_origin);
    if (res != TEE_SUCCESS) return res;

    params[1].memref.size = op[1].memref.size;

    // get tee memory digest & signature
    TEE_MemFill(&op, 0, sizeof(op));
    op_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                     TEE_PARAM_TYPE_MEMREF_OUTPUT,
                                     TEE_PARAM_TYPE_NONE,
                                     TEE_PARAM_TYPE_NONE);

    op[0].memref.buffer = nonce;
    op[0].memref.size = sizeof(nonce);
    op[1].memref.buffer = params[2].memref.buffer;
    op[1].memref.size = params[2].memref.size;

    DMSG("Invoking PTA to get tee memory digest.");
    res = TEE_InvokeTACommand(pta_sess, 0,
                              PTA_ATTESTATION_HASH_TEE_MEMORY,
                              op_param_types, op, &err_origin);
    if (res != TEE_SUCCESS) return res;

    params[2].memref.size = op[1].memref.size;

    TEE_CloseTASession(pta_sess);

    return TEE_SUCCESS;
}