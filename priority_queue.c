#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"
#include "mts.h"

priority_queue* create_pq() {
    priority_queue* pq = (priority_queue*)malloc(sizeof(priority_queue));
    pq->head = NULL;
    return pq;
}

int compare_train_priority(struct train* pq_train, struct train* new_train) {
    if (strcmp(pq_train->priority, new_train->priority) == 0) {
        if (pq_train->loading_time == new_train->loading_time) {
            return pq_train->train_no - new_train->train_no;
        }
        if (pq_train->loading_time < new_train->loading_time) {
            return -1;
        } else {
            return 1;
        } 
    }
    if (strcmp(pq_train->priority, new_train->priority) < 0) {
        return -1;
    } else {
        return 1;
    }
}

void insert(priority_queue* pq, struct train* train) {
    if (pq->head == NULL || compare_train_priority(pq->head, train) > 0) {
        train->next = pq->head;
        pq->head = train;
    } else {
        struct train* curr = pq->head;
        while (curr->next != NULL && compare_train_priority(curr->next, train) <= 0) {
            curr = curr->next;
        }
        train->next = curr->next;
        curr->next = train;
    }
}

struct train* pop(priority_queue* pq) {
    if (pq->head == NULL) {
        printf("Priority queue is empty\n");
        return NULL;
    }
    struct train* highest_priority_train = pq->head;
    pq->head = pq->head->next;
    return highest_priority_train;
}

struct train* peek(priority_queue* pq) {
    if (pq->head == NULL) {
        // printf("Priority queue is empty\n");
        return NULL;
    }
    return pq->head;
}

int is_empty(priority_queue* pq) {
    return pq->head == NULL;
}

//? DO I NEED ONE TO FREE THE QUEUE?