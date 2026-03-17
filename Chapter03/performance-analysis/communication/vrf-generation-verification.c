#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/obj_mac.h>

static inline long long now_cpu_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static char *hex_encode(const unsigned char *buf, size_t len) {
    char *hex = (char*)malloc(len * 2 + 1);
    if (!hex) return NULL;
    for (size_t i = 0; i < len; i++) sprintf(hex + 2*i, "%02x", buf[i]);
    hex[len * 2] = '\0';
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

static int gen_ec_keypair_pem(char **priv_pem, char **pub_pem) {
    int ok = 0;
    EC_KEY *eckey = NULL;
    BIO *b_priv = NULL, *b_pub = NULL;
    BUF_MEM *bptr = NULL;

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
    if (!ok) {
        free(*priv_pem); *priv_pem = NULL;
        free(*pub_pem);  *pub_pem  = NULL;
    }
    if (b_priv) BIO_free(b_priv);
    if (b_pub)  BIO_free(b_pub);
    if (eckey)  EC_KEY_free(eckey);
    return ok;
}

static EC_KEY *load_priv_from_pem(const char *pem) {
    BIO *b = BIO_new_mem_buf((void *)pem, -1);
    if (!b) return NULL;
    EC_KEY *k = PEM_read_bio_ECPrivateKey(b, NULL, NULL, NULL);
    BIO_free(b);
    return k;
}

static EVP_PKEY *load_pub_from_pem(const char *pem) {
    BIO *b = BIO_new_mem_buf((void *)pem, -1);
    if (!b) return NULL;
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(b, NULL, NULL, NULL);
    BIO_free(b);
    return pkey;
}

static int vrf_prove(const char *priv_pem,
                     int node_id,
                     int round,
                     char **gamma_hex,
                     char **proof_hex) {
    int ok = 0;
    EC_KEY *ec = NULL;
    ECDSA_SIG *sig = NULL;
    unsigned char digest[SHA256_DIGEST_LENGTH];

    char alpha[64];
    snprintf(alpha, sizeof(alpha), "%d-%d", node_id, round);
    SHA256((unsigned char*)alpha, strlen(alpha), digest);

    *gamma_hex = hex_encode(digest, SHA256_DIGEST_LENGTH);
    if (!*gamma_hex) goto done;

    ec = load_priv_from_pem(priv_pem);
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
    if (!ok) {
        free(*gamma_hex); *gamma_hex = NULL;
        free(*proof_hex); *proof_hex = NULL;
    }
    if (sig) ECDSA_SIG_free(sig);
    if (ec)  EC_KEY_free(ec);
    return ok;
}

static int vrf_verify(const char *pub_pem,
                      int node_id,
                      int round,
                      const char *gamma_hex,
                      const char *proof_hex) {
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

    EVP_PKEY *pk = load_pub_from_pem(pub_pem);
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

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <block_height> <N>\n", argv[0]);
        return 1;
    }

    int round = atoi(argv[1]);
    int N = atoi(argv[2]);
    int node_id = 0;

    if (N <= 0) {
        fprintf(stderr, "N must be > 0\n");
        return 1;
    }

    char *priv_pem = NULL, *pub_pem = NULL;
    if (!gen_ec_keypair_pem(&priv_pem, &pub_pem)) {
        fprintf(stderr, "Keypair generation failed\n");
        return 1;
    }

    long long prove_ns_sum = 0;
    long long verify_ns_sum = 0;

    for (int i = 0; i < N; i++) {
        char *gamma_hex = NULL;
        char *proof_hex = NULL;

        /* === VRF prove === */
        long long t0 = now_cpu_ns();
        if (!vrf_prove(priv_pem, node_id, round, &gamma_hex, &proof_hex)) {
            fprintf(stderr, "vrf_prove failed\n");
            return 1;
        }
        long long t1 = now_cpu_ns();
        prove_ns_sum += (t1 - t0);

        /* === VRF verify === */
        long long t2 = now_cpu_ns();
        int ok = vrf_verify(pub_pem, node_id, round, gamma_hex, proof_hex);
        long long t3 = now_cpu_ns();
        verify_ns_sum += (t3 - t2);

        if (!ok) {
            fprintf(stderr, "vrf_verify failed at iter %d\n", i);
            return 1;
        }

        free(gamma_hex);
        free(proof_hex);
    }

    double avg_prove_ms  = (double)prove_ns_sum  / (double)N / 1e6;
    double avg_verify_ms = (double)verify_ns_sum / (double)N / 1e6;

    printf("round = %d\n", round);
    printf("N = %d\n", N);
    printf("avg_prove = %.3f ms\n", avg_prove_ms);
    printf("avg_verify = %.3f ms\n", avg_verify_ms);

    free(priv_pem);
    free(pub_pem);
    return 0;
}

