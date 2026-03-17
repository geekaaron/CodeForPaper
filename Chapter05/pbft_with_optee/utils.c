#include <stdio.h>
#include <string.h>
#include "utils.h"

void print_hex(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) printf("%02x", buf[i]);
    printf("\n");
}

int hex_to_bytes(const char *hexstr, uint8_t *buf) {
    size_t len = strlen(hexstr);
    if (len % 2 != 0) return -1;
    for (size_t i = 0; i < len / 2; ++i) sscanf(hexstr + 2 * i, "%2hhx", &buf[i]);
    return 0;
}

char *bytes_to_hex(const uint8_t *bytes, size_t len) {
    if (!bytes || len == 0) return NULL;
    char *hexstr = malloc(len * 2 + 1);
    if (!hexstr) return NULL;
    for (size_t i = 0; i < len; ++i) sprintf(hexstr + 2*i, "%02x", bytes[i]);
    hexstr[len * 2] = '\0';
    return hexstr;
}