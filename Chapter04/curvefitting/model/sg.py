import numpy as np

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
    avg_payoff = 0.0

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

        prov_profit = 0.0
        for i in range(P):
            sold = min(demands[i], amounts[i])
            prov_profit += (prices[i] - costs[i]) * sold

        price_trace.append(prices.copy())
        demand_trace.append(demands.copy())
        prov_payoff_trace.append(prov_profit)
        req_payoff_trace.append(req_profit)

        old_prices = prices.copy()

        for i in range(P):
            prices[i] += rho * (demands[i] - amounts[i])
            prices[i] = max(prices[i], costs[i])

        if np.all(np.abs(prices - old_prices) < epsilon):
            break

    return np.array(price_trace), np.array(demand_trace), \
           np.array(prov_payoff_trace), np.array(req_payoff_trace)

def run_sg(data):
    return stackelberg_game_trace(data["providers"], data["requesters"])