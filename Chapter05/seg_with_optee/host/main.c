#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <tee_client_api.h>
#include <seg_with_optee_ta.h>

#define PORT        8080
#define BUFFER_SIZE 512
#define SIGN_SIZE   64
#define NONCE_SIZE  16

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

int main(int argc, char *argv[])
{
    TEEC_Result res;
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    TEEC_UUID uuid = TA_SEG_WITH_OPTEE_UUID;
    uint32_t err_origin;

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    uint8_t public_key_x[ECDSA256_KEY_BYTE_SIZE];
    uint8_t public_key_y[ECDSA256_KEY_BYTE_SIZE];
    uint8_t peer_public_key_x[ECDSA256_KEY_BYTE_SIZE];
    uint8_t peer_public_key_y[ECDSA256_KEY_BYTE_SIZE];
    uint8_t ecdh_public_key[ECDH256_KEY_BYTE_SIZE * 2];
    uint8_t iv[AES128_BLOCK_BYTE_SIZE];
    uint8_t params_hash[HASH_SIZE];

    uint8_t exponent[BUFFER_SIZE];
    uint8_t modulus[BUFFER_SIZE];
    uint32_t sig_algo = 0;

    uint8_t ta_shdr[BUFFER_SIZE];
    uint8_t ta_mem[BUFFER_SIZE];
    uint8_t tee_mem[BUFFER_SIZE];
    uint8_t nonce[NONCE_SIZE];

    uint8_t signature[SIGN_SIZE];
    uint8_t *buffer = NULL;
    int psize, rsize, size;
    char *prices = NULL;
    int prices_size;

    clock_t start, end, tstart, tend;

    if (argc < 2) {
        printf("Usage: %s <rainfos|keygen|start>\n", argv[0]);
        return 0;
    }

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

    res = TEEC_OpenSession(&ctx, &sess, &uuid,
                           TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
             res, err_origin);

    if (!strcmp(argv[1], "keygen")) goto KEYGEN;
    if (!strcmp(argv[1], "start")) goto START;

    if (strcmp(argv[1], "rainfos")) {
        printf("Unknown command\n");
        goto EXIT;
    }

// RAINFOS:
    // get remote attestation public key
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_VALUE_OUTPUT, TEEC_NONE);
    op.params[0].tmpref.buffer = exponent;
    op.params[0].tmpref.size = BUFFER_SIZE;
    op.params[1].tmpref.buffer = modulus;
    op.params[1].tmpref.size = BUFFER_SIZE;
    op.params[2].value.a = sig_algo;

    printf("Invoking TA to get remote attestation public key.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_GET_RA_PUBKEY, &op,
                             &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
            res, err_origin);

    print_hex("exponent", exponent, op.params[0].tmpref.size);
    print_hex("modulus", modulus, op.params[1].tmpref.size);

    // get remote attestation digest & signature
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT);

    op.params[0].tmpref.buffer = ta_shdr;
    op.params[0].tmpref.size = BUFFER_SIZE;
    op.params[1].tmpref.buffer = ta_mem;
    op.params[1].tmpref.size = BUFFER_SIZE;
    op.params[2].tmpref.buffer = tee_mem;
    op.params[2].tmpref.size = BUFFER_SIZE;
    op.params[3].tmpref.buffer = nonce;
    op.params[3].tmpref.size = NONCE_SIZE;

    printf("Invoking TA to get remote attestation digest & signature.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_REMOTE_ATTESTATION,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);

    print_hex("SEG TA SHDR", ta_shdr, op.params[0].tmpref.size);
    print_hex("SEG TA MEM", ta_mem, op.params[1].tmpref.size);
    print_hex("SEG TEE MEM", tee_mem, op.params[2].tmpref.size);
    print_hex("SEG NOCNE", nonce, op.params[3].tmpref.size);

    goto EXIT;

KEYGEN:
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE);

    op.params[0].tmpref.buffer = public_key_x;
    op.params[0].tmpref.size = ECDSA256_KEY_BYTE_SIZE;
    op.params[1].tmpref.buffer = public_key_y;
    op.params[1].tmpref.size = ECDSA256_KEY_BYTE_SIZE;

    printf("Invoking TA to generate ECDSA key pair.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_ECDSA_KEYGEN,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
            res, err_origin);

    print_hex("SEG TA ECDSA public key x", public_key_x, op.params[0].tmpref.size);
    print_hex("SEG TA ECDSA public key y", public_key_y, op.params[1].tmpref.size);

    goto EXIT;

