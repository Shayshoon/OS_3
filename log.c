#include <stdlib.h>
#include <string.h>
#include "log.h"
#include <unistd.h>

// Creates a new server log instance (stub)
ServerLog* create_log() {
    ServerLog* log = (ServerLog*)malloc(sizeof(ServerLog));

    log->entry_count = 0;
    log->total_length = 0;
    log->head = NULL;
    log->tail = NULL;
    log->readers = 0;
    log->writers = 0;
    sem_init(&log->rmutex, 0, 1);
    sem_init(&log->wmutex, 0, 1);
    sem_init(&log->readTry, 0, 1);
    sem_init(&log->log, 0, 1);

    return log; // Return the newly created log
}

// Destroys and frees the log (stub)
void destroy_log(ServerLog* log) {
    if (!log) {
        return;
//        TODO: handle error?
    }

    // Free all log entries
    // Begin critical section
    sem_wait(&log->log);
    sem_wait(&log->wmutex);
    sem_wait(&log->readTry);
    sem_wait(&log->rmutex);

    LogEntry* current = log->head;

    while (current) {
        LogEntry* next = current->next;
        free(current->data); // Free the data string
        free(current);       // Free the log entry
        current = next;     // Move to the next entry
    }
    log->head = NULL;
    log->tail = NULL;

    free(log);
    sem_post(&log->rmutex);
    sem_post(&log->readTry);
    sem_post(&log->wmutex);
    sem_post(&log->log);
    // End critical section

    sem_destroy(&log->rmutex);
    sem_destroy(&log->wmutex);
    sem_destroy(&log->readTry);
    sem_destroy(&log->log);
}

// Used second readers-writers solution https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem
// Returns log content as string
int get_log(ServerLog* log, char** dst) {
    if (!log) {
        return 0; // Invalid parameters
    }

    // Entry section
    sem_wait(&log->readTry);
    sem_wait(&log->rmutex);
    log->readers++;
    if (log->readers == 1) {
        sem_wait(&log->log);
    }
    sem_post(&log->rmutex);
    sem_post(&log->readTry);

    // Begin critical section
    int log_length = log->total_length;
    int i = 0;
    *dst = (char*) malloc(log_length + 1); // Allocate for caller

    // iterate over log linked list and append entries to dst
    for (LogEntry* entry = log->head; entry != NULL; entry = entry->next) {
        strncpy(*dst + i, entry->data, entry->data_len);
        i += entry->data_len;
    }
    // End critical section

    // Exit section
    sem_wait(&log->rmutex);
    log->readers--;
    if (log->readers == 0) {
        sem_post(&log->log);
    }
    sem_post(&log->rmutex);

    (*dst)[log_length] = '\0';

    return log_length;
}

// Appends a new entry to the log (no-op stub)
void add_to_log(ServerLog* log, const char* data, int data_len) {
    if (log == NULL || data == NULL || data_len <= 0) {
        return; // Invalid parameters
    }

    // Create a new log entry
    LogEntry *new_entry = (LogEntry *) malloc(sizeof(LogEntry));
    if (new_entry == NULL) {
        return; // Memory allocation failed
    }

    new_entry->data = (char *) malloc(data_len + 1); // Allocate memory for data
    new_entry->data_len = data_len;

    if (new_entry->data == NULL) {
        free(new_entry);
        return; // Memory allocation failed
    }

    strncpy(new_entry->data, data, data_len);
    new_entry->data[data_len] = '\0';
    new_entry->next = NULL;

    // Entry section
    sem_wait(&log->wmutex);
    log->writers++;
    if (log->writers == 1) {
        sem_wait(&log->readTry);
    }
    sem_post(&log->wmutex);

    sem_wait(&log->log);
    usleep(200000);
    // Begin critical section
    if (log->tail == NULL) {
        // Log is empty, initialize head and tail
        log->head = new_entry;
        log->tail = new_entry;
    } else {
        // Append to the end of the log
        log->tail->next = new_entry;
        log->tail = new_entry;
    }

    log->entry_count++;
    log->total_length += data_len;
    // End critical section
    sem_post(&log->log);

    // Exit section
    sem_wait(&log->wmutex);
    log->writers--;
    if (log->writers == 0) {
        sem_post(&log->readTry);
    }
    sem_post(&log->wmutex);
}
