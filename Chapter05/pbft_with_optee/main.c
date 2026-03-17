#include "utils.h"
#include "json.h"
#include "message.h"
#include "network.h"
#include "global.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s NODE_ID REPUTATION\n", argv[0]);
        return -1;
    }

    node_id = atoi(argv[1]);
    reputation = atoi(argv[2]);

    for (int i = 0; i < MAX_NODES; i++) {
        nodes[i].id = i;
        nodes[i].reputation = 0;
    }
    nodes[node_id].reputation = reputation;

    uint8_t exponent[EXPONENT_SIZE] = {0x01, 0x00, 0x01};

    char *teeInfoUrl = "http://192.168.31.2:48080/GameController/getElectedTeeInfo";
    cJSON *json = json_from_url(teeInfoUrl);
    if (!json) {
        fprintf(stderr, "Failed to get JSON from URL.\n");
        return -1;
    }

    if (json_to_tee_struct(json, &tinfo) != 0) {
        fprintf(stderr, "Failed to parse JSON to structs.\n");
        cJSON_Delete(json);
        return -1;
    }

    tee_key = generate_rsa_key(tinfo.ra_public_key, exponent);

    local_privkey = generate_key();
    if (!local_privkey) {
        printf("Generate EC key failed!\n");
        return -1;
    }

    if (!extract_pubkey_xy(local_privkey, local_pubkey_x, local_pubkey_y)) {
        printf("Extract public key failed!\n");
        EC_KEY_free(local_privkey);
        return -1;
    }

    printf("[Node %d] start, reputation = %d\n", node_id, reputation);

    pthread_t tid;
    pthread_create(&tid, NULL, server_thread, NULL);

    sleep(2);

    PBFTMessage rep_msg;
    memset(&rep_msg, 0, sizeof(rep_msg));
    strcpy(rep_msg.type, "REPUTE");
    rep_msg.from = node_id;
    rep_msg.reputation = reputation;

    char rep_str[64];
    snprintf(rep_str, sizeof(rep_str), "%d", reputation);
    SHA256((uint8_t *)rep_str, strlen(rep_str), params_hash);
    memcpy(rep_msg.params_hash, params_hash, HASH_SIZE);

    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];

    RAND_bytes(rep_msg.params_nonce, NONCE_SIZE);
    memcpy(msg_nonces[msg_nonces_count++], rep_msg.params_nonce, NONCE_SIZE);

    memcpy(nonced_buf, rep_msg.params_nonce, NONCE_SIZE);
    memcpy(nonced_buf + NONCE_SIZE, rep_msg.params_hash, HASH_SIZE);
    SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

    sign_hash(local_privkey, nonced_hash, rep_msg.params_signature);
    extract_pubkey_xy(local_privkey, rep_msg.pubkey_x, rep_msg.pubkey_y);

    broadcast(&rep_msg);
    pthread_join(tid, NULL);

    EC_KEY_free(local_privkey);
    EVP_PKEY_free(tee_key);
    if (buffer) free(buffer);

    return 0;
}
