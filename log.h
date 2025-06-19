#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

// TODO:
// Implement a thread-safe server log system.
// - The log should support concurrent access from multiple threads.
// - You must implement a multiple-readers/single-writer synchronization model.
// - Writers must have priority over readers.
//   This means that if a writer is waiting, new readers should be blocked until the writer is done.
// - Use appropriate synchronization primitives (e.g., pthread mutexes and condition variables).
// - The log should allow appending entries and returning the full log content.


typedef struct LogEntry {
    char* data;
    int data_len;
    struct LogEntry* next;
} LogEntry;

// Opaque struct definition
typedef struct ServerLog {
    // This struct should include synchronization primitives for thread safety (we use linked list)
    int entry_count;      // Number of entries in the log
    int total_length;    // Total length of all entries in the log
    LogEntry* head; // Pointer to the first log entry
    LogEntry* tail; // Pointer to the last log entry
    sem_t rmutex;
    sem_t wmutex;
    sem_t readTry;
    sem_t log;
    int readers;
    int writers;
} ServerLog;

// Creates a new server log instance
ServerLog* create_log();

// Destroys and frees the log
void destroy_log(ServerLog* log);

// Returns the log contents as a string (null-terminated)
// NOTE: caller is responsible for freeing dst
int get_log(ServerLog* log, char** dst);

// Appends a new entry to the log
void add_to_log(ServerLog* log, const char* data, int data_len);

#endif // SERVER_LOG_H
