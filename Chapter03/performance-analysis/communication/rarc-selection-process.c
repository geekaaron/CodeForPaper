#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/obj_mac.h>

/* ================= CONFIG ================= */

#define MAX_NODES      1000
#define MAX_NEIGHBORS  1000
#define MAX_EVENTS     4000000

#define VRF_GENERATE_COST   0.031   /* ms */
#define VRF_VERIFY_COST     0.067   /* ms */

/* GA params (tunable) */
#define GA_MAX_GEN      200
#define GA_STALL_J      10
#define GA_EPS          1e-4

/* ================= RTT GRAPH ================= */

typedef struct { int to; int rtt; } edge_t; /* one-way delay in ms */
typedef struct { int deg; edge_t nbr[MAX_NEIGHBORS]; } adj_t;

static adj_t GRAPH[MAX_NODES];
static int NUM_NODES;

/* ================= NODE ================= */

typedef struct {
    int id;
    int rep;

    char *priv_pem;
    char *pub_pem;

    /* VRF flooding state */
    int seen_vrf[MAX_NODES];
    double vrf_raw[MAX_NODES];
    long long vrf_ts[MAX_NODES];
} Node;

static Node nodes[MAX_NODES];
static char *node_priv_pem[MAX_NODES];
static char *node_pub_pem[MAX_NODES];

/* ================= TIME (CPU) ================= */

static inline long long now_cpu_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

/* ================= HEX ================= */

static char *hex_encode(const unsigned char *buf, size_t len) {
    char *hex = (char*)malloc(len * 2 + 1);
    if (!hex) return NULL;
    for (size_t i = 0; i < len; i++) sprintf(hex + 2*i, "%02x", buf[i]);
    hex[len*2] = '\0';
    return hex;
}

static unsigned char *hex_decode(const char *hex, size_t *out_len) {
    size_t slen = strlen(hex);
    if (slen % 2 != 0) return NULL;
    size_t len = slen / 2;
    unsigned char *buf = (unsigned char*)malloc(len);
    if (!buf) return NULL;
    for (size_t i = 0; i < len; i++) sscanf(hex + 2*i, "%2hhx", &buf[i]);
    *out_len = len;
    return buf;
}

static double gamma_to_double01(const char *gamma_hex) {
    size_t len = 0;
    unsigned char *b = hex_decode(gamma_hex, &len);
    if (!b || len == 0) { free(b); return 0.0; }
    unsigned long long v = 0;
    for (int i = 0; i < 8 && i < (int)len; i++) v = (v << 8) | b[i];
    free(b);
    return (double)v / (double)0xFFFFFFFFFFFFFFFFULL;
}

/* ================= VRF (hash + ECDSA proof) ================= */

static int gen_ec_keypair_pem(char **priv_pem, char **pub_pem) {
    int ok = 0;
    EC_KEY *eckey = NULL;
    BIO *b_priv = NULL, *b_pub = NULL;
    BUF_MEM *bptr = NULL;

    *priv_pem = NULL; *pub_pem = NULL;

    eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!eckey) goto done;
    if (EC_KEY_generate_key(eckey) != 1) goto done;

    b_priv = BIO_new(BIO_s_mem());
    b_pub  = BIO_new(BIO_s_mem());
    if (!b_priv || !b_pub) goto done;

    if (PEM_write_bio_ECPrivateKey(b_priv, eckey, NULL, NULL, 0, NULL, NULL) != 1) goto done;
    if (PEM_write_bio_EC_PUBKEY(b_pub, eckey) != 1) goto done;

    BIO_get_mem_ptr(b_priv, &bptr);
    *priv_pem = strndup(bptr->data, bptr->length);

    BIO_get_mem_ptr(b_pub, &bptr);
    *pub_pem = strndup(bptr->data, bptr->length);

    if (!*priv_pem || !*pub_pem) goto done;
    ok = 1;

