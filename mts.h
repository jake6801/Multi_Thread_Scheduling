#ifndef MTS_H
#define MTS_H

#include <stdbool.h>
#include <pthread.h>
#include <time.h>

// Constants
#define NANOSECOND_CONVERSION 1e9

// Train object structure
struct train {
    int train_no;
    char priority[5];
    char direction[5];
    float loading_time;
    float crossing_time;
    char state[20]; 
    struct train* next;
};

// Global variables
extern bool ready_to_load;
extern pthread_mutex_t start_timer;
extern pthread_cond_t train_ready_to_load;

extern int ready_to_load_count;
extern pthread_mutex_t ready_to_load_count_mutex;
extern pthread_cond_t all_ready_to_load;

extern int num_trains;
extern struct timespec start_time;

// Function declarations
double timespec_to_seconds(struct timespec *ts);
void* train_thread_func(void *train);
void start_trains();

#endif 