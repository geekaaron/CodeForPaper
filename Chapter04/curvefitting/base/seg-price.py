import json
import sys
import copy
import numpy as np
import matplotlib.pyplot as plt
from collections import deque
from tqdm import tqdm

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


# ================== 演化博弈（原封不动） ==================
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

    return demands


# ================== 固定价格收益计算 ==================
def fixed_price_game(pinfo, rinfo):
    prices = np.array(pinfo["prices"], float)
    amounts = np.array(pinfo["amounts"], float)
    reputations = np.array(pinfo["reputations"], float)
    costs = np.array(pinfo["costs"], float)

    lam1s = np.array(rinfo["lambda1s"], float)
    lam2s = np.array(rinfo["lambda2s"], float)
    sens = np.array(rinfo["sensitivities"], float)

    P = len(prices)

    xs = np.ones(P) / P
    qxs = [deque(maxlen=QUEUE_CAPACITY) for _ in range(P)]

    demands = evolutionary_game(xs, qxs, prices, amounts,
                                reputations, lam1s, lam2s, sens)

    prov_profit = np.zeros(P)
    req_profit = np.zeros(len(lam1s))

    for i in range(P):
        sold = min(demands[i], amounts[i])
        prov_profit[i] = (prices[i] - costs[i]) * sold

        d = get_demand(prices[i], reputations[i], lam1s, lam2s, sens) / P
        temp_profit = lam1s * np.log(1 + lam2s * d) - prices[i] * d - sens * (1 - reputations[i]) * d
        if demands[i] > amounts[i]:
            temp_profit *= amounts[i] / demands[i]
        req_profit += temp_profit

    return np.mean(prov_profit), np.mean(req_profit)


# ================== 扫描价格 ==================
def scan_price(data, pmin=1, pmax=100):
    prov_avg_list, req_avg_list = [], []
    prices = np.arange(pmin, pmax+0.1, 0.1)

    for p in tqdm(prices, desc="Scanning price", ncols=80):
        data_tmp = copy.deepcopy(data)
        P = len(data_tmp["providers"]["prices"])
        data_tmp["providers"]["prices"] = [float(p)] * P

        prov_avg, req_avg = fixed_price_game(
            data_tmp["providers"], data_tmp["requesters"]
        )

        prov_avg_list.append(prov_avg)
        req_avg_list.append(req_avg)

    return prices, np.array(prov_avg_list), np.array(req_avg_list)


# ================== main ==================
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python fixed_price_payoff_scan.py data.json")
        sys.exit(0)

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    prices, prov_avg, req_avg = scan_price(data, 1, 100)

    # ===== 找最大收益对应的价格 =====
    best_p_prov = prices[np.argmax(prov_avg)]
    best_profit_prov = np.max(prov_avg)

    best_p_req = prices[np.argmax(req_avg)]
    best_profit_req = np.max(req_avg)

    print("\n========== Best Price ==========")
    print(f"Provider best price = {best_p_prov}, max avg payoff = {best_profit_prov:.3f}")
    print(f"Requester best price = {best_p_req}, max avg payoff = {best_profit_req:.3f}")

    plt.figure(figsize=(8, 5))
    plt.plot(prices, prov_avg, label="Provider Avg Payoff")
    plt.plot(prices, req_avg, label="Requester Avg Payoff")
    plt.xlabel("Price")
    plt.ylabel("Average Payoff")
    plt.title("Average Payoff under Fixed Price")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()
