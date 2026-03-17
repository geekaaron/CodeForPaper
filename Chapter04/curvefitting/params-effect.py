import json
import numpy as np
import sys
import time
import matplotlib.pyplot as plt
from collections import deque
from tqdm import tqdm

plt.rcParams['font.family']=' Times New Roman, SimSun'

QUEUE_CAPACITY = 9

theta = 5e-6
rho = 1e-4
epsilon = 1e-2
pepsilon = 1e-5


def fast_log(x):
    return np.log(x)


def get_demand(price, rep, lam1, lam2, sens):
    return np.maximum(lam1 / (price + sens * (1.0 - rep)) - 1.0 / lam2, 0.0)


def get_payoff(price, rep, lam1, lam2, sens, demand):
    return lam1 * np.log(1.0 + lam2 * demand) \
           - price * demand \
           - sens * (1.0 - rep) * demand


def evolutionary_game(xs, qxs, prices, amounts, reputations, lam1s, lam2s, sens):
    global inner_count

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

    return demands



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

    while True:
        demands = evolutionary_game(xs, qxs, prices, amounts, reputations, lam1s, lam2s, sens)

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

    return np.array(price_trace), np.array(demand_trace)


if __name__ == "__main__":

    prefixes = ["amounts", "repus", "costs",
                "lambda1s", "lambda2s", "sens"]
    fig_num = ["a", "b", "c", "d", "e", "f"]
    
    titiles = {
                "amounts": "数据量D", 
                "repus": "声誉R",
                "costs": "成本c",
                "lambda1s": "收益系数λ1",
                "lambda2s": "收益系数λ2",
                "sens": "声誉敏感度s"
            }

    # ===== 外层进度条：6 个参数 =====
    for idx, prefix in enumerate(tqdm(prefixes, desc="Running parameter groups")):
        row = idx // 3
        col = idx % 3

        all_price_hist = []
        all_demand_hist = []
        legends = []

        # ===== 内层进度条：每个参数下 5 个实验 =====
        for k in tqdm(range(1, 6),
                      desc=f"{prefix} simulations",
                      leave=False):
            json_file = f"{prefix}-{k}.json"
            with open(json_file, "r", encoding="utf-8") as f:
                data = json.load(f)

            pinfo = data["providers"]
            rinfo = data["requesters"]

            global inner_count, outer_count
            inner_count = 0
            outer_count = 0

            price_hist, demand_hist = noncooperative_game(pinfo, rinfo)

            all_price_hist.append(price_hist)
            all_demand_hist.append(demand_hist)

            # 图例自动标注
            if prefix == "amounts":
                legends.append(f"D={int(pinfo['amounts'][0])}")
            elif prefix == "repus":
                legends.append(f"R={pinfo['reputations'][0]:.1f}")
            elif prefix == "costs":
                legends.append(f"c={pinfo['costs'][0]:.1f}")
            elif prefix == "lambda1s":
                legends.append(f"λ1={int(rinfo['lambda1s'][0])}")
            elif prefix == "lambda2s":
                legends.append(f"λ2={int(rinfo['lambda2s'][0])}")
            elif prefix == "sens":
                legends.append(f"s={rinfo['sensitivities'][0]:.1f}")

        # ===== 价格曲线 =====
        plt.figure(figsize=(6, 4))
        for i, hist in enumerate(all_price_hist):
            plt.plot(hist[:, 0], label=legends[i])
        plt.xlabel("迭代次数", fontsize=18)
        plt.ylabel("价格", fontsize=18)
        plt.tick_params(labelsize=14)
        plt.legend(fontsize=16)
        plt.tight_layout()
        plt.show()

        # ===== 需求曲线 =====
        plt.figure(figsize=(6, 4))
        for i, hist in enumerate(all_demand_hist):
            plt.plot(hist[:, 0], label=legends[i])
        plt.xlabel("迭代次数", fontsize=18)
        plt.ylabel("需求量", fontsize=18)
        plt.tick_params(labelsize=14)
        plt.legend(fontsize=16)
        plt.tight_layout()
        plt.show()
