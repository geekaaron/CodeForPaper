
#include <queue.h>
#include <poly.h>
#include <mymath.h>
#include <seg_curvefit_ta.h>
#include <game.h>

#define QUEUE_CAPACITY  9

ProvidersInfo pinfo = {NULL, NULL, NULL, NULL, 0};
RequestersInfo rinfo = {NULL, NULL, NULL, 0};

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

TEE_Result noncooperative_game(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE);
    TEE_Result res;

    double *xs = TEE_Malloc(pinfo.size * sizeof(double), 0);
    Queue **qxs = TEE_Malloc(pinfo.size * sizeof(Queue*), 0);
    double **coef = TEE_Malloc(pinfo.size * sizeof(double*), 0);
    double *demands = TEE_Malloc(pinfo.size * sizeof(double), 0);

    double **each_demands = TEE_Malloc(pinfo.size * sizeof(double*), 0);
    double *payoffs = TEE_Malloc(pinfo.size * sizeof(double), 0);

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

    // return prices
    TEE_MemMove(params[0].memref.buffer, pinfo.prices, params[0].memref.size);
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

    return res;
}

TEE_Result load_game_params(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_NONE);
    int current = 0;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    pinfo.size = params[0].value.a;
    pinfo.prices = TEE_Malloc(pinfo.size * sizeof(double), 0);
    pinfo.amounts = TEE_Malloc(pinfo.size * sizeof(double), 0);
    pinfo.reputations = TEE_Malloc(pinfo.size * sizeof(double), 0);
    pinfo.costs = TEE_Malloc(pinfo.size * sizeof(double), 0);

    rinfo.size = params[1].value.a;
    rinfo.lambda1s = TEE_Malloc(rinfo.size * sizeof(double), 0);
    rinfo.lambda2s = TEE_Malloc(rinfo.size * sizeof(double), 0);
    rinfo.sensitivities = TEE_Malloc(rinfo.size * sizeof(double), 0);

    TEE_MemMove(pinfo.prices, params[2].memref.buffer + current,
                sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    TEE_MemMove(pinfo.amounts, params[2].memref.buffer + current,
                sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    TEE_MemMove(pinfo.reputations, params[2].memref.buffer + current,
                sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    TEE_MemMove(pinfo.costs, params[2].memref.buffer + current,
                sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);

    TEE_MemMove(rinfo.lambda1s, params[2].memref.buffer + current,
                sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    TEE_MemMove(rinfo.lambda2s, params[2].memref.buffer + current,
                sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    TEE_MemMove(rinfo.sensitivities, params[2].memref.buffer + current,
                sizeof(double) * rinfo.size);

    return TEE_SUCCESS;
}