done:
    if (!ok) { free(*priv_pem); free(*pub_pem); *priv_pem = *pub_pem = NULL; }
    if (b_priv) BIO_free(b_priv);
    if (b_pub)  BIO_free(b_pub);
    if (eckey)  EC_KEY_free(eckey);
    return ok;
}

static EC_KEY *load_priv_from_pem(const char *pem) {
    BIO *b = BIO_new_mem_buf((void*)pem, -1);
    if (!b) return NULL;
    EC_KEY *k = PEM_read_bio_ECPrivateKey(b, NULL, NULL, NULL);
    BIO_free(b);
    return k;
}

static EVP_PKEY *load_pub_from_pem(const char *pem) {
    BIO *b = BIO_new_mem_buf((void*)pem, -1);
    if (!b) return NULL;
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(b, NULL, NULL, NULL);
    BIO_free(b);
    return pkey;
}

static int vrf_prove(int node_id, int round, char **gamma_hex, char **proof_hex) {
    int ok = 0;
    EC_KEY *ec = NULL;
    ECDSA_SIG *sig = NULL;
    unsigned char digest[SHA256_DIGEST_LENGTH];

    *gamma_hex = NULL; *proof_hex = NULL;

    char alpha[64];
    snprintf(alpha, sizeof(alpha), "%d-%d", node_id, round);
    SHA256((unsigned char*)alpha, strlen(alpha), digest);

    *gamma_hex = hex_encode(digest, SHA256_DIGEST_LENGTH);
    if (!*gamma_hex) goto done;

    ec = load_priv_from_pem(node_priv_pem[node_id]);
    if (!ec) goto done;

    sig = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, ec);
    if (!sig) goto done;

    int len = i2d_ECDSA_SIG(sig, NULL);
    if (len <= 0) goto done;

    unsigned char *buf = (unsigned char*)malloc((size_t)len);
    if (!buf) goto done;

    unsigned char *p = buf;
    if (i2d_ECDSA_SIG(sig, &p) != len) { free(buf); goto done; }

    *proof_hex = hex_encode(buf, (size_t)len);
    free(buf);
    if (!*proof_hex) goto done;

    ok = 1;

done:
    if (!ok) { free(*gamma_hex); free(*proof_hex); *gamma_hex = *proof_hex = NULL; }
    if (sig) ECDSA_SIG_free(sig);
    if (ec)  EC_KEY_free(ec);
    return ok;
}

static int vrf_verify(int node_id, int round, const char *gamma_hex, const char *proof_hex) {
    unsigned char digest[SHA256_DIGEST_LENGTH];

    char alpha[64];
    snprintf(alpha, sizeof(alpha), "%d-%d", node_id, round);
    SHA256((unsigned char*)alpha, strlen(alpha), digest);

    size_t g_len = 0;
    unsigned char *g = hex_decode(gamma_hex, &g_len);
    if (!g || g_len != SHA256_DIGEST_LENGTH) { free(g); return 0; }
    if (memcmp(g, digest, SHA256_DIGEST_LENGTH) != 0) { free(g); return 0; }
    free(g);

    size_t p_len = 0;
    unsigned char *pbuf = hex_decode(proof_hex, &p_len);
    if (!pbuf) return 0;

    const unsigned char *pp = pbuf;
    ECDSA_SIG *sig = d2i_ECDSA_SIG(NULL, &pp, (long)p_len);
    if (!sig) { free(pbuf); return 0; }

    EVP_PKEY *pk = load_pub_from_pem(node_pub_pem[node_id]);
    if (!pk) { free(pbuf); ECDSA_SIG_free(sig); return 0; }

    EC_KEY *ec = EVP_PKEY_get1_EC_KEY(pk);
    if (!ec) { free(pbuf); ECDSA_SIG_free(sig); EVP_PKEY_free(pk); return 0; }

    int ok = ECDSA_do_verify(digest, SHA256_DIGEST_LENGTH, sig, ec);

    EC_KEY_free(ec);
    EVP_PKEY_free(pk);
    ECDSA_SIG_free(sig);
    free(pbuf);

    return ok == 1;
}

