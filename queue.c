#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.h"
#include "segel.h"

Queue* q_create(int size) {
    Queue* q = malloc(sizeof(Queue));
    if (!q) return NULL;

    q->max_size = size;
    q->current_size = 0;
    q->head = NULL;
    q->tail = NULL;

    pthread_mutex_init(&q->lock, NULL);
    sem_init(&q->available_positions, 0, size);
    sem_init(&q->waiting_requests, 0, 0);

    return q;
}

void q_enqueue(Queue* queue, int fd, struct timeval arrival) {
    pthread_mutex_lock(&queue->lock);

    Node* node = malloc(sizeof(Node));
    if (!node) {
        pthread_mutex_unlock(&queue->lock);
        exit(1); // Handle memory allocation failure
    }

    node->fd = fd;
    node->arrival = arrival;
    node->next = NULL;

    if (queue->tail) {
        queue->tail->next = node;
        queue->tail = node;
    } else {
        queue->head = queue->tail = node;
    }

    queue->current_size++;

    pthread_mutex_unlock(&queue->lock);
    sem_post(&queue->waiting_requests); // signal there's a new item
}

Node* q_dequeue(Queue* queue) {
    sem_wait(&queue->waiting_requests);
    pthread_mutex_lock(&queue->lock);

    Node* node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL)
        queue->tail = NULL;

    queue->current_size--;
    pthread_mutex_unlock(&queue->lock);

    return node;
}

void q_destroy(Queue* queue) {
    pthread_mutex_lock(&queue->lock);

    Node* current = queue->head;
    while (current) {
        Node* tmp = current;
        current = current->next;
        free(tmp);
    }

    pthread_mutex_unlock(&queue->lock);
    pthread_mutex_destroy(&queue->lock);
    sem_destroy(&queue->available_positions);
    sem_destroy(&queue->waiting_requests);

    free(queue);
}
