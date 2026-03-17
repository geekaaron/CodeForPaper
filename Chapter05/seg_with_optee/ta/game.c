#include <queue.h>
#include <poly.h>
#include <mymath.h>
#include <seg_with_optee_ta.h>
#include <nonce.h>
#include <global.h>
#include <game.h>

#include <string.h>

#define QUEUE_CAPACITY  9

uint8_t params_hash[HASH_SIZE];
uint32_t params_hash_size = HASH_SIZE;

ProvidersInfo pinfo = {NULL, NULL, NULL, NULL, 0};
RequestersInfo rinfo = {NULL, NULL, NULL, 0};

double theta    = 5e-6;
double rho      = 1e-3;
double epsilon  = 1e-2;
double pepsilon = 1e-5;

static double get_demand(int pi, int ri)
{
    return rinfo.lambda1s[ri] / (pinfo.prices[pi] +
           rinfo.sensitivities[ri] * (1.0 - pinfo.reputations[pi])) - 1.0 / rinfo.lambda2s[ri];
}

static double get_payoff(int pi, int ri, double demand)
{
    return rinfo.lambda1s[ri] * mylog(1.0 + rinfo.lambda2s[ri] * demand) -
           pinfo.prices[pi] * demand - rinfo.sensitivities[ri] * (1.0 - pinfo.reputations[pi]) * demand;
}

static void evolutionary_game(double *xs, Queue **qxs, double *demands,
                              double **each_demands, double *payoffs)
{
    for (int i = 0; i < pinfo.size; i++) {
        demands[i] = 0.0;
        for (int j = 0; j < rinfo.size; j++) {
            each_demands[i][j] = get_demand(i, j);
            if (each_demands[i][j] < 0.0) each_demands[i][j] = 0.0;
            demands[i] += xs[i] * each_demands[i][j]; 
        }
    }

    while (1) {
        double average_payoff = 0.0;
        for (int i = 0; i < pinfo.size; i++) {
            payoffs[i] = 0.0;
            for (int j = 0; j < rinfo.size; j++) payoffs[i] += get_payoff(i, j, each_demands[i][j]);

            if (demands[i] > pinfo.amounts[i]) payoffs[i] *= pinfo.amounts[i] / demands[i];
            average_payoff += xs[i] * payoffs[i];
        }

        bool converged = false;
        for (int i = 0; i < pinfo.size; i++) {
            if (myfabs(payoffs[i] - average_payoff) < epsilon) {
                converged = true;
                break;
            }
        }

        if (converged) break;

        for (int i = 0; i < pinfo.size; i++) {
            xs[i] += theta * xs[i] * (payoffs[i] - average_payoff);
            enqueue(qxs[i], xs[i]);

            demands[i] = 0.0;
            for (int j = 0; j < rinfo.size; j++) demands[i] += xs[i] * each_demands[i][j];
        }
    }
}

