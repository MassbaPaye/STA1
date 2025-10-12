#include "queue.h"
#include <stdlib.h>
#include <string.h>

int queue_init(Queue* q, int capacity, int elem_size) {
    q->buffer = malloc(capacity * elem_size);
    if (!q->buffer) return -1;

    q->capacity = capacity;
    q->elem_size = elem_size;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    pthread_mutex_init(&q->mtx, NULL);

    return 0;
}

void queue_destroy(Queue* q) {
    free(q->buffer);
    pthread_mutex_destroy(&q->mtx);
}

int queue_enqueue(Queue* q, const void* elem) {
    pthread_mutex_lock(&q->mtx);
    if (q->size == q->capacity) {
        pthread_mutex_unlock(&q->mtx);
        return -1; // pleine
    }

    void* target = (char*)q->buffer + (q->tail * q->elem_size);
    memcpy(target, elem, q->elem_size);

    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    pthread_mutex_unlock(&q->mtx);
    return 0;
}

int queue_dequeue(Queue* q, void* elem) {
    pthread_mutex_lock(&q->mtx);
    if (q->size == 0) {
        pthread_mutex_unlock(&q->mtx);
        return -1; // vide
    }

    void* source = (char*)q->buffer + (q->head * q->elem_size);
    memcpy(elem, source, q->elem_size);

    q->head = (q->head + 1) % q->capacity;
    q->size--;
    pthread_mutex_unlock(&q->mtx);
    return 0;
}

int queue_size(Queue* q) {
    pthread_mutex_lock(&q->mtx);
    int s = q->size;
    pthread_mutex_unlock(&q->mtx);
    return s;
}