/* ================= LOAD GRAPH ================= */

static void load_graph(const char *file) {
    // reset deg
    for (int i = 0; i < MAX_NODES; i++) GRAPH[i].deg = 0;

    FILE *f = fopen(file, "r");
    if (!f) { perror("rtt"); exit(1); }

    char line[256];
    int u, v, w;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 3) continue;
        if (sscanf(line, "%d %d %d", &u, &v, &w) == 3) {
            if (u < 0 || v < 0 || u >= MAX_NODES || v >= MAX_NODES) continue;
            int one_way_rtt = w / 2;
            if (GRAPH[u].deg < MAX_NEIGHBORS) GRAPH[u].nbr[GRAPH[u].deg++] = (edge_t){v, one_way_rtt};
            if (GRAPH[v].deg < MAX_NEIGHBORS) GRAPH[v].nbr[GRAPH[v].deg++] = (edge_t){u, one_way_rtt};
        }
    }
    fclose(f);
}

/* ================= GA (paper fitness; keep exact committee size B via repair) ================= */

typedef struct {
    unsigned char *g; /* length k */
    double fit;
} Chrom;

static int cmp_chrom_desc(const void *a, const void *b) {
    const Chrom *x = (const Chrom*)a;
    const Chrom *y = (const Chrom*)b;
    if (x->fit != y->fit) return (y->fit > x->fit) - (y->fit < x->fit);
    return 0;
}

/* paper Eq.(12): avg reputation + size term */
static double fitness(const unsigned char *x, const double *R, int num_nodes) {
    double sum = 0.0;
    int b = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (x[i]) { sum += R[i]; b++; }
    }
    if (b == 0) return 0.0;
    double avg = sum / (double)b;
    double size_term = 1.0 - fabs(((double)b / (double)num_nodes) - 0.5);
    return avg + size_term;
}

static void init_gene(unsigned char *g, int num_nodes, int top_k) {
    memset(g, 0, (size_t)num_nodes);
    int chosen = 0;
    while (chosen < top_k) {
        int p = rand() % num_nodes;
        if (!g[p]) { g[p] = 1; chosen++; }
    }
}

static void repair(unsigned char *g, int num_nodes, int top_k) {
    int ones = 0;
    for (int i = 0; i < num_nodes; i++) ones += (g[i] != 0);

    while (ones > top_k) {
        int p = rand() % num_nodes;
        if (g[p]) { g[p] = 0; ones--; }
    }
    while (ones < top_k) {
        int p = rand() % num_nodes;
        if (!g[p]) { g[p] = 1; ones++; }
    }
}

/* swap-mutation keeps #ones constant */
static void mutate_swap(unsigned char *g, int num_nodes) {
    int p1 = -1, p0 = -1;
    while (p1 < 0) { int t = rand()%num_nodes; if (g[t]) p1 = t; }
    while (p0 < 0) { int t = rand()%num_nodes; if (!g[t]) p0 = t; }
    g[p1] = 0;
    g[p0] = 1;
}

