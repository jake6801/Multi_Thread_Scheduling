#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h> 
#include "mts.h"
#include "priority_queue.h"

#define NANOSECOND_CONVERSION 1e9

// mutex, convar and global variable for loading trains 
//! need to change these a bit from loading_train.c
bool ready_to_load = false; 
pthread_mutex_t start_timer;
pthread_cond_t train_ready_to_load;
// mutex, convar and global variable for determining when dispatcher should broadcast to trians to start loading 
int ready_to_load_count = 0;
pthread_mutex_t ready_to_load_count_mutex;
pthread_cond_t all_ready_to_load;
// mutex, convar and global variable for dispatcher to know when to start scheduling and avoid busy waiting 
int ready_to_cross_count = 0;
pthread_mutex_t ready_to_cross_count_mutex;
pthread_cond_t train_available; 
// station queues and associated mutexes
priority_queue* eastbound_pq;
pthread_mutex_t epq_access;
priority_queue* westbound_pq;
pthread_mutex_t wpq_access;
// mutex for putting train on main track
pthread_mutex_t main_track;
pthread_cond_t track_free;
// global variables 
int num_trains = 0;

// initialize output file pointer globally to be used by all threads 
FILE *output_file;

struct timespec start_time = { 0 };

// Convert timespec to seconds
double timespec_to_seconds(struct timespec *ts) {
    return ((double) ts->tv_sec) + (((double) ts->tv_nsec) / NANOSECOND_CONVERSION);
}

// format output time 
char* format_output_time(double sim_seconds) {
    char* formatted_time = malloc(12);
    int hours = (int)(sim_seconds / 3600);
    int minutes = (int)((sim_seconds - (hours * 3600)) / 60);
    int seconds = (int)((sim_seconds - (hours * 3600) - (minutes * 60)));
    int tenths_of_seconds = (int)((sim_seconds - (int)(sim_seconds)) * 10);
    snprintf(formatted_time, 12, "%02d:%02d:%02d.%d", hours, minutes, seconds, tenths_of_seconds);
    return formatted_time;
}

// thread function 
void* train_thread_func(void *train) {
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

    usleep(train_object->loading_time * 1000000); //? is this the best way to do the loading time?
    clock_gettime(CLOCK_MONOTONIC, &load_time); //! whats wrong with CLOCK_MONOTONIC but its in the sample code? 
    char* formatted_time = format_output_time(timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    printf("%s Train %2d is ready to go %4s\n", formatted_time, train_object->train_no, train_object->direction);
    fprintf(output_file, "%s Train %2d is ready to go %4s\n", formatted_time, train_object->train_no, train_object->direction);

    free(formatted_time);

    // insert train into corresponding station queue 
    if (strcmp(train_object->direction, "East") == 0) {
        pthread_mutex_lock(&epq_access);
        insert(eastbound_pq, train_object);
        pthread_mutex_unlock(&epq_access); 
    } else {
        pthread_mutex_lock(&wpq_access);
        insert(westbound_pq, train_object);
        pthread_mutex_unlock(&wpq_access);
    }
    // signal to dispatcher that train is ready
    pthread_mutex_lock(&ready_to_cross_count_mutex);
    ready_to_cross_count++;
    pthread_cond_signal(&train_available);
    pthread_mutex_unlock(&ready_to_cross_count_mutex);
    
    // after receiving signal from dispatcher and mutex to cross, cross 
    pthread_mutex_lock(&train_object->main_track_mutex);
    pthread_cond_wait(&train_object->main_track_available, &train_object->main_track_mutex);
    
    // cross and write to output file 
    struct timespec cross_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &cross_time);
    formatted_time = format_output_time(timespec_to_seconds(&cross_time) - timespec_to_seconds(&start_time));
    printf("%s Train %2d is ON the main track going %4s\n", formatted_time, train_object->train_no, train_object->direction);
    fprintf(output_file, "%s Train %2d is ON the main track going %4s\n", formatted_time, train_object->train_no, train_object->direction);
    free(formatted_time);
    usleep(train_object->crossing_time * 1000000);
    clock_gettime(CLOCK_MONOTONIC, &cross_time);
    formatted_time = format_output_time(timespec_to_seconds(&cross_time) - timespec_to_seconds(&start_time));
    printf("%s Train %2d is OFF the main tack after going %4s\n", formatted_time, train_object->train_no, train_object->direction);    
    fprintf(output_file, "%s Train %2d is OFF the main tack after going %4s\n", formatted_time, train_object->train_no, train_object->direction);   
    free(formatted_time);
    pthread_mutex_unlock(&train_object->main_track_mutex);

    // signal back to dispatcher that train has crossed 
    pthread_mutex_lock(&main_track);
    pthread_cond_signal(&track_free);
    pthread_mutex_unlock(&main_track);

    pthread_exit(NULL);
}

// broadcast to start loading trains 
void start_trains() {
    pthread_mutex_lock(&start_timer);
    ready_to_load = true; 
    pthread_cond_broadcast(&train_ready_to_load);
    pthread_mutex_unlock(&start_timer);
} 

