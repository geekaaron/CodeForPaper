#ifndef GAME_H
#define GAME_H

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

typedef struct {
    double *prices;
    double *amounts;
    double *reputations;
    double *costs;
    int size;
} ProvidersInfo;

typedef struct {
    double *lambda1s;
    double *lambda2s;
    double *sensitivities;
    int size;
} RequestersInfo;

TEE_Result noncooperative_game(uint32_t param_types, TEE_Param params[4]);
TEE_Result load_game_params(uint32_t param_types, TEE_Param params[4]);

#endif