static void ga_run_once(const double *R, int num_nodes, int top_k, unsigned char *best_out) {
    Chrom *pop = (Chrom*)calloc((size_t)num_nodes, sizeof(Chrom));
    if (!pop) { fprintf(stderr, "GA alloc failed\n"); exit(1); }

    for (int i = 0; i < num_nodes; i++) {
        pop[i].g = (unsigned char*)calloc((size_t)num_nodes, 1);
        if (!pop[i].g) { fprintf(stderr, "GA gene alloc failed\n"); exit(1); }
        init_gene(pop[i].g, num_nodes, top_k);
        pop[i].fit = fitness(pop[i].g, R, num_nodes);
    }

    double best_prev = -1e100;
    int stall = 0;

    for (int gen = 0; gen < GA_MAX_GEN; gen++) {
        qsort(pop, (size_t)num_nodes, sizeof(Chrom), cmp_chrom_desc);
        double best_now = pop[0].fit;

        if (best_prev > 0 && fabs(best_now - best_prev) / (fabs(best_prev) + 1e-12) < GA_EPS)
            stall++;
        else
            stall = 0;
        best_prev = best_now;

        if (stall >= GA_STALL_J) break;

        /* crossover top-2 (single-point) */
        int cp = 1 + (rand() % (num_nodes - 1));
        Chrom c1 = {0}, c2 = {0}, mut = {0};
        c1.g = (unsigned char*)calloc((size_t)num_nodes, 1);
        c2.g = (unsigned char*)calloc((size_t)num_nodes, 1);
        mut.g = (unsigned char*)calloc((size_t)num_nodes, 1);
        if (!c1.g || !c2.g || !mut.g) { fprintf(stderr, "GA temp alloc failed\n"); exit(1); }

        for (int i = 0; i < num_nodes; i++) {
            c1.g[i] = pop[0].g[i];
            c2.g[i] = pop[1].g[i];
            mut.g[i]= pop[0].g[i];
        }
        for (int i = cp; i < num_nodes; i++) {
            unsigned char t = c1.g[i]; c1.g[i] = c2.g[i]; c2.g[i] = t;
        }
        repair(c1.g, num_nodes, top_k);
        repair(c2.g, num_nodes, top_k);

        /* mutation on best: swap 1 and 0 */
        mutate_swap(mut.g, num_nodes);

        c1.fit = fitness(c1.g, R, num_nodes);
        c2.fit = fitness(c2.g, R, num_nodes);
        mut.fit= fitness(mut.g, R, num_nodes);

        /* replace 3 worst */
        int w1 = num_nodes - 1, w2 = num_nodes - 2, w3 = num_nodes - 3;
        free(pop[w1].g); pop[w1] = c1;
        free(pop[w2].g); pop[w2] = c2;
        free(pop[w3].g); pop[w3] = mut;
    }

    qsort(pop, (size_t)num_nodes, sizeof(Chrom), cmp_chrom_desc);
    memcpy(best_out, pop[0].g, (size_t)num_nodes);

    for (int i = 0; i < num_nodes; i++) free(pop[i].g);
    free(pop);
}

/* ================= "ONE-BROADCAST FAST VOTING" (count identical chromosomes) ================= */

static unsigned long long fnv1a64(const unsigned char *buf, size_t len) {
    unsigned long long h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned long long)buf[i];
        h *= 1099511628211ULL;
    }
    return h;
}

/* ================= EVENT SIM (CHROM flooding + VRF flooding) ================= */

typedef enum { MSG_CHROM=1, MSG_VRF=2 } MsgType;

typedef struct {
    double time;   /* simulated ms */
    int from, to;
    MsgType type;

    /* for CHROM: origin candidate index (0..k-1) */
    int origin_idx;

    /* for VRF */
    int vrf_origin_node;
    int vrf_payload_idx; /* index in payload arrays */
} Event;

static Event event_q[MAX_EVENTS];
static int ev_cnt = 0;
static double now_sim = 0.0;

static void push_event(Event e) {
    if (ev_cnt >= MAX_EVENTS) {
        fprintf(stderr, "event queue overflow\n");
        exit(1);
    }
    int i = ev_cnt++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (event_q[p].time <= e.time) break;
        event_q[i] = event_q[p];
        i = p;
    }
    event_q[i] = e;
}

static Event pop_event(void) {
    Event ret = event_q[0];
    Event last = event_q[--ev_cnt];
    int i = 0;
    while (1) {
        int l = 2*i+1, r = l+1;
        if (l >= ev_cnt) break;
        int m = (r < ev_cnt && event_q[r].time < event_q[l].time) ? r : l;
        if (event_q[m].time >= last.time) break;
        event_q[i] = event_q[m];
        i = m;
    }
    event_q[i] = last;
    return ret;
}

