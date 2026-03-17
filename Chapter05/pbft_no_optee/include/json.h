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

cJSON* json_from_url(const char *url);
int json_to_params_struct(cJSON *json, ProvidersInfo *pinfo, RequestersInfo *rinfo);

void free_providers_info(ProvidersInfo *pinfo);
void free_requesters_info(RequestersInfo *rinfo);

#endif

