import numpy as np

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

def get_payoff(price, rep, lam1, lam2, sens, demand):
    return lam1 * np.log(1 + lam2 * demand) - price * demand - sens * (1 - rep) * demand

# ===== PT 请求者：均匀分配 =====
def requester_response_pt(prices, amounts, reputations, lam1s, lam2s, sens):
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

    for i in range(P):
        payoffs[i] = np.sum(get_payoff(prices[i], reputations[i], lam1s, lam2s, sens, each_demands[i]))

        if demands[i] > amounts[i]:
            payoffs[i] *= amounts[i] / demands[i]

        avg_payoff += xs[i] * payoffs[i]

    return demands, avg_payoff

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

    demands, req_profit = requester_response_pt(prices, amounts, reputations, lam1s, lam2s, sens)

    prov_profit = 0.0
    for i in range(P):
        sold = min(demands[i], amounts[i])
        prov_profit += (prices[i]-costs[i])*sold

    return prov_profit, req_profit
