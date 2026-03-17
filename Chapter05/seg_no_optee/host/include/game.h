#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define PER_PRICE_STR_SIZE 32

typedef struct {
    double *prices;
    double *amounts;
    double *reputations;
    double *costs;
    int size;
} ProvidersInfo;
 
typedef struct {
    double *lambda1s;
    double *lambda2s;
    double *sensitivities;
    int size;
} RequestersInfo;

void noncooperative_game(char *prices, uint8_t *nonce, uint8_t *signature);
void load_game_params(uint8_t *buffer, int psize, int rsize);
void get_game_params_info(uint8_t *nonce, uint8_t *signature);

#endif