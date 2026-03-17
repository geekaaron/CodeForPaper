#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "json.h"

#define PORT 8080

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;

    cJSON *json;

    ProvidersInfo pinfo;
    RequestersInfo rinfo;

    uint8_t *buffer = NULL;
    int size = 0;
    int current = 0;

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 0;
    }

    if ((json = json_from_file(argv[1])) == NULL) return -1;
    if (json_to_struct(json, &pinfo, &rinfo) != 0) return -1;

    size = (pinfo.size * PROVIDER_PARAMS_NUM + rinfo.size * REQUESTER_PARAMS_NUM) * sizeof(double);
    buffer = malloc(size);

    current = 0;
    memcpy(buffer + current, pinfo.prices, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(buffer + current, pinfo.amounts, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(buffer + current, pinfo.reputations, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);
    memcpy(buffer + current, pinfo.costs, sizeof(double) * pinfo.size);
    current += pinfo.size * sizeof(double);

    memcpy(buffer + current, rinfo.lambda1s, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    memcpy(buffer + current, rinfo.lambda2s, sizeof(double) * rinfo.size);
    current += rinfo.size * sizeof(double);
    memcpy(buffer + current, rinfo.sensitivities, sizeof(double) * rinfo.size);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "192.168.31.3", &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    send(sock, &pinfo.size, sizeof(int), 0);
    send(sock, &rinfo.size, sizeof(int), 0);
    send(sock, buffer, size, 0);

    close(sock);
    return 0;
}