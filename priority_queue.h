#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

struct train;

typedef struct {
    struct train* head;
} priority_queue;

priority_queue* create_pq();
void insert(priority_queue* pq, struct train* train);
struct train* pop(priority_queue* pq);
struct train* peek(priority_queue* pq);
int is_empty(priority_queue* pq);

#endif