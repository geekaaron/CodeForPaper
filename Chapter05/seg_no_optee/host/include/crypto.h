#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/rand.h>
#include <openssl/obj_mac.h>

#define HASH_SIZE 32
#define AES_KEY_SIZE 32
#define AES_BLOCK_SIZE 16
#define EC_PUBKEY_SIZE 32
#define SIGN_SIZE 64
#define HALF_SIGN_SIZE (SIGN_SIZE / 2)
#define RA_SIZE 416
#define RA_SIGN_SIZE (RA_SIZE - HASH_SIZE)

EC_KEY *generate_key();
int sign_hash(EC_KEY *eckey, const uint8_t *hash, uint8_t *sig_raw);
int verify_sig(EC_KEY *eckey, const uint8_t *hash, const uint8_t *nonce, const uint8_t *sig_raw);
int extract_pubkey_xy(EC_KEY *eckey, uint8_t *x, uint8_t *y);
int extract_privkey(EC_KEY *eckey, uint8_t *privkey);
EC_KEY* build_pubkey_from_xy(const uint8_t *x, const uint8_t *y);
int compute_share_key(const uint8_t *private_key, const uint8_t *public_key_x, const uint8_t *public_key_y, uint8_t *shared_key);
int aes_decrypt_cbc(const uint8_t *ciphertext, int ciphertext_size,
                    const uint8_t *key, uint8_t *iv,
                    uint8_t *plaintext, int *plaintext_size);

#endif