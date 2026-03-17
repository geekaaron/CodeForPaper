#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "include/nonce.h"
#include "include/crypto.h"
#include "include/global.h"
#include "include/game.h"

#define PORT        8080
#define BUFFER_SIZE 512

void print_hex(const char *lable, const uint8_t *buf, size_t len) {
    printf("%s: ", lable);
    for (size_t i = 0; i < len; i++) printf("%02x", buf[i]);
    printf("\n");
}

char *bytes_to_hex(const uint8_t *bytes, size_t len) {
    if (!bytes || len == 0) return NULL;
    char *hexstr = malloc(len * 2 + 1);
    if (!hexstr) return NULL;
    for (size_t i = 0; i < len; ++i) sprintf(hexstr + 2*i, "%02x", bytes[i]);
    hexstr[len * 2] = '\0';
    return hexstr;
}

void safe_recv(int sock, uint8_t *buffer, int size) {
    int received = 0;
    while (received < size) {
        int n = recv(sock, buffer + received, size - received, 0);
        if (n <= 0) {
            err(1, "recv failed or connection closed");
        }
        received += n;
    }
}

int upload_game_params_info(const uint8_t *hash, const uint8_t nonce[NONCE_SIZE],
                            const uint8_t signature[SIGN_SIZE]) {
    CURL *curl;
    CURLcode res;
    int ret = -1;

    char *hash_hex = bytes_to_hex(hash, HASH_SIZE);
    char *nonce_hex = bytes_to_hex(nonce, NONCE_SIZE);
    char *sig_hex = bytes_to_hex(signature, SIGN_SIZE);

    char url[2048];
    snprintf(url, sizeof(url),
             "http://192.168.31.2:48080/GameController/uploadGameParamsInfo?hashHex=%s&nonceHex=%s&signatureHex=%s",
             hash_hex, nonce_hex, sig_hex);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Game params info result uploaded to %s\n", url);
            ret = 0;
        }

        curl_easy_cleanup(curl);
    }

    free(hash_hex);
    free(nonce_hex);
    free(sig_hex);

    return ret;
}

int upload_game_result(const char *prices, const uint8_t nonce[NONCE_SIZE],
                       const uint8_t signature[SIGN_SIZE]) {
    CURL *curl;
    CURLcode res;

    char *nonce_hex = bytes_to_hex(nonce, NONCE_SIZE);
    char *sig_hex = bytes_to_hex(signature, SIGN_SIZE);

    size_t json_capacity = strlen(prices) + strlen(nonce_hex) + strlen(sig_hex) + 100;
    char *json_data = malloc(json_capacity);
    if (!json_data) return -1;

    snprintf(json_data, json_capacity,
             "{\"optimalPrices\":\"%s\",\"nonceHex\":\"%s\",\"signatureHex\":\"%s\"}",
             prices, nonce_hex, sig_hex);

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.31.2:48080/GameController/uploadGameResult");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    free(json_data);
    return 0;
}

