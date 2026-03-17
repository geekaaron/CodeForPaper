import json
import numpy as np
import sys
import matplotlib.pyplot as plt
from tqdm import tqdm

plt.rc('font', family='Times New Roman')

def get_demand(price, rep, lam1, lam2, sens):
    return np.maximum(lam1 / (price + sens * (1 - rep)) - 1.0 / lam2, 0.0)

# ===== 请求者：均匀分配策略 =====
def requester_response(prices, reputations, lam1s, lam2s, sens):
    P = len(prices)
    demands = np.zeros(P)

    for i in range(P):
        d = get_demand(prices[i], reputations[i], lam1s, lam2s, sens)
        demands[i] = np.sum(d) / P

    return demands

# ===== 在指定 price 下计算一次 Stackelberg 收益 =====
def payoff_under_price(price, pinfo, rinfo):
    prices = np.ones(len(pinfo["prices"])) * price
    amounts = np.array(pinfo["amounts"], dtype=float)
    reputations = np.array(pinfo["reputations"], dtype=float)
    costs = np.array(pinfo["costs"], dtype=float)

    lam1s = np.array(rinfo["lambda1s"], dtype=float)
    lam2s = np.array(rinfo["lambda2s"], dtype=float)
    sens  = np.array(rinfo["sensitivities"], dtype=float)

    P = len(prices)
    R = len(lam1s)

    demands = requester_response(prices, reputations, lam1s, lam2s, sens)

    prov_profit = np.zeros(P)
    req_profit = np.zeros(R)

    for i in range(P):
        sold = min(demands[i], amounts[i])
        prov_profit[i] = (prices[i] - costs[i]) * sold

        d = get_demand(prices[i], reputations[i], lam1s, lam2s, sens) / P
        temp_profit = lam1s * np.log(1 + lam2s * d) - prices[i] * d - sens * (1 - reputations[i]) * d
        if demands[i] > amounts[i]:
            temp_profit *= amounts[i] / demands[i]
        req_profit += temp_profit

    return np.mean(prov_profit), np.mean(req_profit)

# =================== MAIN ===================
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python sg_price_scan.py <json_file>")
        sys.exit(0)

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    price_grid = np.arange(1, 100.1, 0.1)

    prov_payoffs = []
    req_payoffs  = []

    for p in tqdm(price_grid, desc="Scanning price", ncols=80):
        prov_p, req_p = payoff_under_price(p, data["providers"], data["requesters"])
        prov_payoffs.append(prov_p)
        req_payoffs.append(req_p)

    prov_payoffs = np.array(prov_payoffs)
    req_payoffs  = np.array(req_payoffs)

    # ===== 找最大收益对应的价格 =====
    best_p_prov = price_grid[np.argmax(prov_payoffs)]
    best_profit_prov = np.max(prov_payoffs)

    best_p_req = price_grid[np.argmax(req_payoffs)]
    best_profit_req = np.max(req_payoffs)

    print("\n========== Best Price ==========")
    print(f"Provider best price = {best_p_prov}, max avg payoff = {best_profit_prov:.3f}")
    print(f"Requester best price = {best_p_req}, max avg payoff = {best_profit_req:.3f}")

    # ===== Plot =====
    plt.figure(figsize=(6,4))
    plt.plot(price_grid, prov_payoffs, label="Provider Avg Payoff")
    plt.plot(price_grid, req_payoffs, label="Requester Avg Payoff")
    plt.xlabel("Price")
    plt.ylabel("Average Payoff")
    plt.title("Average Payoff vs Price (SG)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()
