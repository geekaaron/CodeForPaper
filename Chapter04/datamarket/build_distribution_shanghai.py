import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import gaussian_kde

plt.rcParams['font.family']=' Times New Roman, SimSun'

df = pd.read_csv("aiTag_provider_stats.csv")
provider_values = df["提供者数量"].values

kde = gaussian_kde(provider_values)

x = np.linspace(provider_values.min(), provider_values.max(), 1000)
y = kde(x)

idx_max = np.argmax(y)
x_peak = x[idx_max]
y_peak = y[idx_max]

print(f"[INFO] KDE density peak at Num_Providers = {x_peak:.2f}")
print(f"[INFO] Peak density value = {y_peak:.4f}")

plt.figure(figsize=(5, 4))

plt.plot(
    x,
    y,
    color="#2A4E7E",
    linewidth=2.5
)

plt.fill_between(
    x,
    y,
    alpha=0.3,
    color="#4C72B0"
)

x_peak = int(x[np.argmax(y)])
plt.axvline(
    x_peak,
    color="#D62728",
    linestyle="--",
    linewidth=1.4,
    alpha=0.9,
    label=f"众数 ≈ {x_peak}"
)

plt.xscale("symlog", linthresh=5)
plt.xlim(left=0)

plt.xlabel("数据提供者规模", fontsize=20)
plt.ylabel("核密度", fontsize=20)
plt.tick_params(labelsize=14)
plt.legend(frameon=False, loc="upper right", fontsize=18)

plt.tight_layout()
plt.savefig("shanghai.svg", dpi=300)
plt.show()
