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

#define NANOSECOND_CONVERSION 1e9

// mutexes 
//! need to change these a bit from loading_train.c
pthread_mutex_t start_timer;
pthread_cond_t train_ready_to_load;
bool ready_to_load = false; 

struct timespec start_time = { 0 };

// train object
struct train {
    //TODO add in thread_id when creating threads
    int train_no;
    char priority[5];
    char direction[5];
    float loading_time;
    float crossing_time;
    char state[20]; //? what should the length of this be 
        // waiting -> loading -> ready -> granted -> crossing -> crossed
};

// Convert timespec to seconds
double timespec_to_seconds(struct timespec *ts) {
    return ((double) ts->tv_sec) + (((double) ts->tv_nsec) / NANOSECOND_CONVERSION);
}

// thread function 
void* train_thread_func(void *train) {
    // size_t i = (intptr_t) t;
    struct train *train_object = (struct train *) train;

    // wait until start signal given 
    pthread_mutex_lock(&start_timer);
    while(!ready_to_load) {
        pthread_cond_wait(&train_ready_to_load, &start_timer);        
    }    
    pthread_mutex_unlock(&start_timer);

    struct timespec load_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &load_time); //! whats wrong with CLOCK_MONOTONIC but its in the sample code? 

    printf("Start loading train %d at time %f (at simulation time %f) \n", train_object->train_no, timespec_to_seconds(&load_time), timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    //TODO how to pass correct train object and load for that amount of time?
    // sleep(train_object->loading_time);
    // double seconds_part;
    // double fractional_part = modf(train_object->loading_time, &seconds_part);
    
    // struct timespec loading_duration;
    // loading_duration.tv_sec = (time_t) seconds_part;
    // loading_duration.tv_nsec = (long) (fractional_part * NANOSECOND_CONVERSION);  // Convert fraction to nanoseconds

    // // Simulate the train loading process using nanosleep()
    // nanosleep(&loading_duration, NULL);

    //? is this the best way to do the loading time?
    usleep(train_object->loading_time * 1000000);
    clock_gettime(CLOCK_MONOTONIC, &load_time); //! whats wrong with CLOCK_MONOTONIC but its in the sample code? 
    printf("train %d finished loading at time %f (at simulation time %f)\n", train_object->train_no, timespec_to_seconds(&load_time), timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    
    *train_object->state = "loaded"; //! fix this?

    //TODO create priority queue and put train in queue (need to do mutex stuff)
    *train_object->state = "ready";    
    //TODO signal to dispatcher that trains are ready? (need to do local convar stuff?)

    //TODO once received signal from dispatcher and mutex to cross, cross 
    // ... mutex and convar stuff 
    struct timespec cross_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &cross_time);
    *train_object->state = "crossing";
    usleep(train_object->crossing_time * 1000000);
    clock_gettime(CLOCK_MONOTONIC, &cross_time);

    //TODO signal back to dispatcher that train has crossed and exit
    // ... signal back to dispatcher
    *train_object->state = "crossed";
    pthread_exit(NULL);
}

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
    
    struct train trains[5];
    int count = 0;
    int num;
    char dir;
    float ltime;
    float ctime;
    // create the train objects and threads 
    while (fscanf(file, "%s %f %f", &dir, &ltime, &ctime) != EOF) {        
        trains[count].train_no = count;
        if (isupper(dir)) {
            strcpy(trains[count].priority, "high");
        } else {
            strcpy(trains[count].priority, "low");
        }
        if (tolower(dir) == 'e') {
            strcpy(trains[count].direction, "east");
        } else {
            strcpy(trains[count].direction, "west");
        }
        trains[count].loading_time = ltime / 10;
        trains[count].crossing_time = ctime / 10;
        strcpy(trains[count].state, "waiting");

        count++;
    }

    // trains 
    pthread_t train_thread_ids[count];
    //TODO could make an array of trains and instead of passing this i thing I can pass the train object at index i?
    

    struct timespec initial_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &initial_time);
    printf("Program start time: %f\n", timespec_to_seconds(&initial_time));

    struct timespec create_time = { 0 };
    // create the thread for each train object
    for (size_t i = 0; i < count; ++i) { //? should this be ++i or i++?
        clock_gettime(CLOCK_MONOTONIC, &create_time);
        printf("Creating train %ld at time %f\n", i, timespec_to_seconds(&create_time));
        
        // create train thread 
        if (pthread_create(&train_thread_ids[i], NULL, train_thread_func, (void *) &trains[i])) { // (void *) (intptr_t) i
            printf("error creating train thread\n");
        }
        sleep(1); //? why do we need to simulate delay
    }    
    // Note: dont sync broadcast this way 
    sleep(1);

    // get start time 
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    printf("program start time: %f\n", timespec_to_seconds(&start_time));

    // start trains loading 
    start_trains();

    for(size_t i = 0; i < count; ++i) {
        pthread_join(train_thread_ids[i], NULL);
    }

    fclose(file);
    return 0;
}