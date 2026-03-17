import json
import numpy as np
import sys
import time
import matplotlib.pyplot as plt
from collections import deque

plt.rcParams['font.family']=' Times New Roman, SimSun'

QUEUE_CAPACITY = 9

theta = 5e-6
rho = 1e-4
epsilon = 1e-2
pepsilon = 1e-5

inner_count = 0
outer_count = 0


def fast_log(x):
    return np.log(x)


def get_demand(price, rep, lam1, lam2, sens):
    return np.maximum(lam1 / (price + sens * (1.0 - rep)) - 1.0 / lam2, 0.0)


def get_payoff(price, rep, lam1, lam2, sens, demand):
    return lam1 * np.log(1.0 + lam2 * demand) \
           - price * demand \
           - sens * (1.0 - rep) * demand


def evolutionary_game(xs, qxs, prices, amounts, reputations, lam1s, lam2s, sens, record_xs=False):
    global inner_count

    xs_trace = []

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
            payoffs[i] = np.sum(get_payoff(prices[i], reputations[i], lam1s, lam2s, sens, each_demands[i]))
            if demands[i] > amounts[i]:
                payoffs[i] *= amounts[i] / demands[i]
            avg_payoff += xs[i] * payoffs[i]

        inner_count += 1

        if np.any(np.abs(payoffs - avg_payoff) < epsilon):
            break

        for i in range(P):
            xs[i] += theta * xs[i] * (payoffs[i] - avg_payoff)
            qxs[i].append(xs[i])
            demands[i] = xs[i] * np.sum(each_demands[i])

        if record_xs:
            xs_trace.append(xs.copy())

    if record_xs:
        return demands, np.array(xs_trace)
    else:
        return demands, None



def noncooperative_game(pinfo, rinfo):
    global outer_count

    prices = np.array(pinfo["prices"], dtype=np.float64)
    amounts = np.array(pinfo["amounts"], dtype=np.float64)
    reputations = np.array(pinfo["reputations"], dtype=np.float64)
    costs = np.array(pinfo["costs"], dtype=np.float64)

    lam1s = np.array(rinfo["lambda1s"], dtype=np.float64)
    lam2s = np.array(rinfo["lambda2s"], dtype=np.float64)
    sens  = np.array(rinfo["sensitivities"], dtype=np.float64)

    P = len(prices)

    xs = np.ones(P) / P
    qxs = [deque(maxlen=QUEUE_CAPACITY) for _ in range(P)]

    price_trace = []
    demand_trace = []

    first_xs_trace = None

    while True:
        if outer_count == 0:
            demands, first_xs_trace = evolutionary_game(
                xs, qxs, prices, amounts, reputations,
                lam1s, lam2s, sens, record_xs=True)
        else:
            demands, _ = evolutionary_game(
                xs, qxs, prices, amounts, reputations,
                lam1s, lam2s, sens, record_xs=False)

        price_trace.append(prices.copy())
        demand_trace.append(demands.copy())

        old_prices = prices.copy()
        outer_count += 1

        for i in range(P):
            prices[i] += rho * (demands[i] - amounts[i])
            prices[i] = max(prices[i], costs[i])

        if np.all(np.abs(demands - amounts) < epsilon):
            break
        if np.all(np.abs(prices - old_prices) < pepsilon):
            break

        if len(qxs[0]) < QUEUE_CAPACITY:
            continue

        for i in range(P):
            y = np.array(qxs[i])
            x = np.arange(1, QUEUE_CAPACITY + 1)
            coef = np.polyfit(x, y, 2)
            xs[i] = np.polyval(coef, QUEUE_CAPACITY + 1)

    pinfo["prices"][:] = prices.tolist()

    return np.array(price_trace), np.array(demand_trace), first_xs_trace


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <filename>")
        sys.exit(0)

    json_file = sys.argv[1]

    with open(json_file, "r", encoding="utf-8") as f:
        data = json.load(f)

    pinfo = data["providers"]
    rinfo = data["requesters"]

    start = time.time()
    price_hist, demand_hist, xs_trace = noncooperative_game(pinfo, rinfo)
    end = time.time()

    print(f"Time: {(end-start)*1000:.3f} ms")
    print("Optimal prices:")
    print(np.round(pinfo["prices"], 2))
    print("outer count:", outer_count, "inner count:", inner_count)

    # ========= 种群密度收敛过程 =========
    plt.figure(figsize=(6, 4))
    for i in range(xs_trace.shape[1]):
        plt.plot(xs_trace[:, i], label=f"选择 DP{i}")
    plt.xlabel("迭代次数", fontsize=18)
    plt.ylabel("种群密度", fontsize=18)
    plt.tick_params(labelsize=14)
    plt.legend(loc="center right", fontsize=16)
    plt.tight_layout()
    plt.show()

    # ===== 价格收敛过程 =====
    plt.figure(figsize=(6, 4))
    for i in range(price_hist.shape[1]):
        plt.plot(price_hist[:, i], label=f"DP{i}")
    plt.xlabel("迭代次数", fontsize=18)
    plt.ylabel("价格", fontsize=18)
    plt.tick_params(labelsize=14)
    plt.legend(fontsize=16)
    plt.tight_layout()
    plt.show()

    # ===== 需求量收敛过程 =====
    plt.figure(figsize=(6, 4))
    for i in range(demand_hist.shape[1]):
        plt.plot(demand_hist[:, i], label=f"DP{i}")
    plt.xlabel("迭代次数", fontsize=18)
    plt.ylabel("需求量", fontsize=18)
    plt.tick_params(labelsize=14)
    plt.legend(fontsize=16)
    plt.tight_layout()
    plt.show()
