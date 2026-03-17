#include <stdlib.h>
#include "include/queue.h"

Queue* create_queue(int capacity) {
    Queue *q = malloc(sizeof(Queue));
    q->data = malloc(capacity * sizeof(double));
    q->front = q->rear = 0;
    q->capacity = capacity;
    return q;
}

void destroy_queue(Queue *q) {
    free(q->data);
    free(q);
}

bool enqueue(Queue *q, double value) {
    if (is_full_queue(q)) dequeue(q);

    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
    return true;
}

double dequeue(Queue *q) {
    if (is_empty_queue(q)) return -1;

    double value = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    return value;
}

bool is_full_queue(Queue *q) {
    return (q->rear + 1) % q->capacity == q->front;
}

bool is_empty_queue(Queue *q) {
    return q->front == q->rear;
}
