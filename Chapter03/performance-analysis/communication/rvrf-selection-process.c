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
#define MAX_EVENTS     2000000

#define VRF_GENERATE_COST   0.031   /* ms */
#define VRF_VERIFY_COST     0.067   /* ms */

#define EPS 1e-9

/* ================= RTT GRAPH ================= */

typedef struct {
    int to;
    int rtt;
} edge_t;

typedef struct {
    int deg;
    edge_t nbr[MAX_NEIGHBORS];
} adj_t;

adj_t GRAPH[MAX_NODES];
int NUM_NODES;

/* ==================== SORT ==================== */
typedef struct {
    int id;
    double w;
    double raw;
    long long ts;
} Rank;

int cmp_consensus(const void *a, const void *b) {
    Rank *x = (Rank*)a;
    Rank *y = (Rank*)b;
    if (x->w != y->w)
        return (y->w > x->w) - (y->w < x->w);
    return (x->ts > y->ts) - (x->ts < y->ts);
}

int cmp_leader(const void *a, const void *b) {
    Rank *x = (Rank*)a;
    Rank *y = (Rank*)b;
    if (x->raw != y->raw)
        return (y->raw > x->raw) - (y->raw < x->raw);
    return (x->ts > y->ts) - (x->ts < y->ts);
}

/* ================= NODE ================= */

typedef struct {
    int id;
    int rep;

    char *priv_pem;
    char *pub_pem;

    int seen[MAX_NODES];
    double wvrf[MAX_NODES];
    double raw_vrf[MAX_NODES];
    long long vrf_ts[MAX_NODES];

    int vote_cnt;
} Node;

Node nodes[MAX_NODES];
char *node_priv_pem[MAX_NODES];
char *node_pub_pem[MAX_NODES];

/* ================= EVENT ================= */

typedef struct {
    long long time;
    int from, to, origin;

    char *gamma_hex;
    char *proof_hex;

    double raw;
    double w;
    long long ts;
} Event;

/* ================= EVENT QUEUE ================= */

Event event_q[MAX_EVENTS];
int ev_cnt = 0;
long long now = 0;

/* ================= TIME ================= */

static inline long long now_cpu_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 +
           ts.tv_nsec / 1000000;
}

/* ================= HEAP ================= */

void push_event(Event e) {
    int i = ev_cnt++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (event_q[p].time <= e.time) break;
        event_q[i] = event_q[p];
        i = p;
    }
    event_q[i] = e;
}

Event pop_event() {
    Event ret = event_q[0];
    Event last = event_q[--ev_cnt];

    int i = 0;
    while (1) {
        int l = 2 * i + 1, r = l + 1;
        if (l >= ev_cnt) break;
        int m = (r < ev_cnt && event_q[r].time < event_q[l].time) ? r : l;
        if (event_q[m].time >= last.time) break;
        event_q[i] = event_q[m];
        i = m;
    }
    event_q[i] = last;
    return ret;
}

/* ================= HEX ================= */

char *hex_encode(const unsigned char *buf, size_t len) {
    char *hex = malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++)
        sprintf(hex + 2*i, "%02x", buf[i]);
    return hex;
}

unsigned char *hex_decode(const char *hex, size_t *out_len) {
    size_t len = strlen(hex) / 2;
    unsigned char *buf = malloc(len);
    for (size_t i = 0; i < len; i++)
        sscanf(hex + 2*i, "%2hhx", &buf[i]);
    *out_len = len;
    return buf;
}

/* ================= VRF ================= */

