import requests
import csv
from collections import defaultdict
import time

API_URL = "https://app.snowflake.com/v0/guest/snowscope/browse"

HEADERS = {
    "User-Agent": "Mozilla/5.0",
    "Content-Type": "application/json"
}

TOTAL_PRODUCTS = 3657
LIMIT = 3657
SLEEP_TIME = 0.5


category_asset_count = defaultdict(int)   # category -> 产品数
category_providers = defaultdict(set)     # category -> provider set

offset = 0

while offset < TOTAL_PRODUCTS:
    payload = {
        "filterContext": {
            "region": "ALL",
            "installedListings": [],
            "purchasedListings": []
        },
        "filter": {
            "cloudRegion": [],
            "includePrivateListings": False,
            "includeOnlyIntraOrgListings": False,
            "includeIntraOrgListings": False,
            "dataExchanges": ["SNOWFLAKE_DATA_MARKETPLACE"]
        },
        "sort": {"field": "mostPopular"},
        "numSnippets": 10,
        "corpus": "marketplace",
        "client": "marketplaceSearch",
        "resultGroups": True,
        "pagination": {
            "limit": LIMIT,
            "offset": offset
        }
    }

    print(f"[INFO] Fetching offset {offset}")
    r = requests.post(API_URL, json=payload, headers=HEADERS, timeout=20)
    r.raise_for_status()

    resp = r.json()
    groups = resp.get("resultGroups", [])

    parsed = 0

    for group in groups:
        for item in group.get("results", []):
            listing = item.get("typeSpecific", {}).get("listing", {})
            if not listing:
                continue

            # -------- provider（关键修正点）--------
            provider = (
                listing
                .get("provider", {})
                .get("name", "")
                .strip()
            )
            if not provider:
                continue

            # -------- categories --------
            categories = listing.get("categories", [])
            if not categories:
                categories = ["Uncategorized"]

            for cat in categories:
                category_asset_count[cat] += 1
                category_providers[cat].add(provider)
                parsed += 1

    if parsed == 0:
        print("[WARN] No listings parsed at this offset")

    offset += LIMIT
    time.sleep(SLEEP_TIME)


csv_file = "snowflake_marketplace_category_stats.csv"

with open(csv_file, "w", newline="", encoding="utf-8-sig") as f:
    writer = csv.writer(f)
    writer.writerow(["Category", "Num_Products", "Num_Providers"])

    for cat in sorted(
        category_asset_count,
        key=lambda x: len(category_providers[x]),
        reverse=True
    ):
        writer.writerow([
            cat,
            category_asset_count[cat],
            len(category_providers[cat])
        ])

print(f"\n[OK] CSV saved: {csv_file}")
