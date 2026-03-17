import numpy as np
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

plt.rcParams['font.family']=' Times New Roman, SimSun'

N = 100
RUNS = 10000

R = np.linspace(0.1, 1.0, N)
w = R / R.sum()

rank_count_pure = np.zeros((N, N))
rank_count_w = np.zeros((N, N))

for _ in range(RUNS):
    vrf = np.random.rand(N)

    # ---------- Pure VRF ----------
    order_p = np.argsort(vrf)[::-1]
    ranks_p = np.empty(N, dtype=int)
    ranks_p[order_p] = np.arange(1, N + 1)

    for i in range(N):
        rank_count_pure[i, ranks_p[i] - 1] += 1

    # ---------- WVRF ----------
    wvrf = w * vrf
    order_w = np.argsort(wvrf)[::-1]
    ranks_w = np.empty(N, dtype=int)
    ranks_w[order_w] = np.arange(1, N + 1)

    for i in range(N):
        rank_count_w[i, ranks_w[i] - 1] += 1

rank_prob_pure = rank_count_pure / RUNS
rank_prob_w = rank_count_w / RUNS

fig = plt.figure(figsize=(16, 5))
gs = GridSpec(1, 3, width_ratios=[1, 1, 0.05], wspace=0.2)

ax1 = fig.add_subplot(gs[0, 0])  # Pure VRF
ax2 = fig.add_subplot(gs[0, 1])  # WVRF
cax = fig.add_subplot(gs[0, 2])  # Colorbar

# ----- Pure VRF -----
im0 = ax1.imshow(
    rank_prob_pure.T,
    origin="lower",
    aspect="auto",
    extent=[R.min(), R.max(), 1, N],
    cmap="viridis"
)
ax1.invert_yaxis()
ax1.set_xlabel("声誉", fontsize=20)
ax1.set_ylabel("排序", fontsize=20)
ax1.tick_params(labelsize=16)


# ----- WVRF -----
im1 = ax2.imshow(
    rank_prob_w.T,
    origin="lower",
    aspect="auto",
    extent=[R.min(), R.max(), 1, N],
    cmap="viridis"
)
ax2.invert_yaxis()
ax2.set_xlabel("声誉", fontsize=20)
ax2.set_ylabel("排序", fontsize=20)
ax2.tick_params(labelsize=16)

cbar = fig.colorbar(im1, cax=cax)
cbar.set_label("概率", fontsize=20)      # colorbar标题大小
cbar.ax.tick_params(labelsize=16)       # colorbar刻度大小
plt.subplots_adjust(left=0.05, right=0.95, top=1, bottom=0.12)

plt.show()
