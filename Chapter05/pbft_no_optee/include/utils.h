#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdlib.h>

void print_hex(const uint8_t *buf, size_t len);
int hex_to_bytes(const char *hexstr, uint8_t *buf);
char* bytes_to_hex(const uint8_t *bytes, size_t len);

#endif