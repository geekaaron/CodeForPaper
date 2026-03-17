import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import gaussian_kde

plt.rcParams['font.family']=' Times New Roman, SimSun'

def plot_kde(ax, values, title, xscale=None, linthresh=None):
    values = np.asarray(values)

    kde = gaussian_kde(values)
    x = np.linspace(values.min(), values.max(), 2000)
    y = kde(x)

    # KDE line
    ax.plot(
        x, y,
        color="#2A4E7E",
        linewidth=2.2
    )

    # Fill
    ax.fill_between(
        x, y,
        color="#4C72B0",
        alpha=0.30
    )

    # Mode
    x_peak = int(x[np.argmax(y)])
    ax.axvline(
        x_peak,
        color="#D62728",
        linestyle="--",
        linewidth=1.4,
        alpha=0.9,
        label=f"众数 ≈ {x_peak}"
    )

    # Axis scale
    if xscale == "symlog":
        ax.set_xscale("symlog", linthresh=linthresh)

    ax.set_title(title, fontsize=22)
    ax.set_xlabel("数据提供者规模", fontsize=18)
    ax.set_ylabel("核密度", fontsize=18)
    ax.tick_params(labelsize=14)
    ax.legend(frameon=False, loc="upper right", fontsize=18)

# Snowflake
df_snow = pd.read_csv("snowflake_marketplace_category_stats.csv")
snow_vals = df_snow["Num_Providers"].values

# Shanghai
df_sh = pd.read_csv("aiTag_provider_stats.csv")
sh_vals = df_sh["提供者数量"].values

# Ocean
df_ocean = pd.read_csv("ocean_submarket_statistics.csv")
ocean_vals = df_ocean["Num_Providers"].values

fig, axes = plt.subplots(
    1, 3,
    figsize=(15, 4),
    constrained_layout=True
)

plot_kde(
    axes[0],
    snow_vals,
    title="(a) Snowflake数据市场"
)

plot_kde(
    axes[1],
    sh_vals,
    title="(b) 上海数据交易所",
    xscale="symlog",
    linthresh=5
)

axes[1].set_xlim(left=0)


plot_kde(
    axes[2],
    ocean_vals,
    title="(c) Ocean Protocol数据市场",
)

plt.savefig("provider_distribution.svg", dpi=300, bbox_inches="tight")
plt.show()
