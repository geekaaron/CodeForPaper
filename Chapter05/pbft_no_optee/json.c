#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "utils.h"
#include "json.h"

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

cJSON* json_from_url(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "CURL init failed!\n");
        return NULL;
    }

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "CURL request failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        curl_easy_cleanup(curl);
        return NULL;
    }

    cJSON *json = cJSON_Parse(chunk.memory);
    if (!json) {
        fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
        free(chunk.memory);
        curl_easy_cleanup(curl);
        return NULL;
    }

    free(chunk.memory);
    curl_easy_cleanup(curl);

    return json;
}

int json_to_params_struct(cJSON *json, ProvidersInfo *pinfo, RequestersInfo *rinfo) {
    if (!json || !pinfo || !rinfo) return -1;

    cJSON *providers = cJSON_GetObjectItemCaseSensitive(json, "providers");
    cJSON *requesters = cJSON_GetObjectItemCaseSensitive(json, "requesters");

    if (!cJSON_IsArray(providers) || !cJSON_IsArray(requesters)) {
        fprintf(stderr, "Invalid JSON structure: providers or requesters missing or not arrays.\n");
        return -1;
    }

    int psize = cJSON_GetArraySize(providers);
    pinfo->size = psize;
    pinfo->prices = malloc(psize * sizeof(double));
    pinfo->amounts = malloc(psize * sizeof(double));
    pinfo->reputations = malloc(psize * sizeof(double));
    pinfo->costs = malloc(psize * sizeof(double));
    if (!pinfo->prices || !pinfo->amounts || !pinfo->reputations || !pinfo->costs) {
        fprintf(stderr, "Memory allocation failed for providers arrays.\n");
        return -1;
    }

    for (int i = 0; i < psize; i++) {
        cJSON *item = cJSON_GetArrayItem(providers, i);
        cJSON *price = cJSON_GetObjectItem(item, "price");
        cJSON *amount = cJSON_GetObjectItem(item, "amount");
        cJSON *reputation = cJSON_GetObjectItem(item, "reputation");
        cJSON *cost = cJSON_GetObjectItem(item, "cost");

        if (!price || !amount || !reputation || !cost) {
            fprintf(stderr, "Missing fields in provider item %d\n", i);
            return -1;
        }

        pinfo->prices[i] = atof(price->valuestring);
        pinfo->amounts[i] = atof(amount->valuestring);
        pinfo->reputations[i] = atof(reputation->valuestring);
        pinfo->costs[i] = atof(cost->valuestring);
    }

    int rsize = cJSON_GetArraySize(requesters);
    rinfo->size = rsize;
    rinfo->lambda1s = malloc(rsize * sizeof(double));
    rinfo->lambda2s = malloc(rsize * sizeof(double));
    rinfo->sensitivities = malloc(rsize * sizeof(double));
    if (!rinfo->lambda1s || !rinfo->lambda2s || !rinfo->sensitivities) {
        fprintf(stderr, "Memory allocation failed for requesters arrays.\n");
        return -1;
    }

    for (int i = 0; i < rsize; i++) {
        cJSON *item = cJSON_GetArrayItem(requesters, i);
        cJSON *lambda1 = cJSON_GetObjectItem(item, "lambda1");
        cJSON *lambda2 = cJSON_GetObjectItem(item, "lambda2");
        cJSON *sensitivity = cJSON_GetObjectItem(item, "sensitivity");

        if (!lambda1 || !lambda2 || !sensitivity) {
            fprintf(stderr, "Missing fields in requester item %d\n", i);
            return -1;
        }

        rinfo->lambda1s[i] = atof(lambda1->valuestring);
        rinfo->lambda2s[i] = atof(lambda2->valuestring);
        rinfo->sensitivities[i] = atof(sensitivity->valuestring);
    }

    return 0;
}

void free_providers_info(ProvidersInfo *pinfo) {
    if (!pinfo) return;
    free(pinfo->prices);
    free(pinfo->amounts);
    free(pinfo->reputations);
    free(pinfo->costs);
    pinfo->size = 0;
}

void free_requesters_info(RequestersInfo *rinfo) {
    if (!rinfo) return;
    free(rinfo->lambda1s);
    free(rinfo->lambda2s);
    free(rinfo->sensitivities);
    rinfo->size = 0;
}
