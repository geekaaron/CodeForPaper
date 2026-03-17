
#ifndef JSON_H
#define JSON_H

#define PROVIDER_PARAMS_NUM     4
#define REQUESTER_PARAMS_NUM    3

#include <cjson/cJSON.h>

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

cJSON* json_from_file(const char *filename);
int json_to_struct(cJSON *json, ProvidersInfo *pinfo, RequestersInfo *rinfo);

#endif