int main(void)
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    EC_KEY *ecdh_key = NULL;

    uint8_t public_key_x[EC_PUBKEY_SIZE];
    uint8_t public_key_y[EC_PUBKEY_SIZE];
    uint8_t private_key[EC_PUBKEY_SIZE];
    uint8_t shared_key[AES_KEY_SIZE];

    uint8_t peer_public_key_x[EC_PUBKEY_SIZE];
    uint8_t peer_public_key_y[EC_PUBKEY_SIZE];
    uint8_t public_key_buffer[EC_PUBKEY_SIZE * 2];
    uint8_t hash[HASH_SIZE];

    uint8_t nonce[NONCE_SIZE];
    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];
    uint8_t signature[SIGN_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    uint8_t *ciphertext = NULL;
    uint8_t *plaintext = NULL;
    int ciphertext_size, plaintext_size;

    int received, n;

    int psize, rsize, size, prices_size;
    char *prices = NULL;

    clock_t start, end, tstart, tend;

    // server socket start
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
        err(1, "Failed to create socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        err(1, "Failed to bind");

    if (listen(server_sock, 5) < 0)
        err(1, "Failed to listen");

    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0)
        err(1, "Failed to accept client connection");

    // receive peer public key x, peer public key y
    safe_recv(client_sock, peer_public_key_x, EC_PUBKEY_SIZE);
    safe_recv(client_sock, peer_public_key_y, EC_PUBKEY_SIZE);

    // build self key handle and peer key handle
    tstart = clock();
    self_key = generate_key();
    peer_key = build_pubkey_from_xy(peer_public_key_x, peer_public_key_y);

    // send self public key to peer
    extract_pubkey_xy(self_key, public_key_x, public_key_y);
    send(client_sock, public_key_x, EC_PUBKEY_SIZE, 0);
    send(client_sock, public_key_y, EC_PUBKEY_SIZE, 0);

    // receive peer ecdh public key
    safe_recv(client_sock, public_key_buffer, EC_PUBKEY_SIZE * 2);
    safe_recv(client_sock, nonce, NONCE_SIZE);
    safe_recv(client_sock, signature, SIGN_SIZE);

    // verify signature
    start = clock();
    SHA256(public_key_buffer, EC_PUBKEY_SIZE * 2, hash);
    if (!verify_sig(peer_key, hash, nonce, signature)) {
        fprintf(stderr, "Verify ECDH keys signature failed!\n");
        goto EXIT;
    }

    // ecdh key derive
    ecdh_key = generate_key();
    memcpy(peer_public_key_x, public_key_buffer, EC_PUBKEY_SIZE);
    memcpy(peer_public_key_y, public_key_buffer + EC_PUBKEY_SIZE, EC_PUBKEY_SIZE);
    extract_privkey(ecdh_key, private_key);
    if (compute_share_key(private_key, peer_public_key_x, peer_public_key_y, shared_key) != 0) goto EXIT;

    do {
        RAND_bytes(nonce, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, nonce));
    memcpy(nonces[nonces_count++], nonce, NONCE_SIZE);

    extract_pubkey_xy(ecdh_key, public_key_x, public_key_y);
    memcpy(public_key_buffer, public_key_x, EC_PUBKEY_SIZE);
    memcpy(public_key_buffer + EC_PUBKEY_SIZE, public_key_y, EC_PUBKEY_SIZE);
    SHA256(public_key_buffer, EC_PUBKEY_SIZE * 2, hash);

    memcpy(nonced_buf, nonce, NONCE_SIZE);
    memcpy(nonced_buf + NONCE_SIZE, hash, HASH_SIZE);
    SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);
    sign_hash(self_key, nonced_hash, signature);
    end = clock();
    printf("ecdh key exchange time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // send
    send(client_sock, public_key_buffer, EC_PUBKEY_SIZE * 2, 0);
    send(client_sock, nonce, NONCE_SIZE, 0);
    send(client_sock, signature, SIGN_SIZE, 0);

    // receive encrypted game params, nocne and signature
    safe_recv(client_sock, &psize, sizeof(int));
    safe_recv(client_sock, &rsize, sizeof(int));
    safe_recv(client_sock, iv, AES_BLOCK_SIZE);
    safe_recv(client_sock, &ciphertext_size, sizeof(int));
    ciphertext = malloc(ciphertext_size * sizeof(uint8_t));
    safe_recv(client_sock, ciphertext, ciphertext_size);

    safe_recv(client_sock, nonce, NONCE_SIZE);
    safe_recv(client_sock, signature, SIGN_SIZE);

    // verify signature
    start = clock();
    SHA256(ciphertext, ciphertext_size, hash);
    if (!verify_sig(peer_key, hash, nonce, signature)) {
        fprintf(stderr, "Verify ciphertext signature failed!\n");
        goto EXIT;
    }

    // aes decrypt
    plaintext = malloc(ciphertext_size * sizeof(uint8_t));
    plaintext_size = ciphertext_size;
    aes_decrypt_cbc(ciphertext, ciphertext_size, shared_key, iv, plaintext, &plaintext_size);
    load_game_params(plaintext, psize, rsize);
    end = clock();
    printf("aes decryption time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // get game params info
    start = clock();
    get_game_params_info(nonce, signature);

    // upload game params info to blockchain
    if (upload_game_params_info(params_hash, nonce, signature) != 0) {
        fprintf(stderr, "Upload game result failed!\n");
        goto EXIT;
    }
    end = clock();
    printf("upload game params time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // start pricing game
    prices_size = psize * PER_PRICE_STR_SIZE * sizeof(char);
    prices = malloc(prices_size);
    memset(prices, 0, prices_size);

    start = clock();
    noncooperative_game(prices, nonce, signature);
    end = clock();
    printf("seg game execution time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    start = clock();
    if (upload_game_result(prices, nonce, signature) != 0) {
        fprintf(stderr, "Upload game result failed!\n");
        goto EXIT;
    }
    end = clock();
    printf("upload game result time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    tend = clock();
    printf("total time: %.3f s\n", (double)(tend - tstart) / CLOCKS_PER_SEC);

    printf("optimal prices: %s\n", prices);
    print_hex("nonce", nonce, NONCE_SIZE);
    print_hex("signature", signature, SIGN_SIZE);

EXIT:
    if (ciphertext != NULL) free(ciphertext);
    if (plaintext != NULL) free(plaintext);
    if (prices != NULL) free(prices);
    close(server_sock);
    close(client_sock);

    return 0;
}
