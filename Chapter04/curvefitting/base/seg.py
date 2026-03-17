import json
import numpy as np
import sys
import matplotlib.pyplot as plt
from collections import deque

plt.rc('font', family='Times New Roman')

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


# ========== 非合作博弈：统一收益统计 ==========
def noncooperative_game(pinfo, rinfo):
    prices = np.array(pinfo["prices"], float)
    amounts = np.array(pinfo["amounts"], float)
    reputations = np.array(pinfo["reputations"], float)
    costs = np.array(pinfo["costs"], float)

    lam1s = np.array(rinfo["lambda1s"], float)
    lam2s = np.array(rinfo["lambda2s"], float)
    sens = np.array(rinfo["sensitivities"], float)

    P = len(prices)
    R = len(lam1s)

    xs = np.ones(P) / P
    qxs = [deque(maxlen=QUEUE_CAPACITY) for _ in range(P)]

    price_hist, demand_hist = [], []
    prov_avg_hist, req_avg_hist = [], []

    while True:
        demands, req_profit = evolutionary_game(xs, qxs, prices, amounts, reputations, lam1s, lam2s, sens)

        prov_profit = np.zeros(P)

        for i in range(P):
            sold = min(demands[i], amounts[i])
            prov_profit[i] = (prices[i] - costs[i]) * sold

        price_hist.append(prices.copy())
        demand_hist.append(demands.copy())
        prov_avg_hist.append(np.mean(prov_profit))
        req_avg_hist.append(req_profit)

        old_prices = prices.copy()
        prices += rho * (demands - amounts)
        prices = np.maximum(prices, costs)

        if np.all(np.abs(prices - old_prices) < pepsilon):
            break

    return np.array(price_hist), np.array(demand_hist), \
           np.array(prov_avg_hist), np.array(req_avg_hist)


# ================= MAIN =================
if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    price_hist, demand_hist, prov_avg, req_avg = noncooperative_game(
        data["providers"], data["requesters"])

    print("Final prices:", np.round(price_hist[-1], 2))

    print("Provider average payoff:", prov_avg[-1])
    print("Requester average payoff:", req_avg[-1])

    fig, axes = plt.subplots(1, 3, figsize=(15, 4))

    for i in range(price_hist.shape[1]):
        axes[0].plot(price_hist[:, i])
    axes[0].set_title("(a) Price")

    for i in range(demand_hist.shape[1]):
        axes[1].plot(demand_hist[:, i])
    axes[1].set_title("(b) Demand")

    axes[2].plot(prov_avg, label="Provider Avg")
    axes[2].plot(req_avg, label="Requester Avg")
    axes[2].legend()
    axes[2].set_title("(c) Avg Payoff")

    plt.tight_layout()
    plt.show()
