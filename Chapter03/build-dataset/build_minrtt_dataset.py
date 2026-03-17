import bz2
import argparse
import random
import math
from collections import defaultdict
from tqdm import tqdm

import networkx as nx
import matplotlib.pyplot as plt

plt.rcParams["font.family"] = "Times New Roman"
plt.rcParams["mathtext.fontset"] = "stix"


# =========================================================
# 1. Load minRTT dataset
# =========================================================
def load_minrtt(path):
    probe_site = defaultdict(dict)
    with bz2.open(path, "rt", encoding="utf-8", errors="ignore") as f:
        for line in f:
            parts = line.split()
            if len(parts) != 3:
                continue
            probe, site, rtt = parts
            probe_site[probe][site] = float(rtt)
    return probe_site


# =========================================================
# 2. Probe–probe RTT
# =========================================================
def compute_pair_rtt(a, b):
    best = math.inf
    for site in a:
        if site in b:
            best = min(best, a[site] + b[site])
    return best


# =========================================================
# 3. Select N connected probes
# =========================================================
def select_connected_probes(probes, probe_site, n, rtt_threshold):
    random.shuffle(probes)
    selected = [probes[0]]
    remaining = set(probes[1:])

    with tqdm(total=n, desc="Selecting probes") as pbar:
        pbar.update(1)
        while len(selected) < n and remaining:
            candidate = random.choice(list(remaining))
            remaining.remove(candidate)

            for s in selected:
                rtt = compute_pair_rtt(
                    probe_site[s],
                    probe_site[candidate]
                )
                if rtt <= rtt_threshold:
                    selected.append(candidate)
                    pbar.update(1)
                    break

    return selected


# =========================================================
# 4. Build all valid RTT edges
# =========================================================
def build_edges(probes, probe_site, rtt_threshold):
    edges = []
    n = len(probes)

    for i in tqdm(range(n), desc="Computing RTT"):
        for j in range(i + 1, n):
            rtt = compute_pair_rtt(
                probe_site[probes[i]],
                probe_site[probes[j]]
            )
            if rtt <= rtt_threshold:
                edges.append((i, j, rtt))

    return edges


# =========================================================
# 5. Random spanning tree
# =========================================================
def random_spanning_tree(n, edges):
    adj = defaultdict(list)
    for u, v, w in edges:
        adj[u].append((v, w))
        adj[v].append((u, w))

    visited = {0}
    tree = []
    frontier = [(0, v, w) for v, w in adj[0]]

    pbar = tqdm(total=n - 1, desc="Building random spanning tree")

    while frontier and len(visited) < n:
        u, v, w = frontier.pop(random.randrange(len(frontier)))
        if v in visited:
            continue

        visited.add(v)
        tree.append((u, v, w))
        pbar.update(1)

        for nv, nw in adj[v]:
            if nv not in visited:
                frontier.append((v, nv, nw))

    pbar.close()

    if len(tree) != n - 1:
        raise RuntimeError("Random spanning tree failed")

    return tree


# =========================================================
# 6. Add extra edges
# =========================================================
def add_extra_edges(tree, all_edges, target_edge_count):
    tree_set = set((min(u, v), max(u, v)) for u, v, _ in tree)
    candidates = [
        e for e in all_edges
        if (min(e[0], e[1]), max(e[0], e[1])) not in tree_set
    ]
    random.shuffle(candidates)

    needed = target_edge_count - len(tree)
    return tree + candidates[:needed]


# =========================================================
# 7. MST
# =========================================================
def mst(n, edges):
    parent = list(range(n))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[rb] = ra
            return True
        return False

    tree = []
    for u, v, w in sorted(edges, key=lambda x: x[2]):
        if union(u, v):
            tree.append((u, v, w))
            if len(tree) == n - 1:
                break

    return tree


# =========================================================
# 8. Export edges & mapping
# =========================================================
def export_edges(edges, path):
    with open(path, "w") as f:
        f.write("# u v rtt_ms\n")
        for u, v, w in edges:
            f.write(f"{u} {v} {int(w)}\n")