int main(int argc, char *argv[]) { 
    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        printf("Could not open file.\n");
        return 1;
    }
    
    struct train** trains_array = NULL;
    int trains_array_capacity = 10;
    trains_array = malloc(trains_array_capacity * sizeof(struct train*));
    char dir;
    float ltime;
    float ctime;
    // create the train objects and threads 
    while (fscanf(input_file, "%s %f %f", &dir, &ltime, &ctime) != EOF) {        
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
            strcpy(new_train->direction, "East");
        } else {
            strcpy(new_train->direction, "West");
        }
        new_train->loading_time = ltime/10;
        new_train->crossing_time = ctime/10;
        new_train->next = NULL;

        trains_array[num_trains] = new_train;
        num_trains++;
    }
    fclose(input_file);

    // open the output file to be written to 
    output_file = fopen("output.txt", "w");

    // trains 
    pthread_t train_thread_ids[num_trains];
    // priority queues representing stations 
    eastbound_pq = create_pq();
    westbound_pq = create_pq();

    struct timespec initial_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &initial_time);

    struct timespec create_time = { 0 };
    // create the thread for each train object
    for (size_t i = 0; i < num_trains; ++i) { //? should this be ++i or i++?
        clock_gettime(CLOCK_MONOTONIC, &create_time);
        
        // create train thread 
        if (pthread_create(&train_thread_ids[i], NULL, train_thread_func, (void *) trains_array[i])) { // (void *) (intptr_t) i
            printf("error creating train thread\n");
        }
    }    

    // use convar and mutex to wait for all trains to be ready to load 
    pthread_mutex_lock(&ready_to_load_count_mutex);
    while (ready_to_load_count != num_trains) {
        pthread_cond_wait(&all_ready_to_load, &ready_to_load_count_mutex);
    }
    pthread_mutex_unlock(&ready_to_load_count_mutex);

    // get start time 
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // start trains loading 
    start_trains();

    int remaining_trains = num_trains;
    char last_train_direction[5] = "None";
    int back_to_back_count = 0;
    struct train* main_track_train;
    while (remaining_trains > 0) {
        // check/wait for ready trains 
        pthread_mutex_lock(&ready_to_cross_count_mutex);
        while(ready_to_cross_count == 0) {            
            pthread_cond_wait(&train_available, &ready_to_cross_count_mutex);
        }        
        pthread_mutex_unlock(&ready_to_cross_count_mutex);

        // choose/signal next train to cross 
        pthread_mutex_lock(&wpq_access);
        struct train* top_west_pq = peek(westbound_pq);
        pthread_mutex_unlock(&wpq_access);
        pthread_mutex_lock(&epq_access);
        struct train* top_east_pq = peek(eastbound_pq);      
        pthread_mutex_unlock(&epq_access);
        
        // if westbound station is empty, send eastbound available train
        if (top_west_pq == NULL) {
            pthread_mutex_lock(&epq_access);
            main_track_train = pop(eastbound_pq);
            pthread_mutex_unlock(&epq_access);
        } 
        // if eastbound station is empty, send westbound available train
        else if (top_east_pq == NULL) {
            pthread_mutex_lock(&wpq_access);
            main_track_train = pop(westbound_pq);
            pthread_mutex_unlock(&wpq_access);
        }    
        // handle starvation condition 
        else if (back_to_back_count > 0) {
            if (strcmp(last_train_direction, "West") == 0) {
                pthread_mutex_lock(&epq_access);
                main_track_train = pop(eastbound_pq);
                pthread_mutex_unlock(&epq_access);
            } else {
                pthread_mutex_lock(&wpq_access);
                main_track_train = pop(westbound_pq);
                pthread_mutex_unlock(&wpq_access);
            }
        }
        // same priority
        else if (strcmp(top_east_pq->priority, top_west_pq->priority) == 0) {            
            // opposite direction 
            if (strcmp(last_train_direction, "None") == 0 || strcmp(last_train_direction, "East") == 0) { // none have crossed yet or the last was eastbound so westbound gets priority
                pthread_mutex_lock(&wpq_access);
                main_track_train = pop(westbound_pq);
                pthread_mutex_unlock(&wpq_access);
            } else { // last was westbound so eastbound gets priority 
                pthread_mutex_lock(&epq_access);
                main_track_train = pop(eastbound_pq);
                pthread_mutex_unlock(&epq_access);
            }
        }  
        // west is higher priority
        else if (strcmp(top_east_pq->priority, top_west_pq->priority) > 0) {
            pthread_mutex_lock(&wpq_access);
            main_track_train = pop(westbound_pq);
            pthread_mutex_unlock(&wpq_access);            
        }
        // east is higher priority
        else {
            pthread_mutex_lock(&epq_access);
            main_track_train = pop(eastbound_pq);
            pthread_mutex_unlock(&epq_access);            
        }
        // update back to back count 
        if (strcmp(last_train_direction, main_track_train->direction) == 0) {
            back_to_back_count++;
        } else {
            back_to_back_count = 0;
        }
        // signal the main track train to cross             
        pthread_mutex_lock(&main_track_train->main_track_mutex);
        pthread_cond_signal(&main_track_train->main_track_available);
        pthread_mutex_unlock(&main_track_train->main_track_mutex);

        // wait until receiving signal that it is finished crossing
        pthread_mutex_lock(&main_track);
        pthread_cond_wait(&track_free, &main_track);
        pthread_mutex_unlock(&main_track);

        // update last_train_direction
        strcpy(last_train_direction, main_track_train->direction);

        remaining_trains--;
    }

    fclose(output_file);

    for(size_t i = 0; i < num_trains; ++i) {
        pthread_join(train_thread_ids[i], NULL);
        free(trains_array[i]);
    }

    free(trains_array);
    return 0;
}