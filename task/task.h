//
// Created by wlodi on 16.05.2023.
//

#ifndef SCR2_TASK_H
#define SCR2_TASK_H
#include <time.h>
#define MAX_CMD_LEN 256
#define MAX_ARGS 10
#include <malloc.h>
#include <string.h>

typedef enum {
    RELATIVE,
    ABSOLUTE,
    CYCLIC
} TaskType;

typedef struct {
    int id;                 // task identifier
    char cmd[MAX_CMD_LEN];  // command to execute
    char* args[MAX_ARGS];   // command arguments
    time_t exec_time;       // time of execution
    TaskType type;
    time_t interval;        // Only used for CYCLIC tasks
} Task;

Task* create_task(int id, char* cmd, char* args[], time_t exec_time, TaskType type, time_t interval);
void free_task(Task* task);

#endif //SCR2_TASK_H
