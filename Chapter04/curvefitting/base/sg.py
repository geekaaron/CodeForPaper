import json
import numpy as np
import sys
import matplotlib.pyplot as plt

plt.rc('font', family='Times New Roman')

rho = 1e-4
epsilon = 1e-5

def get_demand(price, rep, lam1, lam2, sens):
    return np.maximum(lam1 / (price + sens * (1 - rep)) - 1.0 / lam2, 0.0)

def get_payoff(price, rep, lam1, lam2, sens, demand):
    return lam1 * np.log(1 + lam2 * demand) - price * demand - sens * (1 - rep) * demand

# ===== 请求者：均匀分配 =====
def requester_response(prices, amounts, reputations, lam1s, lam2s, sens):
    P = len(prices)
    R = len(lam1s)

    each_demands = np.zeros((P, R))
    demands = np.zeros(P)
    payoffs = np.zeros(P)
    avg_payoff = 0

    xs = np.ones(P) / P

    for i in range(P):
        each_demands[i] = get_demand(prices[i], reputations[i], lam1s, lam2s, sens)
        demands[i] = xs[i] * np.sum(each_demands[i])

    for i in range(P):
        payoffs[i] = np.sum(get_payoff(prices[i], reputations[i],
                                        lam1s, lam2s, sens, each_demands[i]))
        if demands[i] > amounts[i]:
            payoffs[i] *= amounts[i] / demands[i]
        avg_payoff += xs[i] * payoffs[i]

    return demands, avg_payoff

# ===== SG 反向归纳动力学 =====
def stackelberg_game_trace(pinfo, rinfo):
    prices = np.array(pinfo["prices"], dtype=float)
    amounts = np.array(pinfo["amounts"], dtype=float)
    reputations = np.array(pinfo["reputations"], dtype=float)
    costs = np.array(pinfo["costs"], dtype=float)

    lam1s = np.array(rinfo["lambda1s"], dtype=float)
    lam2s = np.array(rinfo["lambda2s"], dtype=float)
    sens  = np.array(rinfo["sensitivities"], dtype=float)

    P = len(prices)

    price_trace, demand_trace = [], []
    prov_payoff_trace, req_payoff_trace = [], []

    while True:
        demands, req_profit = requester_response(prices, amounts, reputations, lam1s, lam2s, sens)

        prov_profit = np.zeros(P)
        for i in range(P):
            sold = min(demands[i], amounts[i])
            prov_profit[i] = (prices[i] - costs[i]) * sold

        price_trace.append(prices.copy())
        demand_trace.append(demands.copy())
        prov_payoff_trace.append(np.mean(prov_profit))
        req_payoff_trace.append(req_profit)

        old_prices = prices.copy()

        for i in range(P):
            prices[i] += rho * (demands[i] - amounts[i])
            prices[i] = max(prices[i], costs[i])

        if np.all(np.abs(prices - old_prices) < epsilon):
            break

    return np.array(price_trace), np.array(demand_trace), \
           np.array(prov_payoff_trace), np.array(req_payoff_trace)

# ================= MAIN ===================
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python sg_trace.py <json>")
        sys.exit(0)

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    price_hist, demand_hist, prov_payoff, req_payoff = stackelberg_game_trace(
        data["providers"], data["requesters"])

    print("Final prices:", np.round(price_hist[-1], 2))

    print("Provider average payoff:", prov_payoff[-1])
    print("Requester average payoff:", req_payoff[-1])

    # ============ Plot ============
    fig, axes = plt.subplots(1, 3, figsize=(15,4))

    # (a) price
    for i in range(price_hist.shape[1]):
        axes[0].plot(price_hist[:, i], label=f"DP{i}")
    axes[0].set_title("(a) Price Dynamics")
    axes[0].set_xlabel("Iteration")
    axes[0].set_ylabel("Price")
    axes[0].legend()

    # (b) demand
    for i in range(demand_hist.shape[1]):
        axes[1].plot(demand_hist[:, i], label=f"DP{i}")
    axes[1].set_title("(b) Demand Dynamics")
    axes[1].set_xlabel("Iteration")
    axes[1].set_ylabel("Demand")
    axes[1].legend()

    # (c) avg payoff
    axes[2].plot(prov_payoff, label="Provider Avg Payoff")
    axes[2].plot(req_payoff, label="Requester Avg Payoff")
    axes[2].set_title("(c) Average Payoff")
    axes[2].set_xlabel("Iteration")
    axes[2].set_ylabel("Payoff")
    axes[2].legend()

    plt.tight_layout()
    plt.show()
