//
// Created by wlodi on 16.05.2023.
//

#ifndef SCR2_PRIORITY_QUEUE_H
#define SCR2_PRIORITY_QUEUE_H

#include "../task/task.h"

#define MAX_TASKS 100  // Maximum number of tasks

typedef struct {
    Task* tasks[MAX_TASKS];  // Array of tasks
    int size;  // Current number of tasks
} PriorityQueue;

PriorityQueue* create_priority_queue();
void add_task(PriorityQueue* pq, Task* task);
void sift_down(PriorityQueue* pq, int start);
Task* get_next_task(PriorityQueue* pq);
void free_priority_queue(PriorityQueue* pq);

#endif //SCR2_PRIORITY_QUEUE_H