/* ================= MAIN ================= */

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <round> <num_nodes> <rtt_file>\n", argv[0]);
        return 1;
    }

    int ROUND = atoi(argv[1]);
    NUM_NODES = atoi(argv[2]);
    const char *rtt_file = argv[3];

    if (NUM_NODES < 4 || NUM_NODES > MAX_NODES) {
        fprintf(stderr, "num_candidates_k invalid\n");
        return 1;
    }

    int TOP_K = 0;

    if (NUM_NODES >= 4 && NUM_NODES <= 20) TOP_K = 4 + NUM_NODES / 5;
    else if (NUM_NODES >= 20) TOP_K = 8 + sqrt(NUM_NODES - 20);

    load_graph(rtt_file);

    /* init nodes & keys */
    int max_rep = 1;
    for (int i = 0; i < NUM_NODES; i++) {
        nodes[i].id = i;
        nodes[i].rep = 4 + (i % 7);  /* demo rep; you can replace with your own */
        if (nodes[i].rep > max_rep) max_rep = nodes[i].rep;

        memset(nodes[i].seen_vrf, 0, sizeof(int)*NUM_NODES);
        for (int j = 0; j < NUM_NODES; j++) {
            nodes[i].vrf_raw[j] = 0.0;
            nodes[i].vrf_ts[j]  = 0;
        }

        if (!gen_ec_keypair_pem(&nodes[i].priv_pem, &nodes[i].pub_pem)) {
            fprintf(stderr, "Keypair gen failed at node %d\n", i);
            return 1;
        }
        node_priv_pem[i] = nodes[i].priv_pem;
        node_pub_pem[i]  = nodes[i].pub_pem;
    }

    int *cand_ids = (int*)malloc(sizeof(int) * (size_t)NUM_NODES);
    if (!cand_ids) { fprintf(stderr, "alloc failed\n"); return 1; }
    for (int i = 0; i < NUM_NODES; i++) cand_ids[i] = i;

    int node_to_cidx[MAX_NODES];
    for (int i = 0; i < NUM_NODES; i++) node_to_cidx[i] = i;

    /* reputation array for candidates (normalize to [0,1]) */
    double *R = (double*)malloc(sizeof(double) * (size_t)NUM_NODES);
    if (!R) { fprintf(stderr, "alloc failed\n"); return 1; }
    for (int i = 0; i < NUM_NODES; i++) {
        int nid = cand_ids[i];
        R[i] = (double)nodes[nid].rep / (double)max_rep;
    }

    /* ===================== Step A: each candidate runs GA independently ===================== */

    unsigned char **chrom_best = (unsigned char**)calloc((size_t)NUM_NODES, sizeof(unsigned char*));
    long long *chrom_ts = (long long*)calloc((size_t)NUM_NODES, sizeof(long long));
    double *chrom_fit = (double*)calloc((size_t)NUM_NODES, sizeof(double));
    if (!chrom_best || !chrom_ts || !chrom_fit) { fprintf(stderr, "alloc failed\n"); return 1; }

    clock_t ga_ticks_sum = 0;

    for (int oi = 0; oi < NUM_NODES; oi++) {
        chrom_best[oi] = (unsigned char*)calloc((size_t)NUM_NODES, 1);
        if (!chrom_best[oi]) { fprintf(stderr, "alloc chrom failed\n"); return 1; }

        unsigned seed = 1469598103u ^ (unsigned)ROUND ^ (unsigned)(cand_ids[oi] * 2654435761u);
        srand(seed);

        clock_t start = clock();
        ga_run_once(R, NUM_NODES, TOP_K, chrom_best[oi]);
        clock_t end = clock();
        ga_ticks_sum += (end - start);

        /* ensure exact B ones */
        repair(chrom_best[oi], NUM_NODES, TOP_K);

        chrom_fit[oi] = fitness(chrom_best[oi], R, NUM_NODES);
        chrom_ts[oi]  = now_cpu_ms();
    }

    double ga_cpu_ms = (double)ga_ticks_sum / NUM_NODES * 1000.0 / (double)CLOCKS_PER_SEC;

    /* ===================== Step B: ONLY ONE broadcast (CHROM flooding) among candidates ===================== */

    ev_cnt = 0;
    now_sim = 0.0;

    unsigned char *cand_seen_chrom = (unsigned char*)calloc((size_t)NUM_NODES * (size_t)NUM_NODES, 1);
    int *cand_seen_cnt = (int*)calloc((size_t)NUM_NODES, sizeof(int));
    if (!cand_seen_chrom || !cand_seen_cnt) { fprintf(stderr, "alloc failed\n"); return 1; }

    /* global dedup: (to_node, origin_idx) */
    unsigned char *seen_chrom_all = (unsigned char*)calloc((size_t)NUM_NODES * (size_t)NUM_NODES, 1);
    if (!seen_chrom_all) { fprintf(stderr, "alloc failed\n"); return 1; }

    for (int oi = 0; oi < NUM_NODES; oi++) {
        int origin_node = cand_ids[oi];

        cand_seen_chrom[oi*NUM_NODES + oi] = 1;
        cand_seen_cnt[oi] = 1;
        seen_chrom_all[(size_t)origin_node * (size_t)NUM_NODES + (size_t)oi] = 1;

        for (int j = 0; j < GRAPH[origin_node].deg; j++) {
            edge_t e = GRAPH[origin_node].nbr[j];
            push_event((Event){
                .time = (double)e.rtt,
                .from = origin_node,
                .to = e.to,
                .type = MSG_CHROM,
                .origin_idx = oi
            });
        }
    }

    double chrom_comm_ms = 0.0;

    while (ev_cnt > 0) {
        Event ev = pop_event();
        if (now_sim < ev.time) now_sim = ev.time;
        if (ev.type != MSG_CHROM) continue;

        if (ev.to < 0 || ev.to >= NUM_NODES) continue;

        unsigned char *seen_row_all = &seen_chrom_all[(size_t)ev.to * (size_t)NUM_NODES];
        if (seen_row_all[ev.origin_idx]) {
            continue;
        }
        seen_row_all[ev.origin_idx] = 1;

        int to_cidx = node_to_cidx[ev.to]; /* identity */
        unsigned char *row = &cand_seen_chrom[to_cidx * NUM_NODES];
        if (!row[ev.origin_idx]) {
            row[ev.origin_idx] = 1;
            cand_seen_cnt[to_cidx]++;

            if (cand_seen_cnt[to_cidx] == NUM_NODES && now_sim > chrom_comm_ms)
                chrom_comm_ms = now_sim;
        }

        for (int i = 0; i < GRAPH[ev.to].deg; i++) {
            edge_t e = GRAPH[ev.to].nbr[i];
            if (e.to == ev.from) continue;

            push_event((Event){
                .time = now_sim + (double)e.rtt,
                .from = ev.to,
                .to = e.to,
                .type = MSG_CHROM,
                .origin_idx = ev.origin_idx
            });
        }
    }

    /* ===================== Step C: "Fast voting" with ONE broadcast only ===================== */
    /* Each origin's broadcast chromosome is its vote; pick most frequent identical chromosome. */

    clock_t pick_start = clock();

    typedef struct {
        unsigned long long h;
        int rep_idx;
        int count;
    } Bucket;

    int tb_sz = NUM_NODES * 2 + 1;
    Bucket *tb = (Bucket*)calloc((size_t)tb_sz, sizeof(Bucket));
    if (!tb) { fprintf(stderr, "alloc failed\n"); return 1; }

    for (int oi = 0; oi < NUM_NODES; oi++) {
        unsigned long long h = fnv1a64(chrom_best[oi], (size_t)NUM_NODES);
        int pos = (int)(h % (unsigned long long)tb_sz);
        while (tb[pos].count != 0) {
            if (tb[pos].h == h) {
                int rep = tb[pos].rep_idx;
                if (memcmp(chrom_best[rep], chrom_best[oi], (size_t)NUM_NODES) == 0) {
                    tb[pos].count++;
                    goto next_oi;
                }
            }
            pos = (pos + 1) % tb_sz;
        }
        tb[pos].h = h;
        tb[pos].rep_idx = oi;
        tb[pos].count = 1;

    next_oi:
        ;
    }

    /* choose by max votes, then higher fitness, then earlier ts, then smaller oi */
    int win_oi = -1;
    int win_votes = -1;

    for (int i = 0; i < tb_sz; i++) {
        if (tb[i].count == 0) continue;
        int oi = tb[i].rep_idx;
        int votes = tb[i].count;

        if (win_oi < 0 || votes > win_votes) {
            win_oi = oi; win_votes = votes;
        } else if (votes == win_votes) {
            if (chrom_fit[oi] > chrom_fit[win_oi] + 1e-12) win_oi = oi;
            else if (fabs(chrom_fit[oi] - chrom_fit[win_oi]) <= 1e-12) {
                if (chrom_ts[oi] < chrom_ts[win_oi]) win_oi = oi;
                else if (chrom_ts[oi] == chrom_ts[win_oi] && oi < win_oi) win_oi = oi;
            }
        }
    }

    free(tb);

    clock_t pick_end = clock();
    double pick_winner_ms = (double)(pick_end - pick_start) * 1000.0 / (double)CLOCKS_PER_SEC;

    if (win_oi < 0) win_oi = 0;

    int *committee = (int*)malloc(sizeof(int) * (size_t)TOP_K);
    if (!committee) { fprintf(stderr, "alloc failed\n"); return 1; }
    int bi = 0;
    for (int i = 0; i < NUM_NODES; i++) {
        if (chrom_best[win_oi][i]) committee[bi++] = cand_ids[i];
    }

    /* ===================== Step D: VRF among committee nodes (unchanged) ===================== */

    ev_cnt = 0;
    now_sim = 0.0;

    for (int i = 0; i < NUM_NODES; i++) {
        memset(nodes[i].seen_vrf, 0, sizeof(int)*NUM_NODES);
        for (int j = 0; j < NUM_NODES; j++) {
            nodes[i].vrf_raw[j] = 0.0;
            nodes[i].vrf_ts[j]  = 0;
        }
    }

    /* store VRF payloads so we can reference them by index in events */
    char **gamma_store = (char**)calloc((size_t)TOP_K, sizeof(char*));
    char **proof_store = (char**)calloc((size_t)TOP_K, sizeof(char*));
    int  *origin_store  = (int*)calloc((size_t)TOP_K, sizeof(int));
    double *raw_store   = (double*)calloc((size_t)TOP_K, sizeof(double));
    long long *ts_store = (long long*)calloc((size_t)TOP_K, sizeof(long long));
    if (!gamma_store || !proof_store || !origin_store || !raw_store || !ts_store) {
        fprintf(stderr, "alloc failed\n");
        return 1;
    }

    for (int i = 0; i < TOP_K; i++) {
        int nid = committee[i];

        char *gamma = NULL, *proof = NULL;
        if (!vrf_prove(nid, ROUND, &gamma, &proof)) {
            fprintf(stderr, "vrf_prove failed at node %d\n", nid);
            return 1;
        }

        long long ts = now_cpu_ms();
        double raw = gamma_to_double01(gamma);

        gamma_store[i] = gamma;
        proof_store[i] = proof;
        origin_store[i] = nid;
        raw_store[i] = raw;
        ts_store[i] = ts;

        nodes[nid].seen_vrf[nid] = 1;
        nodes[nid].vrf_raw[nid]  = raw;
        nodes[nid].vrf_ts[nid]   = ts;

        for (int j = 0; j < GRAPH[nid].deg; j++) {
            edge_t e = GRAPH[nid].nbr[j];
            push_event((Event){
                .time = VRF_GENERATE_COST + (double)e.rtt,
                .from = nid,
                .to = e.to,
                .type = MSG_VRF,
                .vrf_origin_node = nid,
                .vrf_payload_idx = i
            });
        }
    }

    while (ev_cnt > 0) {
        Event ev = pop_event();
        if (now_sim < ev.time) now_sim = ev.time;
        if (ev.type != MSG_VRF) continue;

        if (ev.to < 0 || ev.to >= NUM_NODES) continue;

        int origin = ev.vrf_origin_node;
        if (nodes[ev.to].seen_vrf[origin]) continue;

        int idx = ev.vrf_payload_idx;
        const char *gamma_hex = gamma_store[idx];
        const char *proof_hex = proof_store[idx];

        if (!vrf_verify(origin, ROUND, gamma_hex, proof_hex)) continue;

        nodes[ev.to].seen_vrf[origin] = 1;
        nodes[ev.to].vrf_raw[origin]  = raw_store[idx];
        nodes[ev.to].vrf_ts[origin]   = ts_store[idx];

        for (int i = 0; i < GRAPH[ev.to].deg; i++) {
            edge_t e = GRAPH[ev.to].nbr[i];
            if (e.to == ev.from) continue;
            push_event((Event){
                .time = now_sim + VRF_VERIFY_COST + (double)e.rtt,
                .from = ev.to,
                .to = e.to,
                .type = MSG_VRF,
                .vrf_origin_node = origin,
                .vrf_payload_idx = idx
            });
        }
    }

    /* select leader: max VRF output (use node 0's view) */
    clock_t sort_start = clock();
    int leader = committee[0];
    for (int i = 1; i < TOP_K; i++) {
        int nid = committee[i];
        double r1 = nodes[0].vrf_raw[nid];
        double r0 = nodes[0].vrf_raw[leader];
        if (r1 > r0 + 1e-12) leader = nid;
        else if (fabs(r1 - r0) <= 1e-12) {
            if (nodes[0].vrf_ts[nid] < nodes[0].vrf_ts[leader]) leader = nid;
            else if (nodes[0].vrf_ts[nid] == nodes[0].vrf_ts[leader] && nid < leader) leader = nid;
        }
    }
    clock_t sort_end = clock();
    double sort_ms = (double)(sort_end - sort_start) * 1000.0 / (double)CLOCKS_PER_SEC;

    // printf("ga once run: %.3f ms\n", ga_cpu_ms);
    // printf("ga votting time cost: %.3f\n", chrom_comm_ms);
    // printf("ga pick winner time cost: %.3f ms\n", pick_winner_ms);
    // printf("vrf votting time cost: %.3f ms\n", now_sim);
    // printf("sorting time cost: %.3f ms\n", sort_ms);

    double total_ms = ga_cpu_ms + pick_winner_ms + chrom_comm_ms + now_sim + sort_ms;
    printf("%.3f\n", total_ms);

    /* cleanup */
    for (int i = 0; i < TOP_K; i++) {
        free(gamma_store[i]);
        free(proof_store[i]);
    }
    free(gamma_store);
    free(proof_store);
    free(origin_store);
    free(raw_store);
    free(ts_store);

    free(committee);

    free(cand_seen_chrom);
    free(cand_seen_cnt);
    free(seen_chrom_all);

    for (int i = 0; i < NUM_NODES; i++) free(chrom_best[i]);
    free(chrom_best);
    free(chrom_ts);
    free(chrom_fit);

    free(R);
    free(cand_ids);

    for (int i = 0; i < NUM_NODES; i++) {
        free(nodes[i].priv_pem);
        free(nodes[i].pub_pem);
    }

    return 0;
}
