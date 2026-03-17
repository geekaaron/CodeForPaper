#include "nonce.h"
#include "global.h"
#include "crypto.h"

EC_KEY *generate_key() {
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!ec_key) return NULL;

    if (EC_KEY_generate_key(ec_key) != 1) {
        EC_KEY_free(ec_key);
        return NULL;
    }
    return ec_key;
}

int sign_hash(EC_KEY *eckey, const uint8_t *hash, uint8_t *sig_raw) {
    ECDSA_SIG *signature = ECDSA_do_sign(hash, HASH_SIZE, eckey);
    if (!signature) return 0;

    const BIGNUM *r = NULL, *s = NULL;
    ECDSA_SIG_get0(signature, &r, &s);

    if (!r || !s) {
        ECDSA_SIG_free(signature);
        return 0;
    }

    BN_bn2binpad(r, sig_raw, HALF_SIGN_SIZE);
    BN_bn2binpad(s, sig_raw + HALF_SIGN_SIZE, HALF_SIGN_SIZE);

    ECDSA_SIG_free(signature);
    return 1;
}

int verify_sig(EC_KEY *eckey, const uint8_t *hash, const uint8_t *nonce, const uint8_t *sig_raw, uint8_t mode) {
    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];

    if (mode == MODE_PBFT) {
        if (is_nonce_used(msg_nonces, msg_nonces_count, nonce)) return 0;
        memcpy(msg_nonces[msg_nonces_count++], nonce, NONCE_SIZE);
    } else if (mode == MODE_RA) {
        if (is_nonce_used(ra_nonces, ra_nonces_count, nonce)) return 0;
        memcpy(ra_nonces[ra_nonces_count++], nonce, NONCE_SIZE);
    }

    memcpy(nonced_buf, nonce, NONCE_SIZE);
    memcpy(nonced_buf + NONCE_SIZE, hash, HASH_SIZE);

    SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

    BIGNUM *r = BN_bin2bn(sig_raw, HALF_SIGN_SIZE, NULL);
    BIGNUM *s = BN_bin2bn(sig_raw + HALF_SIGN_SIZE, HALF_SIGN_SIZE, NULL);
    if (!r || !s) {
        BN_free(r);
        BN_free(s);
        return 0;
    }

    ECDSA_SIG *signature = ECDSA_SIG_new();
    if (!signature) {
        BN_free(r);
        BN_free(s);
        return 0;
    }

    if (!ECDSA_SIG_set0(signature, r, s)) {
        BN_free(r);
        BN_free(s);
        ECDSA_SIG_free(signature);
        return 0;
    }

    int ret = ECDSA_do_verify(nonced_hash, HASH_SIZE, signature, eckey);
    ECDSA_SIG_free(signature);
    return ret == 1;
}

int extract_pubkey_xy(EC_KEY *eckey, uint8_t *x, uint8_t *y) {
    const EC_POINT *point = EC_KEY_get0_public_key(eckey);
    const EC_GROUP *group = EC_KEY_get0_group(eckey);

    BIGNUM *bx = BN_new();
    BIGNUM *by = BN_new();

    int ret = EC_POINT_get_affine_coordinates_GFp(group, point, bx, by, NULL);

    if (!ret) {
        BN_free(bx);
        BN_free(by);
        return 0;
    }

    memset(x, 0, EC_PUBKEY_SIZE);
    memset(y, 0, EC_PUBKEY_SIZE);
    BN_bn2binpad(bx, x, EC_PUBKEY_SIZE);
    BN_bn2binpad(by, y, EC_PUBKEY_SIZE);

    BN_free(bx);
    BN_free(by);
    return 1;
}

int extract_privkey(EC_KEY *eckey, uint8_t *privkey) {
    const EC_GROUP *group = EC_KEY_get0_group(eckey);
    const BIGNUM *priv_key = EC_KEY_get0_private_key(eckey);

    if (BN_bn2binpad(priv_key, privkey, EC_PUBKEY_SIZE) != EC_PUBKEY_SIZE) return 0;

    return 1;
}

EC_KEY* build_pubkey_from_xy(const uint8_t *x, const uint8_t *y) {
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!ec_key) return NULL;

    const EC_GROUP *group = EC_KEY_get0_group(ec_key);
    BIGNUM *bx = BN_bin2bn(x, EC_PUBKEY_SIZE, NULL);
    BIGNUM *by = BN_bin2bn(y, EC_PUBKEY_SIZE, NULL);

    if (!bx || !by) {
        if (bx) BN_free(bx);
        if (by) BN_free(by);
        EC_KEY_free(ec_key);
        return NULL;
    }

    EC_POINT *point = EC_POINT_new(group);
    if (!point) {
        BN_free(bx);
        BN_free(by);
        EC_KEY_free(ec_key);
        return NULL;
    }

    if (!EC_POINT_set_affine_coordinates_GFp(group, point, bx, by, NULL) ||
        !EC_KEY_set_public_key(ec_key, point)) {
        BN_free(bx);
        BN_free(by);
        EC_POINT_free(point);
        EC_KEY_free(ec_key);
        return NULL;
    }

    BN_free(bx);
    BN_free(by);
    EC_POINT_free(point);

    return ec_key;
}

int compute_share_key(const uint8_t *private_key, const uint8_t *public_key_x, const uint8_t *public_key_y, uint8_t *shared_key) {
    EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    BIGNUM *peer_x_bn = BN_bin2bn(public_key_x, EC_PUBKEY_SIZE, NULL);
    BIGNUM *peer_y_bn = BN_bin2bn(public_key_y, EC_PUBKEY_SIZE, NULL);
    EC_POINT *peer_point = EC_POINT_new(group);
    if (!EC_POINT_set_affine_coordinates_GFp(group, peer_point, peer_x_bn, peer_y_bn, NULL)) {
        fprintf(stderr, "Failed to set server public key point\n");
        return -1;
    }

    EC_KEY *client_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    BIGNUM *priv_bn = BN_bin2bn(private_key, EC_PUBKEY_SIZE, NULL);
    if (!EC_KEY_set_private_key(client_key, priv_bn)) {
        fprintf(stderr, "Failed to set private key\n");
        return -1;
    }

    int secret_len = ECDH_compute_key(shared_key, AES_KEY_SIZE, peer_point, client_key, NULL);
    if (secret_len <= 0) {
        fprintf(stderr, "Failed to compute shared secret\n");
        return -1;
    }

    return 0;
}

int aes_encrypt_cbc(const uint8_t *plaintext, int plaintext_size,
                    const uint8_t *key, uint8_t *iv,
                    uint8_t *ciphertext, int *ciphertext_size) {
    EVP_CIPHER_CTX *ctx;
    int len, out_len;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) return -1;

    if (!EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_size)) return -1;
    out_len = len;

    if (!EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) return -1;
    out_len += len;

    *ciphertext_size = out_len;

    EVP_CIPHER_CTX_free(ctx);
    return 0;
}