import json
import numpy as np
import sys
import matplotlib.pyplot as plt

plt.rc('font', family='Times New Roman')

rho = 1e-4
epsilon = 1e-5

# ===== 前景理论参数 =====
alpha = 0.88
beta = 0.88
lambda_loss = 2.25
gamma = 0.61
sp = 0.5

def value_function(x):
    return x**alpha if x >= 0 else -lambda_loss * ((-x)**beta)

def weight_function(p):
    return (p**gamma) / ((p**gamma + (1 - p)**gamma)**(1/gamma))

def get_demand(price, rep, lam1, lam2, sens):
    return max(lam1/(price + sens * (1 - rep)) - 1/lam2, 0.0)

def get_payoff(price, rep, lam1, lam2, sens, demand):
    return lam1 * np.log(1 + lam2 * demand) - price * demand - sens * (1 - rep) * demand

# ===== PT 请求者：均匀分配 =====
def requester_response(prices, amounts, reputations, lam1s, lam2s, sens):
    P = len(prices)
    R = len(lam1s)

    each_demands = np.zeros((P, R))
    demands = np.zeros(P)
    payoffs = np.zeros(P)
    avg_payoff = 0.0

    xs = np.ones(P) / P   # 均匀分配

    # === PT 最优需求（只在这里用 PT）===
    for i in range(P):
        for r in range(R):
            base_q = get_demand(prices[i], reputations[i], lam1s[r], lam2s[r], sens[r])

            qmin = max(0.0, base_q - 10)
            qmax = base_q + 10
            qs = np.linspace(qmin, qmax, 10)

            best_val = -1e9
            best_q = base_q

            for q in qs:
                payoff = get_payoff(prices[i], reputations[i], lam1s[r], lam2s[r], sens[r], q)
                pv = weight_function(sp) * value_function(payoff)
                if pv > best_val:
                    best_val = pv
                    best_q = q

            each_demands[i, r] = best_q

        demands[i] = xs[i] * np.sum(each_demands[i])

    # === 请求者平均收益（完全复制你给的第一段逻辑）===
    for i in range(P):
        payoffs[i] = np.sum(get_payoff(prices[i], reputations[i], lam1s, lam2s, sens, each_demands[i]))

        if demands[i] > amounts[i]:
            payoffs[i] *= amounts[i] / demands[i]

        avg_payoff += xs[i] * payoffs[i]

    return demands, avg_payoff

# ===== Stackelberg 主博弈 =====
def stackelberg_game_trace(pinfo, rinfo):
    prices = np.array(pinfo["prices"], float)
    amounts = np.array(pinfo["amounts"], float)
    reputations = np.array(pinfo["reputations"], float)
    costs = np.array(pinfo["costs"], float)

    lam1s = np.array(rinfo["lambda1s"], float)
    lam2s = np.array(rinfo["lambda2s"], float)
    sens  = np.array(rinfo["sensitivities"], float)

    P = len(prices)
    R = len(lam1s)

    price_trace, demand_trace = [], []
    prov_trace, req_trace = [], []

    while True:
        demands, req_profit = requester_response(prices, amounts, reputations, lam1s, lam2s, sens)

        prov_profit = np.zeros(P)

        for i in range(P):
            sold = min(demands[i], amounts[i])
            prov_profit[i] = (prices[i]-costs[i])*sold

        price_trace.append(prices.copy())
        demand_trace.append(demands.copy())
        prov_trace.append(np.mean(prov_profit))
        req_trace.append(req_profit)

        old_prices = prices.copy()
        for i in range(P):
            prices[i] += rho * (demands[i]-amounts[i])
            prices[i] = max(prices[i], costs[i])

        if np.all(np.abs(prices - old_prices)<epsilon):
            break

    return np.array(price_trace), np.array(demand_trace), \
           np.array(prov_trace), np.array(req_trace)

# ===== MAIN =====
if __name__ == "__main__":
    with open(sys.argv[1],"r",encoding="utf-8") as f:
        data = json.load(f)

    price_hist, demand_hist, prov_avg, req_avg = stackelberg_game_trace(
        data["providers"], data["requesters"])

    print("Final prices:", np.round(price_hist[-1], 2))

    print("Provider average payoff:", prov_avg[-1])
    print("Requester average payoff:", req_avg[-1])

    fig,axes = plt.subplots(1,3,figsize=(15,4))

    for i in range(price_hist.shape[1]):
        axes[0].plot(price_hist[:,i])
    axes[0].set_title("Price")

    for i in range(demand_hist.shape[1]):
        axes[1].plot(demand_hist[:,i])
    axes[1].set_title("Demand")

    axes[2].plot(prov_avg,label="Provider")
    axes[2].plot(req_avg,label="Requester")
    axes[2].legend()
    axes[2].set_title("Avg Payoff")

    plt.tight_layout()
    plt.show()
