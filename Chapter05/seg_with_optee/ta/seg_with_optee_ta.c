#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <game.h>
#include <crypto.h>
#include <global.h>
#include <seg_with_optee_ta.h>
#include <attestation.h>

TEE_Result TA_CreateEntryPoint(void)
{
    /* Nothing to do */
    return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
    /* Nothing to do */
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
                                    TEE_Param __unused params[4],
                                    void __unused **session)
{
    (void)&params;
    (void)&session;
    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
    (void)&session;

    if (key_handle != TEE_HANDLE_NULL)
        TEE_FreeTransientObject(key_handle);
    if (peer_key_handle != TEE_HANDLE_NULL)
        TEE_FreeTransientObject(peer_key_handle);
    if (sign_op_handle != TEE_HANDLE_NULL)
        TEE_FreeOperation(sign_op_handle);
    if (peer_verify_op_handle != TEE_HANDLE_NULL)
        TEE_FreeOperation(peer_verify_op_handle);

    if (pinfo.prices != NULL) TEE_Free(pinfo.prices);
    if (pinfo.amounts != NULL) TEE_Free(pinfo.amounts);
    if (pinfo.reputations != NULL) TEE_Free(pinfo.reputations);
    if (pinfo.costs != NULL) TEE_Free(pinfo.costs);
    if (rinfo.lambda1s != NULL) TEE_Free(rinfo.lambda1s);
    if (rinfo.lambda2s != NULL) TEE_Free(rinfo.lambda2s);
    if (rinfo.sensitivities != NULL) TEE_Free(rinfo.sensitivities);
}

TEE_Result TA_InvokeCommandEntryPoint(void *session,
                                      uint32_t cmd,
                                      uint32_t param_types,
                                      TEE_Param params[4])
{
    (void)&session;

    switch (cmd) {
    case TA_SEG_WITH_OPTEE_CMD_GET_RA_PUBKEY:
        return get_ra_pubkey(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_ECDSA_KEYGEN:
        return ecdsa_keygen(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_ECDH_DERIVE_KEY:
        return ecdh_derive_key(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_AES_DECRYPT:
        return aes_decrypt(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_GET_PARAMS_INFO:
        return get_game_params_info(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_BUILD_KEY_HANDLE:
        return build_key_handle(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_VERIFY_SIGNATURE:
        return verify_signature(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_REMOTE_ATTESTATION:
        return remote_attestation(param_types, params);
    case TA_SEG_WITH_OPTEE_CMD_START_PRICING_GAME:
        return noncooperative_game(param_types, params);
    default:
        EMSG("Command ID 0x%x is not supported", cmd);
        return TEE_ERROR_NOT_SUPPORTED;
    }
}
