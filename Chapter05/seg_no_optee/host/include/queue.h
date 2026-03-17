
#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct {
    double *data;
    int front;
    int rear;
    int capacity;
} Queue;

Queue* create_queue(int capacity);
void destroy_queue(Queue *q);

bool enqueue(Queue *q, double value);
double dequeue(Queue *q);

bool is_full_queue(Queue *q);
bool is_empty_queue(Queue *q);

#endif
