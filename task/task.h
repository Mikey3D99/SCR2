//
// Created by wlodi on 16.05.2023.
//

#ifndef SCR2_TASK_H
#define SCR2_TASK_H
#include <time.h>
#define MAX_CMD_LEN 256
#define MAX_ARGS 20
#include <malloc.h>
#include <string.h>
#include <bits/pthreadtypes.h>

#define TASK_STRING_SIZE 200
#define MAX_ARG_LEN 200
#define MSG_TYPE_CREATE 1
#define MSG_TYPE_DISPLAY 2
#define MSG_TYPE_CANCEL 3
#define MSG_TYPE_RESPONSE 4
#define MSG_TYPE_QUIT 5
typedef struct {
    long mtype;  // Message type, must be first
    int argc;
    char argv[MAX_ARGS][MAX_ARG_LEN];
} Msg;

typedef enum {
    RELATIVE,
    ABSOLUTE,
    CYCLIC
} TaskType;

extern pthread_cond_t timer_cond;  // Assuming these are defined at global scope or you can move them to a header file
extern pthread_mutex_t timer_mutex;

typedef struct {
    int id;                 // task identifier
    char cmd[MAX_CMD_LEN];  // command to execute
    char* args[MAX_ARGS];   // command arguments
    time_t exec_time;       // time of execution
    TaskType type;
    time_t interval;        // Only used for CYCLIC tasks
} Task;

Task* create_task(int id, char* cmd, char* args[], time_t exec_time, TaskType type, time_t interval);
void task_to_string(Task* task, char* output);
void free_task(Task* task);

#endif //SCR2_TASK_H
