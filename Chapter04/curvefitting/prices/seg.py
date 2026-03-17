import numpy as np
from collections import deque

QUEUE_CAPACITY = 9
theta = 5e-6
rho = 1e-4
epsilon = 1e-2
pepsilon = 1e-5


def get_demand(price, rep, lam1, lam2, sens):
    return np.maximum(lam1 / (price + sens * (1 - rep)) - 1.0 / lam2, 0.0)


def get_payoff(price, rep, lam1, lam2, sens, demand):
    return lam1 * np.log(1 + lam2 * demand) - price * demand - sens * (1 - rep) * demand


# ========== 演化博弈：不做收益统计 ==========
def evolutionary_game(xs, qxs, prices, amounts, reputations, lam1s, lam2s, sens):
    P = len(prices)
    R = len(lam1s)

    each_demands = np.zeros((P, R))
    demands = np.zeros(P)

    for i in range(P):
        each_demands[i] = get_demand(prices[i], reputations[i], lam1s, lam2s, sens)
        demands[i] = xs[i] * np.sum(each_demands[i])

    while True:
        payoffs = np.zeros(P)
        avg_payoff = 0

        for i in range(P):
            payoffs[i] = np.sum(get_payoff(prices[i], reputations[i],
                                           lam1s, lam2s, sens, each_demands[i]))
            if demands[i] > amounts[i]:
                payoffs[i] *= amounts[i] / demands[i]
            avg_payoff += xs[i] * payoffs[i]

        if np.any(np.abs(payoffs - avg_payoff) < epsilon):
            break

        for i in range(P):
            xs[i] += theta * xs[i] * (payoffs[i] - avg_payoff)
            qxs[i].append(xs[i])
            demands[i] = xs[i] * np.sum(each_demands[i])

    return demands, avg_payoff


# ================== 固定价格收益计算 ==================
def payoff_under_price(price, pinfo, rinfo):
    prices = np.ones(len(pinfo["prices"])) * price
    amounts = np.array(pinfo["amounts"], dtype=float)
    reputations = np.array(pinfo["reputations"], dtype=float)
    costs = np.array(pinfo["costs"], dtype=float)

    lam1s = np.array(rinfo["lambda1s"], dtype=float)
    lam2s = np.array(rinfo["lambda2s"], dtype=float)
    sens  = np.array(rinfo["sensitivities"], dtype=float)

    P = len(prices)

    xs = np.ones(P) / P
    qxs = [deque(maxlen=QUEUE_CAPACITY) for _ in range(P)]

    demands, req_profit = evolutionary_game(xs, qxs, prices, amounts, reputations, lam1s, lam2s, sens)

    prov_profit = 0.0
    for i in range(P):
        sold = min(demands[i], amounts[i])
        prov_profit += (prices[i] - costs[i]) * sold

    return prov_profit, req_profit
