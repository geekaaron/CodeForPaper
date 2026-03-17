#ifndef NETWORK_H
#define NETWORK_H

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "message.h"

#define PORT_BASE 8000

ssize_t recv_full(int sock, void *buf, size_t len);
void broadcast(PBFTMessage *msg);
void *server_thread(void *arg);

#endif