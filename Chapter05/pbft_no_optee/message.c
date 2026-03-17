#include <pthread.h>
#include <curl/curl.h>

#include "network.h"
#include "json.h"
#include "crypto.h"
#include "nonce.h"
#include "utils.h"
#include "global.h"
#include "message.h"

#define PEER_PORT  8080
#define PORT_BASE   8000

#define F ((MAX_NODES-1)/3)
#define PREPARE_THRESHOLD   (2*F+1)
#define COMMIT_THRESHOLD    (2*F+1)

int peer_sock;
struct sockaddr_in server_addr;

ProvidersInfo pinfo;
RequestersInfo rinfo;

uint8_t local_pubkey_x[EC_PUBKEY_SIZE];
uint8_t local_pubkey_y[EC_PUBKEY_SIZE];

int node_id, reputation;
int leader_id = -1;
Node nodes[MAX_NODES];

uint8_t *buffer = NULL;
uint8_t params_hash[HASH_SIZE];
uint8_t last_hash[HASH_SIZE];
size_t size = 0;

int prepare_count = 0;
int commit_count = 0;

int rep_collected = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

EVP_PKEY *tee_key;
EC_KEY *local_privkey;
EC_KEY *peer_pubkey;

uint8_t ra_hash[HASH_SIZE] = {
    0x71, 0xbb, 0xb3, 0x50, 0xaa, 0x73, 0xe9, 0x65,
    0xcf, 0x72, 0x7a, 0xd7, 0x30, 0x51, 0xba, 0x7d,
    0x60, 0xdc, 0x29, 0x83, 0x26, 0xc2, 0x41, 0x9b,
    0x1b, 0x3c, 0x9b, 0x8f, 0x5f, 0xd6, 0x1c, 0x13
};

uint8_t msg_nonces[NONCE_COUNT][NONCE_SIZE];
size_t msg_nonces_count = 0;

uint8_t ra_nonces[NONCE_COUNT][NONCE_SIZE];
size_t ra_nonces_count = 0;

uint8_t nonce[NONCE_SIZE];
uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
uint8_t nonced_hash[HASH_SIZE];
uint8_t signature[SIGN_SIZE];

int upload_consensus_result(const uint8_t *hash, const uint8_t *nonce,
                            const uint8_t *signature, int node_index) {
    CURL *curl;
    CURLcode res;
    int ret = -1;

    char *hash_hex = bytes_to_hex(hash, HASH_SIZE);
    char *nonce_hex = bytes_to_hex(nonce, NONCE_SIZE);
    char *sig_hex = bytes_to_hex(signature, SIGN_SIZE);

    char url[2048];
    snprintf(url, sizeof(url),
             "http://192.168.31.2:48080/GameController/uploadConsensusResult?hashHex=%s&nonceHex=%s&signatureHex=%s&nodeIndex=%d",
             hash_hex, nonce_hex, sig_hex, node_index);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Consensus result uploaded to %s\n", url);
            ret = 0;
        }

        curl_easy_cleanup(curl);
    }

    free(hash_hex);
    free(nonce_hex);
    free(sig_hex);

    return ret;
}