int gen_ec_keypair_pem(char **priv_pem, char **pub_pem) {
    EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    EC_KEY_generate_key(eckey);

    BIO *b_priv = BIO_new(BIO_s_mem());
    BIO *b_pub  = BIO_new(BIO_s_mem());
    BUF_MEM *bptr;

    PEM_write_bio_ECPrivateKey(b_priv, eckey, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_EC_PUBKEY(b_pub, eckey);

    BIO_get_mem_ptr(b_priv, &bptr);
    *priv_pem = strndup(bptr->data, bptr->length);
    BIO_get_mem_ptr(b_pub, &bptr);
    *pub_pem = strndup(bptr->data, bptr->length);

    BIO_free(b_priv);
    BIO_free(b_pub);
    EC_KEY_free(eckey);
    return 1;
}

EC_KEY *load_priv_from_pem(const char *pem) {
    BIO *b = BIO_new_mem_buf((void *)pem, -1);
    EC_KEY *k = PEM_read_bio_ECPrivateKey(b, NULL, NULL, NULL);
    BIO_free(b);
    return k;
}

EVP_PKEY *load_pub_from_pem(const char *pem) {
    BIO *b = BIO_new_mem_buf((void *)pem, -1);
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(b, NULL, NULL, NULL);
    BIO_free(b);
    return pkey;
}

int vrf_prove(int node_id, int round,
              char **gamma_hex, char **proof_hex) {

    char alpha[64];
    sprintf(alpha, "%d-%d", node_id, round);

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)alpha, strlen(alpha), digest);

    *gamma_hex = hex_encode(digest, SHA256_DIGEST_LENGTH);

    EC_KEY *ec = load_priv_from_pem(node_priv_pem[node_id]);
    ECDSA_SIG *sig = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, ec);

    int len = i2d_ECDSA_SIG(sig, NULL);
    unsigned char *buf = malloc(len);
    unsigned char *p = buf;
    i2d_ECDSA_SIG(sig, &p);

    *proof_hex = hex_encode(buf, len);

    free(buf);
    ECDSA_SIG_free(sig);
    EC_KEY_free(ec);
    return 1;
}

int vrf_verify(int node_id, int round,
               const char *gamma_hex,
               const char *proof_hex) {

    char alpha[64];
    sprintf(alpha, "%d-%d", node_id, round);

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)alpha, strlen(alpha), digest);

    size_t g_len;
    unsigned char *g = hex_decode(gamma_hex, &g_len);
    if (memcmp(g, digest, g_len) != 0) { free(g); return 0; }
    free(g);

    size_t p_len;
    unsigned char *p = hex_decode(proof_hex, &p_len);
    const unsigned char *pp = p;
    ECDSA_SIG *sig = d2i_ECDSA_SIG(NULL, &pp, p_len);

    EVP_PKEY *pk = load_pub_from_pem(node_pub_pem[node_id]);
    EC_KEY *ec = EVP_PKEY_get1_EC_KEY(pk);

    int ok = ECDSA_do_verify(digest, SHA256_DIGEST_LENGTH, sig, ec);

    free(p);
    ECDSA_SIG_free(sig);
    EC_KEY_free(ec);
    EVP_PKEY_free(pk);
    return ok == 1;
}

double gamma_to_double(const char *gamma_hex) {
    size_t len;
    unsigned char *b = hex_decode(gamma_hex, &len);
    unsigned long long v = 0;
    for (int i = 0; i < 8 && i < len; i++)
        v = (v << 8) | b[i];
    free(b);
    return (double)v / (double)0xFFFFFFFFFFFFFFFFULL;
}

/* ================= LOAD GRAPH ================= */

void load_graph(const char *file) {
    FILE *f = fopen(file, "r");
    if (!f) { perror("rtt"); exit(1); }

    char line[256];
    int u, v, w;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 3) continue;
        if (sscanf(line, "%d %d %d", &u, &v, &w) == 3) {
            int one_way_rtt = w / 2;
            GRAPH[u].nbr[GRAPH[u].deg++] = (edge_t){v, one_way_rtt};
            GRAPH[v].nbr[GRAPH[v].deg++] = (edge_t){u, one_way_rtt};
        }
    }
    fclose(f);
}

