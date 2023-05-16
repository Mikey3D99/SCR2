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

#endif //SCR2_PRIORITY_QUEUE_H