int load_msg_buffer(uint8_t **buffer, size_t *size) {
    const char *url = "http://192.168.31.2:48080/GameController/getGameParams";

    cJSON *json = json_from_url(url);
    if (!json) {
        fprintf(stderr, "Failed to get JSON from URL.\n");
        return -1;
    }

    if (json_to_params_struct(json, &pinfo, &rinfo) != 0) {
        fprintf(stderr, "Failed to parse JSON to structs.\n");
        cJSON_Delete(json);
        return -1;
    }
    cJSON_Delete(json);

    *size = (pinfo.size * PROVIDER_PARAMS_NUM + rinfo.size * REQUESTER_PARAMS_NUM) * sizeof(double);
    *buffer = malloc(*size);

    int current = 0;
    memcpy(*buffer + current, pinfo.prices, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(*buffer + current, pinfo.amounts, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(*buffer + current, pinfo.reputations, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(*buffer + current, pinfo.costs, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);

    memcpy(*buffer + current, rinfo.lambda1s, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    memcpy(*buffer + current, rinfo.lambda2s, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    memcpy(*buffer + current, rinfo.sensitivities, sizeof(double) * rinfo.size);

    return 0;
}

void send_end_msg() {
    char *end_msg = "end";

    PBFTMessage end;
    memset(&end, 0, sizeof(end));
    strcpy(end.type, "END");
    end.from = node_id;
    end.reputation = reputation;

    SHA256(end_msg, strlen(end_msg), end.params_hash);

    do {
        RAND_bytes(end.params_nonce, NONCE_SIZE);
    } while(is_nonce_used(msg_nonces, msg_nonces_count, end.params_nonce));

    memcpy(msg_nonces[msg_nonces_count++], end.params_nonce, NONCE_SIZE);

    memcpy(nonced_buf, end.params_nonce, NONCE_SIZE);
    memcpy(nonced_buf + NONCE_SIZE, end.params_hash, HASH_SIZE);
    SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

    sign_hash(local_privkey, nonced_hash, end.params_signature);
    extract_pubkey_xy(local_privkey, end.pubkey_x, end.pubkey_y);

    printf("[Leader Node %d] Broadcasting END: ", node_id);
    print_hex(params_hash, HASH_SIZE);

    memcpy(last_hash, params_hash, HASH_SIZE);
    prepare_count = 1;
    commit_count = 0;

    broadcast(&end);
}

void handle_msg(PBFTMessage *msg) {
    EC_KEY *msg_pubkey = build_pubkey_from_xy(msg->pubkey_x, msg->pubkey_y);
    if (!msg_pubkey) {
        printf("[Node %d] Rebuild EC public key failed!\n", node_id);
        return;
    }

    if (strcmp(msg->type, "END") == 0 && msg->from == leader_id) {
        if (!verify_sig(msg_pubkey, msg->params_hash, msg->params_nonce, msg->params_signature, MODE_PBFT)) {
            printf("[Node %d] Signature verification failed, message type: %s, from node %d.\n", node_id, msg->type, msg->from);
            EC_KEY_free(msg_pubkey);
            return;
        }
        EC_KEY_free(msg_pubkey);
        exit(0);
    }

    if (strcmp(msg->type, "REPUTE") == 0) {
        char rep_str[64];
        snprintf(rep_str, sizeof(rep_str), "%d", msg->reputation);
        uint8_t rep_hash[HASH_SIZE];
        SHA256((uint8_t *)rep_str, strlen(rep_str), rep_hash);

        int valid = verify_sig(msg_pubkey, rep_hash, msg->params_nonce, msg->params_signature, MODE_PBFT);
        if (!valid) {
            printf("[Node %d] REPUTE Signature verification failed, from node %d.\n", node_id, msg->from);
            EC_KEY_free(msg_pubkey);
            return;
        }

        pthread_mutex_lock(&lock);
        nodes[msg->from].reputation = msg->reputation;
        rep_collected++;
        if (rep_collected == MAX_NODES - 1) {
            int max_rep = reputation;
            leader_id = node_id;
            for (int i = 0; i < MAX_NODES; i++) {
                if (i != node_id && nodes[i].reputation > max_rep) {
                    max_rep = nodes[i].reputation;
                    leader_id = i;
                }
            }
            printf("[Node %d] Leader elected: Node %d\n", node_id, leader_id);

            if (leader_id == node_id) {
                if (load_msg_buffer(&buffer, &size) != 0) return;
                SHA256(buffer, size, params_hash);

                peer_sock = socket(AF_INET, SOCK_STREAM, 0);
                if (peer_sock < 0) {
                    perror("Socket creation failed");
                    pthread_mutex_unlock(&lock);
                    EC_KEY_free(msg_pubkey);
                    send_end_msg();
                    sleep(1);
                    exit(0);
                }

                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(PEER_PORT);
                if (inet_pton(AF_INET, "192.168.31.3", &server_addr.sin_addr) <= 0) {
                    perror("Invalid address or address not supported");
                    close(peer_sock);
                    pthread_mutex_unlock(&lock);
                    EC_KEY_free(msg_pubkey);
                    send_end_msg();
                    sleep(1);
                    exit(0);
                }

                if (connect(peer_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    perror("Connection failed");
                    close(peer_sock);
                    pthread_mutex_unlock(&lock);
                    EC_KEY_free(msg_pubkey);
                    send_end_msg();
                    sleep(1);
                    exit(0);
                }

                // send leader's public key
                send(peer_sock, local_pubkey_x, EC_PUBKEY_SIZE, 0);
                send(peer_sock, local_pubkey_y, EC_PUBKEY_SIZE, 0);

                PBFTMessage prep;
                memset(&prep, 0, sizeof(prep));
                strcpy(prep.type, "PRE-PREPARE");
                prep.from = node_id;
                prep.reputation = reputation;

                memcpy(prep.params_hash, params_hash, HASH_SIZE);

                do {
                    RAND_bytes(prep.params_nonce, NONCE_SIZE);
                } while(is_nonce_used(msg_nonces, msg_nonces_count, prep.params_nonce));

                memcpy(msg_nonces[msg_nonces_count++], prep.params_nonce, NONCE_SIZE);

                memcpy(nonced_buf, prep.params_nonce, NONCE_SIZE);
                memcpy(nonced_buf + NONCE_SIZE, prep.params_hash, HASH_SIZE);
                SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

                sign_hash(local_privkey, nonced_hash, prep.params_signature);
                extract_pubkey_xy(local_privkey, prep.pubkey_x, prep.pubkey_y);

                printf("[Leader Node %d] Broadcasting PRE-PREPARE: ", node_id);
                print_hex(params_hash, HASH_SIZE);

                memcpy(last_hash, params_hash, HASH_SIZE);
                prepare_count = 1;
                commit_count = 0;

                broadcast(&prep);
            }
        }
        pthread_mutex_unlock(&lock);

        EC_KEY_free(msg_pubkey);
        return;
    }
    
    if (!verify_sig(msg_pubkey, msg->params_hash, msg->params_nonce, msg->params_signature, MODE_PBFT)) {
        printf("[Node %d] Signature verification failed, message type: %s, from node %d.\n", node_id, msg->type, msg->from);
        EC_KEY_free(msg_pubkey);
        return;
    }
    EC_KEY_free(msg_pubkey);

    if (strcmp(msg->type, "PRE-PREPARE") == 0 && msg->from == leader_id) {
        printf("[Node %d] Received PRE-PREPARE from Leader. Hash: ", node_id);
        print_hex(msg->params_hash, HASH_SIZE);

        if (load_msg_buffer(&buffer, &size) != 0) return;
        SHA256(buffer, size, params_hash);
        if (memcmp(params_hash, msg->params_hash, HASH_SIZE) != 0) return;

        pthread_mutex_lock(&lock);
        memcpy(last_hash, msg->params_hash, HASH_SIZE);
        pthread_mutex_unlock(&lock);

        PBFTMessage prepare_msg;
        memset(&prepare_msg, 0, sizeof(prepare_msg));
        strcpy(prepare_msg.type, "PREPARE");
        prepare_msg.from = node_id;
        prepare_msg.reputation = reputation;
        memcpy(prepare_msg.params_hash, msg->params_hash, HASH_SIZE);

        do {
            RAND_bytes(prepare_msg.params_nonce, NONCE_SIZE);
        } while(is_nonce_used(msg_nonces, msg_nonces_count, prepare_msg.params_nonce));

        memcpy(msg_nonces[msg_nonces_count++], prepare_msg.params_nonce, NONCE_SIZE);

        memcpy(nonced_buf, prepare_msg.params_nonce, NONCE_SIZE);
        memcpy(nonced_buf + NONCE_SIZE, prepare_msg.params_hash, HASH_SIZE);
        SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

        sign_hash(local_privkey, nonced_hash, prepare_msg.params_signature);
        extract_pubkey_xy(local_privkey, prepare_msg.pubkey_x, prepare_msg.pubkey_y);
        broadcast(&prepare_msg);

        pthread_mutex_lock(&lock);
        prepare_count = 1;
        pthread_mutex_unlock(&lock);
    } else if (strcmp(msg->type, "PREPARE") == 0) {
        pthread_mutex_lock(&lock);
        if (memcmp(last_hash, msg->params_hash, HASH_SIZE) == 0) {
            prepare_count++;
            printf("[Node %d] Received PREPARE from Node %d. Count=%d\n", node_id, msg->from, prepare_count);
            if (prepare_count >= PREPARE_THRESHOLD && node_id != leader_id) {
                PBFTMessage commit_msg;
                memset(&commit_msg, 0, sizeof(commit_msg));
                strcpy(commit_msg.type, "COMMIT");
                commit_msg.from = node_id;
                commit_msg.reputation = reputation;
                memcpy(commit_msg.params_hash, msg->params_hash, HASH_SIZE);

                do {
                    RAND_bytes(commit_msg.params_nonce, NONCE_SIZE);
                } while(is_nonce_used(msg_nonces, msg_nonces_count, commit_msg.params_nonce));

                memcpy(msg_nonces[msg_nonces_count++], commit_msg.params_nonce, NONCE_SIZE);

                memcpy(nonced_buf, commit_msg.params_nonce, NONCE_SIZE);
                memcpy(nonced_buf + NONCE_SIZE, commit_msg.params_hash, HASH_SIZE);
                SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

                sign_hash(local_privkey, nonced_hash, commit_msg.params_signature);
                extract_pubkey_xy(local_privkey, commit_msg.pubkey_x, commit_msg.pubkey_y);
                broadcast(&commit_msg);

                commit_count = 1;
                printf("[Node %d] PREPARE threshold reached, broadcasting COMMIT\n", node_id);
                exit(0); // non leader nodes end.
            }
        }
        pthread_mutex_unlock(&lock);
    } else if (strcmp(msg->type, "COMMIT") == 0) {
        pthread_mutex_lock(&lock);
        if (memcmp(last_hash, msg->params_hash, HASH_SIZE) == 0) {
            commit_count++;
            printf("[Node %d] Received COMMIT from Node %d. Count=%d\n", node_id, msg->from, commit_count);
            if (commit_count >= COMMIT_THRESHOLD && leader_id == node_id) {
                printf("[Leader %d] COMMIT threshold reached. Consensus achieved on hash: ", node_id);
                print_hex(msg->params_hash, HASH_SIZE);

                // upload consensus result
                do {
                    RAND_bytes(nonce, NONCE_SIZE);
                } while(is_nonce_used(ra_nonces, ra_nonces_count, nonce) || is_nonce_used(msg_nonces, msg_nonces_count, nonce));

                memcpy(ra_nonces[ra_nonces_count++], nonce, NONCE_SIZE);
                memcpy(msg_nonces[msg_nonces_count++], nonce, NONCE_SIZE);

                memcpy(nonced_buf, nonce, NONCE_SIZE);
                memcpy(nonced_buf + NONCE_SIZE, params_hash, HASH_SIZE);
                SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);
                sign_hash(local_privkey, nonced_hash, signature);

                upload_consensus_result(params_hash, nonce, signature, node_id);
                printf("[Leader Node %d] Upload parameters hash, nonce and signature to FISCO BCOS\n", node_id);

                // rebuild peer ecdsa public key from teeinfo
                uint8_t public_key_x[EC_PUBKEY_SIZE];
                uint8_t public_key_y[EC_PUBKEY_SIZE];

                recv_full(peer_sock, public_key_x, EC_PUBKEY_SIZE);
                recv_full(peer_sock, public_key_y, EC_PUBKEY_SIZE);
                peer_pubkey = build_pubkey_from_xy(public_key_x, public_key_y);

                // ECDH
                EC_KEY *ecdh_key = generate_key();
                uint8_t private_key[EC_PUBKEY_SIZE];
                uint8_t public_key_buffer[EC_PUBKEY_SIZE * 2];
                uint8_t public_key_hash[HASH_SIZE];
                uint8_t shared_key[AES_KEY_SIZE];

                extract_pubkey_xy(ecdh_key, public_key_x, public_key_y);
                extract_privkey(ecdh_key, private_key);
                do {
                    RAND_bytes(nonce, NONCE_SIZE);
                } while(is_nonce_used(ra_nonces, ra_nonces_count, nonce));
                memcpy(ra_nonces[ra_nonces_count++], nonce, NONCE_SIZE);

                memcpy(public_key_buffer, public_key_x, EC_PUBKEY_SIZE);
                memcpy(public_key_buffer + EC_PUBKEY_SIZE, public_key_y, EC_PUBKEY_SIZE);
                SHA256(public_key_buffer, EC_PUBKEY_SIZE * 2, public_key_hash);

                memcpy(nonced_buf, nonce, NONCE_SIZE);
                memcpy(nonced_buf + NONCE_SIZE, public_key_hash, HASH_SIZE);
                SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);
                sign_hash(local_privkey, nonced_hash, signature);

                send(peer_sock, public_key_buffer, EC_PUBKEY_SIZE * 2, 0);
                send(peer_sock, nonce, NONCE_SIZE, 0);
                send(peer_sock, signature, SIGN_SIZE, 0);

                recv_full(peer_sock, public_key_buffer, EC_PUBKEY_SIZE * 2);
                recv_full(peer_sock, nonce, NONCE_SIZE);
                recv_full(peer_sock, signature, SIGN_SIZE);
                SHA256(public_key_buffer, EC_PUBKEY_SIZE * 2, public_key_hash);

                if (!verify_sig(peer_pubkey, public_key_hash, nonce, signature, MODE_RA)) {
                    printf("[Leader Node %d] Verify peer ECDH message signature failed\n", node_id);
                    exit(0);
                }

                // compute shared key
                memcpy(public_key_x, public_key_buffer, EC_PUBKEY_SIZE);
                memcpy(public_key_y, public_key_buffer + EC_PUBKEY_SIZE, EC_PUBKEY_SIZE);                
                if (compute_share_key(private_key, public_key_x, public_key_y, shared_key) != 0) exit(0);

                // encrypt game params and send it to peer node
                uint8_t iv[AES_BLOCK_SIZE];
                uint8_t ciphertext_hash[HASH_SIZE];
                uint8_t *ciphertext = malloc(size + AES_BLOCK_SIZE * 2);
                int ciphertext_size;

                RAND_bytes(iv, AES_BLOCK_SIZE);
                aes_encrypt_cbc(buffer, size, shared_key, iv, ciphertext, &ciphertext_size);

                send(peer_sock, &pinfo.size, sizeof(int), 0);
                send(peer_sock, &rinfo.size, sizeof(int), 0);
                send(peer_sock, iv, AES_BLOCK_SIZE, 0);

                send(peer_sock, &ciphertext_size, sizeof(int), 0);
                send(peer_sock, ciphertext, ciphertext_size, 0);

                do {
                    RAND_bytes(nonce, NONCE_SIZE);
                } while(is_nonce_used(ra_nonces, ra_nonces_count, nonce));

                memcpy(ra_nonces[ra_nonces_count++], nonce, NONCE_SIZE);

                SHA256(ciphertext, ciphertext_size, ciphertext_hash);
                memcpy(nonced_buf, nonce, NONCE_SIZE);
                memcpy(nonced_buf + NONCE_SIZE, ciphertext_hash, HASH_SIZE);
                SHA256(nonced_buf, NONCE_SIZE + HASH_SIZE, nonced_hash);

                sign_hash(local_privkey, nonced_hash, signature);
                send(peer_sock, nonce, NONCE_SIZE, 0);
                send(peer_sock, signature, SIGN_SIZE, 0);

                printf("[Leader Node %d] Send encrypted parameters, nonce and signature to Peer\n", node_id);

                commit_count = 0;
                prepare_count = 0;

                free(ciphertext);
                exit(0); // leader node end.
            }
        }
        pthread_mutex_unlock(&lock);
    }
}
