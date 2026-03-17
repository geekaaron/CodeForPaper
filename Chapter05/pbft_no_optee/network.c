#include "global.h"
#include "network.h"

ssize_t recv_full(int sock, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t r = recv(sock, (char*)buf + total, len - total, 0);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

static void send_to(int target, PBFTMessage *msg) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_BASE + target);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) >= 0) {
        send(sock, msg, sizeof(PBFTMessage), 0);
    }
    close(sock);
}

void broadcast(PBFTMessage *msg) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (i != node_id) send_to(i, msg);
    }
}

void *server_thread(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_BASE + node_id);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 256) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        PBFTMessage msg;
        ssize_t r = recv_full(new_socket, &msg, sizeof(msg));
        if (r == sizeof(msg)) {
            handle_msg(&msg);
        }
        close(new_socket);
    }
    return NULL;
}