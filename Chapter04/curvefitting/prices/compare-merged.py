import json
import sys
import numpy as np
import matplotlib.pyplot as plt
from tqdm import tqdm
import seg, sg, pt

plt.rcParams['font.family']=' Times New Roman, SimSun'

def scan_game(game_module, price_grid, pinfo, rinfo):
    prov_curve, req_curve = [], []
    for p in tqdm(price_grid, ncols=80):
        pr, rq = game_module.payoff_under_price(p, pinfo, rinfo)
        prov_curve.append(pr)
        req_curve.append(rq)
    return np.array(prov_curve), np.array(req_curve)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python compare.py <json_file>")
        sys.exit(0)

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    price_grid = np.arange(1, 100.1, 0.1)

    print("Running SEG...")
    seg_prov, seg_req = scan_game(seg, price_grid,
                                    data["providers"], data["requesters"])

    print("Running SG...")
    sg_prov, sg_req = scan_game(sg, price_grid,
                                data["providers"], data["requesters"])

    print("Running PT...")
    pt_prov, pt_req = scan_game(pt, price_grid,
                                data["providers"], data["requesters"])

    # ========== 输出最优 price ==========
    def report(name, curve):
        p = price_grid[np.argmax(curve)]
        v = np.max(curve)
        print(f"{name}: best price = {p:.2f}, max payoff = {v:.3f}")

    print("\n========= Provider Best Price =========")
    report("SEG", seg_prov)
    report("SG",    sg_prov)
    report("PT",    pt_prov)

    print("\n========= Requester Best Price =========")
    report("SEG", seg_req)
    report("SG",    sg_req)
    report("PT",    pt_req)

    # ========== 绘图 ==========
    fig, axs = plt.subplots(1, 3, figsize=(15, 4))

    # Provider
    axs[0].plot(price_grid, seg_prov, label="SEG")
    axs[0].plot(price_grid, sg_prov,   label="SG（文献[75, 76]）")
    axs[0].plot(price_grid, pt_prov,   label="SGPT（文献[56]）")
    axs[0].set_xlabel("价格", fontsize=18)
    axs[0].set_ylabel("收益", fontsize=18)
    axs[0].set_title("(a) 数据提供者群体的总收益", fontsize=22)
    axs[0].tick_params(labelsize=12)
    axs[0].legend(fontsize=14)

    # Requester
    axs[1].plot(price_grid, seg_req, label="SEG")
    axs[1].plot(price_grid, sg_req,   label="SG（文献[75, 76]）")
    axs[1].plot(price_grid, pt_req,   label="SGPT（文献[56]）")
    axs[1].set_xlabel("价格", fontsize=18)
    axs[1].set_ylabel("收益", fontsize=18)
    axs[1].set_title("(b) 数据请求者群体的总收益", fontsize=22)
    axs[1].tick_params(labelsize=12)
    axs[1].legend(fontsize=14)

    # Total
    axs[2].plot(price_grid, seg_prov + seg_req, label="SEG")
    axs[2].plot(price_grid, sg_prov + sg_req,   label="SG（文献[75, 76]）")
    axs[2].plot(price_grid, pt_prov + pt_req,   label="SGPT（文献[56]）")
    axs[2].set_xlabel("价格", fontsize=18)
    axs[2].set_ylabel("收益", fontsize=18)
    axs[2].set_title("(c) 整体社会福利", fontsize=22)
    axs[2].tick_params(labelsize=12)
    axs[2].legend(fontsize=14)

    plt.tight_layout()
    plt.show()