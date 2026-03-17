#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <tee_client_api.h>
#include <seg_random_ta.h>

#define PROVIDER_PARAMS_NUM     4
#define REQUESTER_PARAMS_NUM    3

#define PORT        8080

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

int main(int argc, char *argv[])
{
    TEEC_Result res;
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    TEEC_UUID uuid = TA_SEG_RANDOM_UUID;
    uint32_t err_origin;

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    int received, n;

    uint8_t *buffer = NULL;
    int psize, rsize, size;
    double *prices = NULL;

    clock_t start, end;

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

    res = TEEC_OpenSession(&ctx, &sess, &uuid,
                           TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
             res, err_origin);

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

    // receive game params
    safe_recv(client_sock, (uint8_t *)&psize, sizeof(int));
    safe_recv(client_sock, (uint8_t *)&rsize, sizeof(int));

    size = (psize * PROVIDER_PARAMS_NUM + rsize * REQUESTER_PARAMS_NUM) * sizeof(double);
    buffer = malloc(size);
    safe_recv(client_sock, buffer, size);

    // load game params
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_NONE);

    op.params[0].value.a = psize;
    op.params[1].value.a = rsize;
    op.params[2].tmpref.buffer = buffer;
    op.params[2].tmpref.size = size;

    printf("Invoking TA to load game params.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_RANDOM_CMD_LOAD_GAME_PARAMS,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG_RANDOM) failed 0x%x origin 0x%x",
             res, err_origin);

    // start pricing game
    start = clock();
    prices = malloc(psize * sizeof(double));

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE, TEEC_NONE);

    op.params[0].tmpref.buffer = prices;
    op.params[0].tmpref.size = psize * sizeof(double);

    printf("Invoking TA to start pricing game.\n");
    res = TEEC_InvokeCommand(&sess, TA_SEG_RANDOM_CMD_START_PRICING_GAME,
                             &op, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(SEG_RANDOM) failed 0x%x origin 0x%x",
             res, err_origin);

    printf("optimal prices: ");
    for (int i = 0; i < psize; i++) {
        printf("%.2f ", prices[i]);
    }
    printf("\n");

    end = clock();
    printf("time: %.3f s\n", (double)(end - start) / CLOCKS_PER_SEC);

    free(buffer);
    free(prices);
    close(server_sock);
    close(client_sock);
    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);

    return 0;
}
