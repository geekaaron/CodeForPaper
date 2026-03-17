import requests
import csv
from collections import defaultdict
from tabulate import tabulate
import time

API_URL = "https://nidts.chinadep.com/dex-api/dex-es/query/all/search"

HEADERS = {
    "User-Agent": "Mozilla/5.0",
    "Content-Type": "application/json"
}

PAGE_SIZE = 7790
SLEEP_TIME = 0.5

tag_asset_count = defaultdict(int)     # aiTag -> 数据产品数
tag_providers = defaultdict(set)       # aiTag -> 提供者集合（去重）

page_num = 1
total_page = 1

while True:
    payload = {
        "pageNum": page_num,
        "pageSize": PAGE_SIZE,
        "operationSubject": "",
        "dataType": "",
        "serviceType": "",
        "zone": "",
        "sectorName": "",
        "aiTag": ""
    }

    print(f"[INFO] Fetching page {page_num}")
    r = requests.post(API_URL, json=payload, headers=HEADERS, timeout=15)
    r.raise_for_status()

    resp = r.json()
    data = resp["data"]

    if total_page is None:
        total_page = data["totalPage"]
        print(f"[INFO] Total pages: {total_page}, total assets: {data['total']}")

    items = data["list"]
    if not items:
        break

    for item in items:
        provider = item.get("supplierCompanyName", "").strip()
        tags = item.get("aiTag", [])

        if not provider or not tags:
            continue

        for tag in tags:
            tag = tag.strip()
            if not tag:
                continue

            tag_asset_count[tag] += 1
            tag_providers[tag].add(provider)

    if page_num >= total_page:
        break

    page_num += 1
    time.sleep(SLEEP_TIME)


table = []
for tag in sorted(tag_asset_count, key=lambda x: len(tag_providers[x]), reverse=True):
    table.append([
        tag,
        tag_asset_count[tag],
        len(tag_providers[tag])
    ])

csv_file = "aiTag_provider_stats.csv"

with open(csv_file, "w", newline="", encoding="utf-8-sig") as f:
    writer = csv.writer(f)
    writer.writerow(["AI标签", "数据产品数量", "提供者数量"])
    for row in table:
        writer.writerow(row)

print(f"\n[INFO] CSV 文件已保存：{csv_file}")