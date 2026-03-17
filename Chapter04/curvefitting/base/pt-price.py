import json
import numpy as np
import sys
import matplotlib.pyplot as plt
from tqdm import tqdm

plt.rc('font', family='Times New Roman')

# ===== 前景理论参数 =====
alpha = 0.88
beta = 0.88
lambda_loss = 2.25
gamma = 0.61
sp = 0.5

def value_function(x):
    return x**alpha if x >= 0 else -lambda_loss*((-x)**beta)

def weight_function(p):
    return (p**gamma) / ((p**gamma + (1-p)**gamma)**(1/gamma))

def get_demand(price, rep, lam1, lam2, sens):
    return max(lam1/(price + sens*(1-rep)) - 1/lam2, 0.0)

# ===== PT 请求者反应 =====
def requester_response_pt(prices, reputations, lam1s, lam2s, sens):
    P, R = len(prices), len(lam1s)
    demands = np.zeros(P)

    for i in range(P):
        for r in range(R):
            base_q = get_demand(prices[i], reputations[i], lam1s[r], lam2s[r], sens[r])

            qmin = max(0, base_q - 10)
            qmax = base_q + 10
            qs = np.linspace(qmin, qmax, 10)

            best_v = -1e9
            best_q = base_q
            for q in qs:
                u = lam1s[r]*np.log(1+lam2s[r]*q) - prices[i]*q - sens[r]*(1-reputations[i])*q
                pv = weight_function(sp)*value_function(u)
                if pv > best_v:
                    best_v, best_q = pv, q

            demands[i] += best_q / P

    return demands

# ===== 固定 price 的收益计算 =====
def payoff_under_price(price, pinfo, rinfo):
    prices = np.ones(len(pinfo["prices"])) * price
    amounts = np.array(pinfo["amounts"])
    reputations = np.array(pinfo["reputations"])
    costs = np.array(pinfo["costs"])

    lam1s = np.array(rinfo["lambda1s"])
    lam2s = np.array(rinfo["lambda2s"])
    sens  = np.array(rinfo["sensitivities"])

    P, R = len(prices), len(lam1s)

    demands = requester_response_pt(prices, reputations, lam1s, lam2s, sens)

    prov_profit = np.zeros(P)
    req_profit = np.zeros(R)

    for i in range(P):
        sold = min(demands[i], amounts[i])
        prov_profit[i] = (prices[i]-costs[i])*sold

        for r in range(R):
            d = get_demand(prices[i], reputations[i], lam1s[r], lam2s[r], sens[r]) / P
            u = lam1s[r]*np.log(1+lam2s[r]*d) - prices[i]*d - sens[r]*(1-reputations[i])*d
            if demands[i] > amounts[i]:
                u *= amounts[i]/demands[i]
            req_profit[r] += u

    return np.mean(prov_profit), np.mean(req_profit)

# ================= MAIN =================
if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    price_grid = np.arange(1, 100.1, 0.1)

    prov_curve, req_curve = [], []

    for p in tqdm(price_grid, desc="Scanning price", ncols=80):
        pr, rq = payoff_under_price(p, data["providers"], data["requesters"])
        prov_curve.append(pr)
        req_curve.append(rq)

    prov_curve = np.array(prov_curve)
    req_curve  = np.array(req_curve)

    best_p_prov = price_grid[np.argmax(prov_curve)]
    best_profit_prov = np.max(prov_curve)

    best_p_req = price_grid[np.argmax(req_curve)]
    best_profit_req = np.max(req_curve)

    print("\n========== Best Price ==========")
    print(f"Provider best price = {best_p_prov}, max avg payoff = {best_profit_prov:.3f}")
    print(f"Requester best price = {best_p_req}, max avg payoff = {best_profit_req:.3f}")

    plt.figure(figsize=(6,4))
    plt.plot(price_grid, prov_curve, label="Provider Avg Payoff")
    plt.plot(price_grid, req_curve, label="Requester Avg Payoff")
    plt.xlabel("Price")
    plt.ylabel("Average Payoff")
    plt.title("PT-Stackelberg Payoff vs Price")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()
