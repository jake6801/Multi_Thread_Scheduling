#ifndef MTS_H
#define MTS_H

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include "priority_queue.h"

// Constants
#define NANOSECOND_CONVERSION 1e9

// Train object structure
struct train {
    int train_no;
    char priority[5];
    char direction[5];
    float loading_time;
    float crossing_time;
    struct train* next;
    pthread_cond_t main_track_available;
    pthread_mutex_t main_track_mutex;
};

// Global variables
extern bool ready_to_load;
extern pthread_mutex_t start_timer;
extern pthread_cond_t train_ready_to_load;

extern int ready_to_load_count;
extern pthread_mutex_t ready_to_load_count_mutex;
extern pthread_cond_t all_ready_to_load;

extern int ready_to_cross_count;
extern pthread_mutex_t ready_to_cross_count_mutex;
extern pthread_cond_t train_available;

extern priority_queue* eastbound_pq;
extern pthread_mutex_t epq_access;
extern priority_queue* westbound_pq;
extern pthread_mutex_t wpq_access;

extern int num_trains;
extern FILE *output_file;
extern struct timespec start_time;

extern pthread_mutex_t main_track;
extern pthread_cond_t track_free;

// Function declarations
double timespec_to_seconds(struct timespec *ts);
char* format_output_time(double sim_seconds);
void* train_thread_func(void *train);
void start_trains();

#endif 