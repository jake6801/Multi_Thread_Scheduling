#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h> //?
#include <errno.h> //?
#include <math.h>
#include "mts.h"

#define NANOSECOND_CONVERSION 1e9

// mutex, convar and global variable for loading trains 
//! need to change these a bit from loading_train.c
bool ready_to_load = false; 
pthread_mutex_t start_timer;
pthread_cond_t train_ready_to_load;
// mutex, convar and global variables for determining when dispatcher should broadcast to trians to start loading 
int ready_to_load_count = 0;
pthread_mutex_t ready_to_load_count_mutex;
pthread_cond_t all_ready_to_load;
//TODO mutexes for addings to station queues 

//TODO mutex for putting train on main track

// global variables 
int num_trains = 0;


struct timespec start_time = { 0 };

// Convert timespec to seconds
double timespec_to_seconds(struct timespec *ts) {
    return ((double) ts->tv_sec) + (((double) ts->tv_nsec) / NANOSECOND_CONVERSION);
}

// thread function 
void* train_thread_func(void *train) {
    // size_t i = (intptr_t) t;
    struct train *train_object = (struct train *) train;

    pthread_mutex_lock(&ready_to_load_count_mutex);
    ready_to_load_count++;
    if (ready_to_load_count == num_trains) {
        pthread_cond_signal(&all_ready_to_load);
    }
    pthread_mutex_unlock(&ready_to_load_count_mutex);

    // wait until start signal given 
    pthread_mutex_lock(&start_timer);
    while(!ready_to_load) {
        pthread_cond_wait(&train_ready_to_load, &start_timer);        
    }    
    pthread_mutex_unlock(&start_timer);

    struct timespec load_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &load_time); //! whats wrong with CLOCK_MONOTONIC but its in the sample code? 

    printf("Start loading train %d at time %f (at simulation time %f) \n", train_object->train_no, timespec_to_seconds(&load_time), timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));

    usleep(train_object->loading_time * 1000000); //? is this the best way to do the loading time?
    clock_gettime(CLOCK_MONOTONIC, &load_time); //! whats wrong with CLOCK_MONOTONIC but its in the sample code? 
    printf("train %d finished loading at time %f (at simulation time %f)\n", train_object->train_no, timespec_to_seconds(&load_time), timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    
    // *train_object->state = "loaded"; //! fix this?

    //TODO create priority queue and put train in queue (need to do mutex stuff)
    // *train_object->state = "ready";    
    //TODO signal to dispatcher that trains are ready? (need to do local convar stuff?)

    //TODO once received signal from dispatcher and mutex to cross, cross 
    // ... mutex and convar stuff 
    // struct timespec cross_time = { 0 };
    // clock_gettime(CLOCK_MONOTONIC, &cross_time);
    // *train_object->state = "crossing";
    // usleep(train_object->crossing_time * 1000000);
    // clock_gettime(CLOCK_MONOTONIC, &cross_time);

    //TODO signal back to dispatcher that train has crossed and exit
    // ... signal back to dispatcher
    // *train_object->state = "crossed";
    // pthread_exit(NULL);
}

// broadcast to start loading trains 
void start_trains() {
    pthread_mutex_lock(&start_timer);
    ready_to_load = true; 
    pthread_cond_broadcast(&train_ready_to_load);
    pthread_mutex_unlock(&start_timer);
} 


int main() { 
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        printf("Could not open file.\n");
        return 1;
    }
    
    struct train** trains_array = NULL;
    int trains_array_capacity = 10;
    trains_array = malloc(trains_array_capacity * sizeof(struct train*));
    // int count = 0;
    char dir;
    float ltime;
    float ctime;
    // create the train objects and threads 
    while (fscanf(file, "%s %f %f", &dir, &ltime, &ctime) != EOF) {        
        // dynamically allocate memory for the trains_array
        if (num_trains >= trains_array_capacity) {
            trains_array_capacity *= 2;
            trains_array = realloc(trains_array, trains_array_capacity * sizeof(struct train*));
        }
        // create train objects and put in trains_array
        struct train* new_train = (struct train*)malloc(sizeof(struct train));
        new_train->train_no = num_trains;
         if (isupper(dir)) {
            strcpy(new_train->priority, "high");
        } else {
            strcpy(new_train->priority, "low");
        }
         if (tolower(dir) == 'e') {
            strcpy(new_train->direction, "east");
        } else {
            strcpy(new_train->direction, "west");
        }
        new_train->loading_time = ltime/10;
        new_train->crossing_time = ctime/10;
        strcpy(new_train->state, "waiting");
        new_train->next = NULL;

        trains_array[num_trains] = new_train;
        num_trains++;
    }

    // trains 
    pthread_t train_thread_ids[num_trains];

    struct timespec initial_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &initial_time);
    printf("Program start time: %f\n", timespec_to_seconds(&initial_time));

    struct timespec create_time = { 0 };
    // create the thread for each train object
    for (size_t i = 0; i < num_trains; ++i) { //? should this be ++i or i++?
        clock_gettime(CLOCK_MONOTONIC, &create_time);
        printf("Creating train %ld at time %f\n", i, timespec_to_seconds(&create_time));
        
        // create train thread 
        if (pthread_create(&train_thread_ids[i], NULL, train_thread_func, (void *) trains_array[i])) { // (void *) (intptr_t) i
            printf("error creating train thread\n");
        }
        sleep(1); //? why do we need to simulate delay
    }    

    // use convar and mutex to wait for all trains to be ready to load 
    pthread_mutex_lock(&ready_to_load_count_mutex);
    while (ready_to_load_count != num_trains) {
        pthread_cond_wait(&all_ready_to_load, &ready_to_load_count_mutex);
    }
    pthread_mutex_unlock(&ready_to_load_count_mutex);

    // get start time 
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    printf("program start time: %f\n", timespec_to_seconds(&start_time));

    // start trains loading 
    start_trains();

    for(size_t i = 0; i < num_trains; ++i) {
        pthread_join(train_thread_ids[i], NULL);
        free(trains_array[i]);
    }

    free(trains_array);
    fclose(file);
    return 0;
}