/* ================= MAIN ================= */

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <round> <num_nodes> <rtt_file>\n", argv[0]);
        return 1;
    }

    int ROUND = atoi(argv[1]);
    NUM_NODES = atoi(argv[2]);
    load_graph(argv[3]);

    int TOP_K = 0;
    if (NUM_NODES >= 4 && NUM_NODES <= 20) TOP_K = 4 + NUM_NODES / 5;
    else if (NUM_NODES >= 20) TOP_K = 8 + sqrt(NUM_NODES - 20);

    // printf("=== Node Initialization ===\n");
    for (int i = 0; i < NUM_NODES; i++) {
        nodes[i].id = i;
        nodes[i].rep = 4 + (i % 7);
        nodes[i].vote_cnt = 0;
        memset(nodes[i].seen, 0, sizeof(int)*NUM_NODES);

        gen_ec_keypair_pem(&nodes[i].priv_pem, &nodes[i].pub_pem);
        node_priv_pem[i] = nodes[i].priv_pem;
        node_pub_pem[i]  = nodes[i].pub_pem;

        // printf("[Node %d] rep=%d keypair generated\n", i, nodes[i].rep);
    }

    double total_rep = 0;
    for (int i = 0; i < NUM_NODES; i++) total_rep += nodes[i].rep;

    // printf("\n=== VRF Generation & Initial Broadcast ===\n");

    for (int i = 0; i < NUM_NODES; i++) {
        char *gamma, *proof;
        vrf_prove(i, ROUND, &gamma, &proof);

        long long ts = now_cpu_ms();
        double raw = gamma_to_double(gamma);
        double w = raw * (nodes[i].rep / total_rep);

        nodes[i].seen[i] = 1;
        nodes[i].wvrf[i] = w;
        nodes[i].raw_vrf[i] = raw;
        nodes[i].vrf_ts[i] = ts;
        nodes[i].vote_cnt = 1;

        // printf("[Node %d] raw=%.8f weight=%.4f wvrf=%.8f\n",
        //        i, raw, nodes[i].rep / total_rep, w);

        for (int j = 0; j < GRAPH[i].deg; j++) {
            edge_t e = GRAPH[i].nbr[j];
            push_event((Event){
                .time = VRF_GENERATE_COST + e.rtt,
                .from = i,
                .to = e.to,
                .origin = i,
                .gamma_hex = gamma,
                .proof_hex = proof,
                .raw = raw,
                .w = w,
                .ts = ts
            });
        }
    }

    //printf("\n=== Flooding Broadcast ===\n");

    while (ev_cnt > 0) {
        Event ev = pop_event();
        if (now < ev.time) now = ev.time;

        Node *n = &nodes[ev.to];
        if (n->seen[ev.origin]) continue;
        if (!vrf_verify(ev.origin, ROUND, ev.gamma_hex, ev.proof_hex)) continue;

        n->seen[ev.origin]   = 1;
        n->wvrf[ev.origin]   = ev.w;
        n->raw_vrf[ev.origin]= ev.raw;
        n->vrf_ts[ev.origin] = ev.ts;
        n->vote_cnt++;

        for (int i = 0; i < GRAPH[ev.to].deg; i++) {
            edge_t e = GRAPH[ev.to].nbr[i];
            if (e.to == ev.from) continue;

            push_event((Event){
                .time = now + VRF_VERIFY_COST + e.rtt,
                .from = ev.to,
                .to = e.to,
                .origin = ev.origin,
                .gamma_hex = ev.gamma_hex,
                .proof_hex = ev.proof_hex,
                .raw = ev.raw,
                .w = ev.w,
                .ts = ev.ts
            });
        }
    }

    // printf("\n=== Vote Collection Summary ===\n");
    // for (int i = 0; i < NUM_NODES; i++)
    //     printf("[Node %d] collected votes: %d\n", i, nodes[i].vote_cnt);

    /* === Selection === */
    Rank rank[MAX_NODES];
    for (int i = 0; i < NUM_NODES; i++) {
        rank[i].id = i;
        rank[i].w = nodes[0].wvrf[i];
        rank[i].raw = nodes[0].raw_vrf[i];
        rank[i].ts = nodes[0].vrf_ts[i];
    }

    clock_t start = clock();
    qsort(rank, NUM_NODES, sizeof(Rank), cmp_consensus);
    qsort(rank, TOP_K, sizeof(Rank), cmp_leader);
    clock_t end = clock();

    // int leader = rank[0].id;

    // printf("\n=== Leader Election Result ===\n");
    // printf("Leader: Node %d  rep=%d  wvrf=%.8f\n",
    //        leader, nodes[leader].rep, nodes[0].wvrf[leader]);

    // printf("\n=== Consensus Members (Top-%d) ===\n", TOP_K);
    // for (int i = 0; i < TOP_K; i++) {
    //     printf(" #%02d Node %d  rep=%d  wvrf=%.8f\n",
    //            i + 1,
    //            rank[i].id,
    //            nodes[rank[i].id].rep,
    //            nodes[0].wvrf[rank[i].id]);
    // }

    // printf("\n=== Vote Collection Summary ===\n");
    // for (int i = 0; i < NUM_NODES; i++) {
    //     printf("[Node %d] collected votes: %d\n",
    //            i, nodes[i].vote_cnt);
    // }

    // printf("\n=== Timing Statistics (Simulated) ===\n");
    //printf("Total consensus time: %.3f ms\n", now + (double)(end - start) * 1000.0 / (double)CLOCKS_PER_SEC);
    //printf("communication latency: %lld ms\n", now);
    //printf("sort time cost: %.3f ms\n", (double)(end - start) * 1000.0 / (double)CLOCKS_PER_SEC);
    printf("%.3f\n", now + (double)(end - start) * 1000.0 / (double)CLOCKS_PER_SEC);

    return 0;
}
