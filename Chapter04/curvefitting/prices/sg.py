import numpy as np

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

    demands, req_profit = requester_response(prices, amounts, reputations, lam1s, lam2s, sens)

    prov_profit = 0.0
    for i in range(P):
        sold = min(demands[i], amounts[i])
        prov_profit += (prices[i] - costs[i]) * sold

    return prov_profit, req_profit
