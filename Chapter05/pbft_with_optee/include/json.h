#ifndef JSON_H
#define JSON_H

#include <cjson/cJSON.h>
#include "crypto.h"

#define PROVIDER_PARAMS_NUM     4
#define REQUESTER_PARAMS_NUM    3

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

typedef struct {
    uint8_t ra_public_key[MODULUS_SIZE];
    uint8_t ecdsa_public_key[EC_PUBKEY_SIZE*2];
    uint8_t ta_shdr_hash[HASH_SIZE];
    uint8_t ta_mem_hash[HASH_SIZE];
    uint8_t tee_mem_hash[HASH_SIZE];
} TeeInfo;

cJSON* json_from_url(const char *url);
int json_to_params_struct(cJSON *json, ProvidersInfo *pinfo, RequestersInfo *rinfo);
int json_to_tee_struct(cJSON *json, TeeInfo *tinfo);

void free_providers_info(ProvidersInfo *pinfo);
void free_requesters_info(RequestersInfo *rinfo);

#endif