def export_mapping(id2probe, path):
    with open(path, "w") as f:
        f.write("# node_id probe_id\n")
        for i, p in enumerate(id2probe):
            f.write(f"{i} {p}\n")


# =========================================================
# 9. Draw topology with probe labels
# =========================================================
def draw_topology(n, mst_edges, extra_edges, id2probe, out_png):
    G = nx.Graph()
    G.add_nodes_from(range(n))

    for u, v, w in mst_edges + extra_edges:
        G.add_edge(u, v, rtt=w)

    rtts = [w for _, _, w in mst_edges + extra_edges]
    rmin, rmax = min(rtts), max(rtts)

    def width(w):
        return 0.8 + 2.5 * (
            1 - (math.sqrt(w) - math.sqrt(rmin)) /
            (math.sqrt(rmax) - math.sqrt(rmin) + 1e-9)
        )

    pos = nx.spring_layout(
        G,
        seed=42,
        k=2.6 / math.sqrt(n),
        iterations=300
    )

    plt.figure(figsize=(11, 7))

    # === 节点 ===
    nx.draw_networkx_nodes(
        G, pos,
        node_size=350,
        node_color="#4C72B0",
        edgecolors="black",
        linewidths=0.6
    )

    # === extra edges（灰色虚线）===
    for u, v, w in extra_edges:
        nx.draw_networkx_edges(
            G, pos,
            [(u, v)],
            width=width(w),
            style="dashed",
            alpha=0.35,
            edge_color="gray"
        )

    # === MST edges（绿色实线）===
    for u, v, w in mst_edges:
        nx.draw_networkx_edges(
            G, pos,
            [(u, v)],
            width=width(w) * 1.2,
            edge_color="#55A868",
            alpha=0.9
        )

    # === 节点标签：原始 probe ID ===
    labels = {i: id2probe[i] for i in range(n)}
    nx.draw_networkx_labels(
        G, pos,
        labels=labels,
        font_size=8,
        font_weight="bold"
    )

    # === MST 边上的 RTT 标签（保留你原来的）===
    edge_labels = {(u, v): f"{int(w)}ms" for u, v, w in mst_edges}
    nx.draw_networkx_edge_labels(
        G, pos,
        edge_labels=edge_labels,
        font_size=7
    )

    #plt.title(f"Random Selection RTT Topology (N={n})", fontsize=16)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(out_png, dpi=200)
    plt.close()



# =========================================================
# 10. Main
# =========================================================
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nodes", type=int, required=True)
    parser.add_argument("--rtt_threshold", type=float, required=True)
    parser.add_argument("--minrtt", required=True)
    parser.add_argument("--seed", type=int, default=0)
    args = parser.parse_args()

    nodes = args.nodes
    seed = args.seed

    edge_out = f"edge_{nodes}_{seed}.txt"
    fig_out  = f"rtt_{nodes}_{seed}.svg"
    map_out  = f"map_{nodes}_{seed}.txt"

    random.seed(seed)

    print("[*] Loading dataset...")
    probe_site = load_minrtt(args.minrtt)
    probes = list(probe_site.keys())

    print("[*] Selecting connected probes...")
    selected = select_connected_probes(
        probes, probe_site, args.nodes, args.rtt_threshold
    )

    # 建立映射
    id2probe = list(selected)
    probe2id = {p: i for i, p in enumerate(id2probe)}

    print("[*] Building RTT edges...")
    edges = build_edges(id2probe, probe_site, args.rtt_threshold)

    print("[*] Random spanning tree...")
    tree = random_spanning_tree(args.nodes, edges)

    print("[*] Adding extra edges...")
    sparse_edges = add_extra_edges(tree, edges, 2 * (args.nodes - 1))

    print("[*] MST...")
    mst_edges = mst(args.nodes, sparse_edges)
    extra_edges = [e for e in sparse_edges if e not in mst_edges]

    print("[*] Exporting data...")
    export_edges(mst_edges + extra_edges, edge_out)
    export_mapping(id2probe, map_out)

    print("[*] Drawing topology...")
    draw_topology(args.nodes, mst_edges, extra_edges, id2probe, fig_out)

    print("[✓] Done")


if __name__ == "__main__":
    main()
