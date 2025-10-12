#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

/* File circulaire générique */
typedef struct {
    void* buffer;        // tableau d'éléments
    int capacity;        // nombre max d’éléments
    int elem_size;       // taille d’un élément
    int head;            // index de lecture
    int tail;            // index d’écriture
    int size;            // nb d’éléments courants
    pthread_mutex_t mtx; // mutex pour protéger la file
} Queue;

/* Création / destruction */
int queue_init(Queue* q, int capacity, int elem_size);
void queue_destroy(Queue* q);

/* Opérations */
int queue_enqueue(Queue* q, const void* elem); // ajoute un élément
int queue_dequeue(Queue* q, void* elem);       // retire un élément
int queue_size(Queue* q);                      // nombre d’éléments

#endif
