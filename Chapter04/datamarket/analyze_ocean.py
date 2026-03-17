from bs4 import BeautifulSoup
import pandas as pd
import re

# ===============================
# 1. 加载 Ocean Market HTML
# ===============================
HTML_FILE = "941 results - Ocean Market.html"

with open(HTML_FILE, "r", encoding="utf-8") as f:
    soup = BeautifulSoup(f, "lxml")

assets = soup.find_all("article")

print(f"[INFO] Total assets found: {len(assets)}")

CATEGORY_KEYWORDS = {
    "Financial / Crypto": [
        "btc", "eth", "usdt", "ohlc", "orderbook", "price", "historical"
    ],
    "Transportation / Vehicle": [
        "vehicle", "car", "traffic", "road", "driving"
    ],
    "Healthcare / Medical": [
        "medical", "health", "brain", "clinical", "patient"
    ],
    "AI / Perception": [
        "ai", "perception", "vision", "image", "dataset", "sign"
    ],
    "IoT / Sensor": [
        "iot", "sensor", "device", "signal"
    ],
    "Generic / Demo": [
        "dummy", "test", "example"
    ]
}

def classify_category(text):
    text = text.lower()
    for cat, keywords in CATEGORY_KEYWORDS.items():
        for kw in keywords:
            if kw in text:
                return cat
    return "Others"

records = []

for asset in assets:
    title_tag = asset.find("h1")
    publisher_tag = asset.find("div", class_="Publisher_publisher__TkU_v")
    content_tag = asset.find("div", class_="AssetTeaser_content__AEgsT")
    footer_tag = asset.find("footer")

    title = title_tag.text.strip() if title_tag else ""
    publisher = publisher_tag.text.strip() if publisher_tag else "Unknown"
    content = content_tag.text.strip() if content_tag else ""

    text_blob = f"{title} {content}"

    category = classify_category(text_blob)

    # 提取销量
    sales = 0
    if footer_tag:
        sale_match = re.search(r"(\d+)\s+sale", footer_tag.text)
        if sale_match:
            sales = int(sale_match.group(1))

    records.append({
        "title": title,
        "publisher": publisher,
        "category": category,
        "sales": sales
    })

df = pd.DataFrame(records)

summary = []

for category, group in df.groupby("category"):
    providers = group["publisher"].unique()
    summary.append({
        "Category (Sub-market)": category,
        "Num_Assets": len(group),
        "Num_Providers": len(providers),
        "Total_Sales": group["sales"].sum()
    })

summary_df = pd.DataFrame(summary).sort_values(
    by="Num_Providers", ascending=False
)

print("\n=== Sub-market Statistics ===")
print(summary_df)

summary_df.to_csv("ocean_submarket_statistics.csv", index=False)
print("\n[INFO] Results saved to ocean_submarket_statistics.csv")
