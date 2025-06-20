#pragma once
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct Node {
    int fd;
    struct timeval arrival;
    struct Node* next;
} Node;

typedef struct Queue {
    int max_size;
    int current_size;
    Node* head;
    Node* tail;
    sem_t available_positions;
    sem_t waiting_requests;
    pthread_mutex_t lock;
} Queue;

Queue* q_create(int size);
void q_enqueue(Queue* queue, int fd, struct timeval arrival);
Node* q_dequeue(Queue* queue);
void q_destroy(Queue* queue);
