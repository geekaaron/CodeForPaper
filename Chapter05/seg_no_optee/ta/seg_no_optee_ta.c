#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <seg_no_optee_ta.h>

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
}

TEE_Result TA_InvokeCommandEntryPoint(void *session,
                                      uint32_t cmd,
                                      uint32_t param_types,
                                      TEE_Param params[4])
{
    (void)&session;

    switch (cmd) {
    default:
        EMSG("Command ID 0x%x is not supported", cmd);
        return TEE_ERROR_NOT_SUPPORTED;
    }
}
