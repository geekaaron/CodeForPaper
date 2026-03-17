#include "include/queue.h"
#include "include/poly.h"
#include "include/mymath.h"
#include "include/nonce.h"
#include "include/crypto.h"
#include "include/global.h"
#include "include/game.h"

#define PROVIDER_PARAMS_NUM     4
#define REQUESTER_PARAMS_NUM    3

#define QUEUE_CAPACITY  9

uint8_t params_hash[HASH_SIZE];
uint32_t params_hash_size = HASH_SIZE;

ProvidersInfo pinfo = {NULL, NULL, NULL, NULL};
RequestersInfo rinfo = {NULL, NULL, NULL};

double theta   = 1e-4;
double rho     = 1e-3;
double epsilon = 1e-2;

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

        bool converged = true;
        for (int i = 0; i < pinfo.size; i++) {
            if (myfabs(payoffs[i] - average_payoff) > epsilon) {
                converged = false;
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

void noncooperative_game(char *prices, uint8_t *nonce, uint8_t *signature)
{
    double *xs = malloc(pinfo.size * sizeof(double));
    Queue **qxs = malloc(pinfo.size * sizeof(Queue*));
    double **coef = malloc(pinfo.size * sizeof(double*));
    double *demands = malloc(pinfo.size * sizeof(double));

    double **each_demands = malloc(pinfo.size * sizeof(double*));
    double *payoffs = malloc(pinfo.size * sizeof(double));

    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];

    for (int i = 0; i < pinfo.size; i++) {
        xs[i] = 1.0 / pinfo.size;
        qxs[i] = create_queue(QUEUE_CAPACITY);
        coef[i] = malloc(3 * sizeof(double));
        each_demands[i] = malloc(rinfo.size * sizeof(double));
    }

    double indexs[QUEUE_CAPACITY-1];
    for (int i = 0; i < QUEUE_CAPACITY-1; i++) {
        indexs[i] = i + 1;
    }

    while (1) {
        evolutionary_game(xs, qxs, demands, each_demands, payoffs);

        bool converged = true;
        for (int i = 0; i < pinfo.size; i++) {
            pinfo.prices[i] += rho * (demands[i] - pinfo.amounts[i]);
            if (pinfo.prices[i] < pinfo.costs[i]) pinfo.prices[i] = pinfo.costs[i];
            if (myfabs(demands[i] - pinfo.amounts[i]) > epsilon) converged = false;
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

    char buf[PER_PRICE_STR_SIZE];
    size_t offset = 0;
    for (int i = 0; i < pinfo.size; ++i) {
        memset(buf, 0, sizeof(buf));

        int price_int = (int)(pinfo.prices[i] * 100);
        snprintf(buf, sizeof(buf), "%d.%02d", price_int / 100, price_int % 100);

        size_t len = strlen(buf);
        if (offset + len >= price_str_capacity)
            return;

        memcpy(prices + offset, buf, len);
        offset += len;

        if (i != pinfo.size - 1) {
            if (offset + 1 >= price_str_capacity)
                return;
            prices[offset++] = ',';
        }
    }

    uint8_t prices_hash[HASH_SIZE];
    SHA256(prices, strlen(prices), prices_hash);

    // sign
    do {
        RAND_bytes(nonce, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, nonce));

    memcpy(nonces[nonces_count++], nonce, NONCE_SIZE);
    memcpy(nonced_buf, nonce, NONCE_SIZE);
    memcpy(nonced_buf + NONCE_SIZE, prices_hash, HASH_SIZE);

    SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);
    sign_hash(self_key, nonced_hash, signature);

    free(xs);
    free(demands);

    for (int i = 0; i < pinfo.size; i++) {
        free(coef[i]);
        destroy_queue(qxs[i]);
    }

    free(coef);
    free(qxs);

    free(payoffs);
    for (int i = 0; i < pinfo.size; i++) free(each_demands[i]);
    free(each_demands);
}

void load_game_params(uint8_t *buffer, int psize, int rsize)
{
    int size = (psize * PROVIDER_PARAMS_NUM + rsize * REQUESTER_PARAMS_NUM) * sizeof(double);
    int current = 0;

    pinfo.size = psize;
    pinfo.prices = malloc(pinfo.size * sizeof(double));
    pinfo.amounts = malloc(pinfo.size * sizeof(double));
    pinfo.reputations = malloc(pinfo.size * sizeof(double));
    pinfo.costs = malloc(pinfo.size * sizeof(double));

    rinfo.size = rsize;
    rinfo.lambda1s = malloc(rinfo.size * sizeof(double));
    rinfo.lambda2s = malloc(rinfo.size * sizeof(double));
    rinfo.sensitivities = malloc(rinfo.size * sizeof(double));

    memcpy(pinfo.prices, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(pinfo.amounts, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(pinfo.reputations, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(pinfo.costs, buffer + current, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);

    memcpy(rinfo.lambda1s, buffer + current, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    memcpy(rinfo.lambda2s, buffer + current, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    memcpy(rinfo.sensitivities, buffer + current, sizeof(double) * rinfo.size);

    SHA256(buffer, size, params_hash);
}

void get_game_params_info(uint8_t *nonce, uint8_t *signature)
{
    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];

    do {
        RAND_bytes(nonce, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, nonce));

    memcpy(nonces[nonces_count++], nonce, NONCE_SIZE);
    memcpy(nonced_buf, nonce, NONCE_SIZE);
    memcpy(nonced_buf + NONCE_SIZE, params_hash, HASH_SIZE);

    SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);
    sign_hash(self_key, nonced_hash, signature);
}