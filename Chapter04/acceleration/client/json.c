#include <stdio.h>
#include <stdlib.h>

#include "json.h"

cJSON* json_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Open file error!");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = malloc(length + 1);
    long ret = fread(data, 1, length, file);
    if (ret != length) {
        fprintf(stderr, "fread error: expected %zu, got %zu\n", length, ret);
        fclose(file);
        return NULL;
    }
    data[length] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(data);
    if (!json) {
        fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
        free(data);
        return NULL;
    }

    return json;
}

int json_to_struct(cJSON *json, ProvidersInfo *pinfo, RequestersInfo *rinfo) {
    cJSON *providers = cJSON_GetObjectItemCaseSensitive(json, "providers");
    cJSON *requesters = cJSON_GetObjectItemCaseSensitive(json, "requesters");

    if (providers) {
        cJSON *prices = cJSON_GetObjectItemCaseSensitive(providers, "prices");
        cJSON *amounts = cJSON_GetObjectItemCaseSensitive(providers, "amounts");
        cJSON *reputations = cJSON_GetObjectItemCaseSensitive(providers, "reputations");
        cJSON *costs = cJSON_GetObjectItemCaseSensitive(providers, "costs");

    if (!prices || !amounts || !reputations || !costs) {
            fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
            return -1;
        }

        pinfo->size = cJSON_GetArraySize(prices);
        pinfo->prices = malloc(pinfo->size * sizeof(double));
        pinfo->amounts = malloc(pinfo->size * sizeof(double));
        pinfo->reputations = malloc(pinfo->size * sizeof(double));
        pinfo->costs = malloc(pinfo->size * sizeof(double));

        for (int i = 0; i < pinfo->size; i++) {
            pinfo->prices[i] = cJSON_GetArrayItem(prices, i)->valuedouble;
            pinfo->amounts[i] = cJSON_GetArrayItem(amounts, i)->valuedouble;
            pinfo->reputations[i] = cJSON_GetArrayItem(reputations, i)->valuedouble;
            pinfo->costs[i] = cJSON_GetArrayItem(costs, i)->valuedouble;
        }
    } else {
        fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    if (requesters) {
        cJSON *lambda1s = cJSON_GetObjectItemCaseSensitive(requesters, "lambda1s");
        cJSON *lambda2s = cJSON_GetObjectItemCaseSensitive(requesters, "lambda2s");
        cJSON *sensitivities = cJSON_GetObjectItemCaseSensitive(requesters, "sensitivities");

    if (!lambda1s || !lambda2s || !sensitivities) {
            fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
            return -1;
        }

        rinfo->size = cJSON_GetArraySize(lambda1s);
        rinfo->lambda1s = malloc(rinfo->size * sizeof(double));
        rinfo->lambda2s = malloc(rinfo->size * sizeof(double));
        rinfo->sensitivities = malloc(rinfo->size * sizeof(double));

        for (int i = 0; i < rinfo->size; i++) {
            rinfo->lambda1s[i] = cJSON_GetArrayItem(lambda1s, i)->valuedouble;
            rinfo->lambda2s[i] = cJSON_GetArrayItem(lambda2s, i)->valuedouble;
            rinfo->sensitivities[i] = cJSON_GetArrayItem(sensitivities, i)->valuedouble;
        }
    } else {
        fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON_Delete(json);
    return 0;
}