START:
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
    safe_recv(client_sock, peer_public_key_x, ECDSA256_KEY_BYTE_SIZE);
    safe_recv(client_sock, peer_public_key_y, ECDSA256_KEY_BYTE_SIZE);

    // build self key handle and peer key handle
    tstart = clock();
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_NONE, TEEC_NONE);

    op.params[0].tmpref.buffer = peer_public_key_x;
    op.params[0].tmpref.size = ECDSA256_KEY_BYTE_SIZE;
    op.params[1].tmpref.buffer = peer_public_key_y;
    op.params[1].tmpref.size = ECDSA256_KEY_BYTE_SIZE;

    printf("Invoking TA to build peer ecdsa key.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_BUILD_KEY_HANDLE,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);

    // receive signature of remote attestation request
    safe_recv(client_sock, nonce, NONCE_SIZE);
    safe_recv(client_sock, signature, SIGN_SIZE);

    // verify signature
    start = clock();
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT);

    op.params[0].value.a = TA_VERIFY_RA;
    op.params[1].tmpref.buffer = nonce;
    op.params[1].tmpref.size = NONCE_SIZE;
    op.params[2].tmpref.buffer = signature;
    op.params[2].tmpref.size = SIGN_SIZE;

    printf("Invoking TA to verify signature of remote attestation.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_VERIFY_SIGNATURE,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);

    // get remote attestation digest & signature
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT);

    op.params[0].tmpref.buffer = ta_shdr;
    op.params[0].tmpref.size = BUFFER_SIZE;
    op.params[1].tmpref.buffer = ta_mem;
    op.params[1].tmpref.size = BUFFER_SIZE;
    op.params[2].tmpref.buffer = tee_mem;
    op.params[2].tmpref.size = BUFFER_SIZE;
    op.params[3].tmpref.buffer = nonce;
    op.params[3].tmpref.size = NONCE_SIZE;

    printf("Invoking TA to get remote attestation digest & signature.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_REMOTE_ATTESTATION,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);
    end = clock();
    printf("remote attestation time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // send remote attestation digest & signature
    send(client_sock, ta_shdr, op.params[0].tmpref.size, 0);
    send(client_sock, ta_mem, op.params[1].tmpref.size, 0);
    send(client_sock, tee_mem, op.params[2].tmpref.size, 0);
    send(client_sock, nonce, op.params[3].tmpref.size, 0);

    // ecdh key exchange
    safe_recv(client_sock, ecdh_public_key, ECDH256_KEY_BYTE_SIZE * 2);
    safe_recv(client_sock, nonce, NONCE_SIZE);
    safe_recv(client_sock, signature, SIGN_SIZE);

    // verify signature
    start = clock();
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT);

    op.params[0].value.a = TA_VERIFY_ECDHKEY;
    op.params[1].tmpref.buffer = nonce;
    op.params[1].tmpref.size = NONCE_SIZE;
    op.params[2].tmpref.buffer = signature;
    op.params[2].tmpref.size = SIGN_SIZE;
    op.params[3].tmpref.buffer = ecdh_public_key;
    op.params[3].tmpref.size = ECDH256_KEY_BYTE_SIZE * 2;

    printf("Invoking TA to verify signature of ECDH keys.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_VERIFY_SIGNATURE,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);

    // ecdh derive key
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE);

    op.params[0].tmpref.buffer = ecdh_public_key;
    op.params[0].tmpref.size = ECDH256_KEY_BYTE_SIZE * 2;
    op.params[1].tmpref.buffer = nonce;
    op.params[1].tmpref.size = NONCE_SIZE;
    op.params[2].tmpref.buffer = signature;
    op.params[2].tmpref.size = SIGN_SIZE;

    printf("Invoking TA to derive shared key.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_ECDH_DERIVE_KEY,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);
    end = clock();
    printf("ecdh key exchange time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // send ecdh public keys, nonce and signature
    send(client_sock, ecdh_public_key, ECDH256_KEY_BYTE_SIZE * 2, 0);
    send(client_sock, nonce, NONCE_SIZE, 0);
    send(client_sock, signature, SIGN_SIZE, 0);

    // receive encrypted game params, nocne and signature
    safe_recv(client_sock, &psize, sizeof(int));
    safe_recv(client_sock, &rsize, sizeof(int));
    safe_recv(client_sock, iv, AES128_BLOCK_BYTE_SIZE);
    safe_recv(client_sock, &size, sizeof(int));
    buffer = malloc(size * sizeof(uint8_t));
    safe_recv(client_sock, buffer, size);

    safe_recv(client_sock, nonce, NONCE_SIZE);
    safe_recv(client_sock, signature, SIGN_SIZE);

    // verify signature
    start = clock();
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT);

    op.params[0].value.a = TA_VERIFY_CIPHER;
    op.params[1].tmpref.buffer = nonce;
    op.params[1].tmpref.size = NONCE_SIZE;
    op.params[2].tmpref.buffer = signature;
    op.params[2].tmpref.size = SIGN_SIZE;
    op.params[3].tmpref.buffer = buffer;
    op.params[3].tmpref.size = size;

    printf("Invoking TA to verify signature of ciphertext.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_VERIFY_SIGNATURE,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);

    // aes decrypt
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_VALUE_INPUT,
                                     TEEC_VALUE_INPUT);

    op.params[0].tmpref.buffer = iv;
    op.params[0].tmpref.size = AES128_BLOCK_BYTE_SIZE;
    op.params[1].tmpref.buffer = buffer;
    op.params[1].tmpref.size = size;
    op.params[2].value.a = psize;
    op.params[3].value.a = rsize;

    printf("Invoking TA to decrypt aes256 cbc encrypted game params\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_AES_DECRYPT, &op,
                 &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
            res, err_origin);
    end = clock();
    printf("aes decryption time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // get game params info
    start = clock();
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE);

    op.params[0].tmpref.buffer = params_hash;
    op.params[0].tmpref.size = HASH_SIZE;
    op.params[1].tmpref.buffer = nonce;
    op.params[1].tmpref.size = NONCE_SIZE;
    op.params[2].tmpref.buffer = signature;
    op.params[2].tmpref.size = SIGN_SIZE;

    printf("Invoking TA to get game params hash, nonce and signature\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_GET_PARAMS_INFO, &op,
                 &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
            res, err_origin);

    // upload game params info
    if (upload_game_params_info(params_hash, nonce, signature) != 0) {
        fprintf(stderr, "Upload game params info failed!\n");
        goto EXIT;
    }
    end = clock();
    printf("upload game params time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    // start pricing game
    prices_size = psize * PER_PRICE_STR_SIZE * sizeof(char);
    prices = malloc(prices_size);
    memset(prices, 0, prices_size);

    start = clock();
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE);

    op.params[0].tmpref.buffer = prices;
    op.params[0].tmpref.size = prices_size;
    op.params[1].tmpref.buffer = nonce;
    op.params[1].tmpref.size = NONCE_SIZE;
    op.params[2].tmpref.buffer = signature;
    op.params[2].tmpref.size = SIGN_SIZE;

    printf("Invoking TA to start pricing game.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_WITH_OPTEE_CMD_START_PRICING_GAME,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG) failed 0x%x origin 0x%x",
             res, err_origin);
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
    free(buffer);
    free(prices);
    close(server_sock);
    close(client_sock);
    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);

    return 0;
}
