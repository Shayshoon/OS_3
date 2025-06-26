#include <pthread.h>
#include "segel.h"
#include "request.h"
#include "log.h"
#include "queue.h"

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

struct Thread_data {
    Queue* queue;          // Pointer to the request queue
    ServerLog* log;        // Pointer to the server log
    threads_stats stats;   // Pointer to thread statistics structure
};

void* work(void* data) {
    struct Thread_data* thread_data = (struct Thread_data*) data;

    while (1) {

        Node* node = q_dequeue(thread_data->queue);

        // Record the request arrival time
        struct timeval dispatch;
        gettimeofday(&dispatch, NULL);
        if (dispatch.tv_usec < node->arrival.tv_usec) {
            dispatch.tv_sec -= 1;
            dispatch.tv_usec += 1000000;
        }
        dispatch.tv_sec -= node->arrival.tv_sec;
        dispatch.tv_usec -= node->arrival.tv_usec;

        // Process the request
        requestHandle(node->fd, node->arrival, dispatch, thread_data->stats, thread_data->log);

        // Close the connection
        Close(node->fd);
        free(node);
        sem_post(&thread_data->queue->available_positions);
    }

    free(thread_data->stats); // Free the thread stats structure
    free(thread_data);
    return NULL;
}

void init_thread_pool(int pool_size, pthread_t threads[], Queue* queue, ServerLog* log) {
    // Initialize the thread pool with the specified number of worker threads
    for (int i = 0; i < pool_size; i++) {
        pthread_t thread;

        // Create a thread stats structure
        threads_stats stats = malloc(sizeof(struct Threads_stats));
        stats->id = i+1;
        stats->post_req = 0;         // POST request count
        stats->stat_req = 0;         // Static request count
        stats->dynm_req = 0;         // Dynamic request count
        stats->total_req = 0;        // Total request count

        struct Thread_data* data = malloc(sizeof(struct Thread_data));
        data->stats = stats; // Assign the thread stats
        data->queue = queue; // Assign the request queue
        data->log = log; // Assign the server log

        if (pthread_create(&thread, NULL, work, data) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(1);
        }
        threads[i] = thread;
    }
}

// args[0] = port number
// args[1] = number of worker threads
// args[2] = size of request queue
// Parses command-line arguments
void getargs(int args[], int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    args[0] = atoi(argv[1]);
    args[1] = atoi(argv[2]);
    args[2] = atoi(argv[3]);

    if (args[0] <= 2000 || args[1] <= 0 || args[2] <= 0) {
        exit(1);
    }
}

// This server currently handles all requests in the main thread.
// You must implement a thread pool (fixed number of worker threads)
// that process requests from a synchronized queue.

int main(int argc, char *argv[])
{
    // Create the global server log
    ServerLog* log = create_log();

    int listenfd, connfd, port, pool_size, queue_size, clientlen;
    int args[3];
    struct sockaddr_in clientaddr;

    if (argc < 4) {
        exit(1);
    }

    getargs(args, argc, argv);
    port = args[0];
    pool_size = args[1];
    queue_size = args[2];

//  Initialize the request queue
    Queue* request_queue = q_create(queue_size);

// Initialize the thread pool
    pthread_t threads[pool_size];
    init_thread_pool(pool_size, threads, request_queue, log);

    listenfd = Open_listenfd(port);
    while (1) {
        sem_wait(&request_queue->available_positions); // BLOCK if full

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        if (connfd < 0) {
            sem_post(&request_queue->available_positions);
            fprintf(stderr, "Error accepting connection\n");
            continue; // Skip to the next iteration on error
        }

        // logic to enqueue the connection and let
        // a worker thread process it from the queue.

        struct timeval arrival;
        gettimeofday(&arrival, NULL);

        q_enqueue(request_queue, connfd, arrival);
    }

    // Clean up the server log before exiting
    for (int i = 0; i < pool_size; i++) {
        pthread_join(threads[i], NULL); // Wait for all threads to finish
    }

    destroy_log(log);
    q_destroy(request_queue);
    Close(listenfd);
    return 0;
}