TEE_Result noncooperative_game(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_NONE);

    TEE_OperationHandle digest_op_handle = TEE_HANDLE_NULL;
    TEE_Result res;

    double *xs = TEE_Malloc(pinfo.size * sizeof(double), 0);
    Queue **qxs = TEE_Malloc(pinfo.size * sizeof(Queue*), 0);
    double **coef = TEE_Malloc(pinfo.size * sizeof(double*), 0);
    double *demands = TEE_Malloc(pinfo.size * sizeof(double), 0);

    double **each_demands = TEE_Malloc(pinfo.size * sizeof(double*), 0);
    double *payoffs = TEE_Malloc(pinfo.size * sizeof(double), 0);
    double *tprices = TEE_Malloc(pinfo.size * sizeof(double), 0);

    if (param_types != exp_param_types) {
        res = TEE_ERROR_BAD_PARAMETERS;
        goto EXIT;
    }

    for (int i = 0; i < pinfo.size; i++) {
        xs[i] = 1.0 / pinfo.size;
        qxs[i] = create_queue(QUEUE_CAPACITY);
        coef[i] = TEE_Malloc(3 * sizeof(double), 0);
        each_demands[i] = TEE_Malloc(rinfo.size * sizeof(double), 0);
    }

    double indexs[QUEUE_CAPACITY-1];
    for (int i = 0; i < QUEUE_CAPACITY-1; i++) {
        indexs[i] = i + 1;
    }

    while (1) {
        evolutionary_game(xs, qxs, demands, each_demands, payoffs);
        TEE_MemMove(tprices, pinfo.prices, pinfo.size * sizeof(double));

        bool converged = true;
        for (int i = 0; i < pinfo.size; i++) {
            pinfo.prices[i] += rho * (demands[i] - pinfo.amounts[i]);
            if (pinfo.prices[i] < pinfo.costs[i]) pinfo.prices[i] = pinfo.costs[i];
            if (myfabs(demands[i] - pinfo.amounts[i]) > epsilon) converged = false;
        }
        if (converged) break;

        converged = true;
        for (int i = 0; i < pinfo.size; i++) {
            if (myfabs(pinfo.prices[i] - tprices[i]) > pepsilon) converged = false;
        }
        if (converged) break;

        if (!is_full_queue(qxs[0])) continue;

        for (int i = 0; i < pinfo.size; i++) {
            polyfit_quadratic(indexs, qxs[i], coef[i]);
            xs[i] = polyval_quadratic(coef[i], QUEUE_CAPACITY);
        }
    }

    // convert double type prices to string splited by ,
    size_t price_str_capacity = pinfo.size * PER_PRICE_STR_SIZE;
    char *price_str = TEE_Malloc(price_str_capacity, 0);

    char buf[PER_PRICE_STR_SIZE];
    size_t offset = 0;
    for (int i = 0; i < pinfo.size; ++i) {
        TEE_MemFill(buf, 0, sizeof(buf));

        int price_int = (int)(pinfo.prices[i] * 100);
        snprintf(buf, sizeof(buf), "%d.%02d", price_int / 100, price_int % 100);

        size_t len = strlen(buf);
        if (offset + len >= price_str_capacity)
            return TEE_ERROR_OVERFLOW;

        TEE_MemMove(price_str + offset, buf, len);
        offset += len;

        if (i != pinfo.size - 1) {
            if (offset + 1 >= price_str_capacity)
                return TEE_ERROR_OVERFLOW;
            price_str[offset++] = ',';
        }
    }

    // return prices
    uint8_t price_hash[HASH_SIZE];
    uint32_t price_hash_size = HASH_SIZE;

    params[0].memref.size = strlen(price_str);
    TEE_MemMove(params[0].memref.buffer, price_str, params[0].memref.size);
    TEE_Free(price_str);

    res = TEE_AllocateOperation(&digest_op_handle,
                                TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle,
                            params[0].memref.buffer, params[0].memref.size,
                            price_hash, &price_hash_size);
    if (res != TEE_SUCCESS) return res;
    TEE_FreeOperation(digest_op_handle);
    digest_op_handle = TEE_HANDLE_NULL;

    // sign
    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];
    uint32_t nonced_hash_size = HASH_SIZE;

    do {
        TEE_GenerateRandom(params[1].memref.buffer, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, params[1].memref.buffer));

    TEE_MemMove(nonces[nonces_count++], params[1].memref.buffer, NONCE_SIZE);
    TEE_MemMove(nonced_buf, params[1].memref.buffer, NONCE_SIZE);
    TEE_MemMove(nonced_buf + params[1].memref.size, price_hash, HASH_SIZE);
    params[1].memref.size = NONCE_SIZE;

    res = TEE_AllocateOperation(&digest_op_handle,
                                TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle,
                            nonced_buf, NONCE_SIZE + HASH_SIZE,
                            nonced_hash, &nonced_hash_size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_AsymmetricSignDigest(sign_op_handle, NULL, 0,
                                   nonced_hash, nonced_hash_size,
                                   params[2].memref.buffer,
                                   &params[2].memref.size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_SUCCESS;

EXIT:
    TEE_Free(xs);
    TEE_Free(demands);

    for (int i = 0; i < pinfo.size; i++) {
        TEE_Free(coef[i]);
        destroy_queue(qxs[i]);
    }

    TEE_Free(coef);
    TEE_Free(qxs);

    TEE_Free(payoffs);
    for (int i = 0; i < pinfo.size; i++) TEE_Free(each_demands[i]);
    TEE_Free(each_demands);
    TEE_Free(tprices);

    return res;
}

TEE_Result load_game_params(uint8_t *buffer, int psize, int rsize)
{
    TEE_OperationHandle digest_op_handle = TEE_HANDLE_NULL;
    TEE_Result res;
    int size = (psize * PROVIDER_PARAMS_NUM + rsize * REQUESTER_PARAMS_NUM) * sizeof(double);
    int current = 0;

    pinfo.size = psize;
    pinfo.prices = TEE_Malloc(pinfo.size * sizeof(double), 0);
    pinfo.amounts = TEE_Malloc(pinfo.size * sizeof(double), 0);
    pinfo.reputations = TEE_Malloc(pinfo.size * sizeof(double), 0);
    pinfo.costs = TEE_Malloc(pinfo.size * sizeof(double), 0);

    rinfo.size = rsize;
    rinfo.lambda1s = TEE_Malloc(rinfo.size * sizeof(double), 0);
    rinfo.lambda2s = TEE_Malloc(rinfo.size * sizeof(double), 0);
    rinfo.sensitivities = TEE_Malloc(rinfo.size * sizeof(double), 0);

    TEE_MemMove(pinfo.prices, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    TEE_MemMove(pinfo.amounts, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    TEE_MemMove(pinfo.reputations, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    TEE_MemMove(pinfo.costs, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);

    TEE_MemMove(rinfo.lambda1s, buffer + current, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    TEE_MemMove(rinfo.lambda2s, buffer + current, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    TEE_MemMove(rinfo.sensitivities, buffer + current, sizeof(double) * rinfo.size);

    res = TEE_AllocateOperation(&digest_op_handle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle, buffer, size, params_hash, &params_hash_size);

    return res;
}

TEE_Result get_game_params_info(uint32_t params_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_NONE);

    TEE_OperationHandle digest_op_handle = TEE_HANDLE_NULL;
    TEE_Result res;

    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];
    uint32_t nonced_hash_size = HASH_SIZE;

    if (params_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    TEE_MemMove(params[0].memref.buffer, params_hash, HASH_SIZE);
    params[0].memref.size = HASH_SIZE;

    do {
        TEE_GenerateRandom(params[1].memref.buffer, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, params[1].memref.buffer));

    TEE_MemMove(nonces[nonces_count++], params[1].memref.buffer, NONCE_SIZE);
    TEE_MemMove(nonced_buf, params[1].memref.buffer, NONCE_SIZE);
    TEE_MemMove(nonced_buf + params[1].memref.size, params_hash, HASH_SIZE);
    params[1].memref.size = NONCE_SIZE;

    res = TEE_AllocateOperation(&digest_op_handle,
                                TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle,
                            nonced_buf, NONCE_SIZE + HASH_SIZE,
                            nonced_hash, &nonced_hash_size);
    if (res != TEE_SUCCESS) return res;

    return TEE_AsymmetricSignDigest(sign_op_handle, NULL, 0,
                                    nonced_hash, nonced_hash_size,
                                    params[2].memref.buffer,
                                    &params[2].memref.size);
}
