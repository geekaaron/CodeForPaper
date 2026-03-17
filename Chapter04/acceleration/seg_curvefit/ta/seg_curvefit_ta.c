#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <game.h>
#include <seg_curvefit_ta.h>

extern ProvidersInfo pinfo;
extern RequestersInfo rinfo;

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
    case TA_SEG_CURVEFIT_CMD_LOAD_GAME_PARAMS:
        return load_game_params(param_types, params);
    case TA_SEG_CURVEFIT_CMD_START_PRICING_GAME:
        return noncooperative_game(param_types, params);
    default:
        EMSG("Command ID 0x%x is not supported", cmd);
        return TEE_ERROR_NOT_SUPPORTED;
    }
}
