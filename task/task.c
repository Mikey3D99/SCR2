//
// Created by wlodi on 16.05.2023.
//
#include "task.h"

// Function to create a new task
Task* create_task(int id, char* cmd, char* args[], time_t exec_time, TaskType type, time_t interval) {
    Task* task = (Task*)malloc(sizeof(Task));

    if (task == NULL) {
        fprintf(stderr, "Error: could not allocate memory for task\n");
        return NULL;
    }

    task->id = id;
    strncpy(task->cmd, cmd, MAX_CMD_LEN-1);
    task->cmd[MAX_CMD_LEN-1] = '\0';  // Ensure null termination in case of long command

    for(int i = 0; i < MAX_ARGS && args[i] != NULL; ++i) {
        task->args[i] = strdup(args[i]);
        if(task->args[i] == NULL) {
            fprintf(stderr, "Error: could not allocate memory for task arguments\n");
            // Free previously allocated strings before returning
            for(int j = 0; j < i; ++j) {
                free(task->args[j]);
            }
            free(task);
            return NULL;
        }
    }

    task->exec_time = exec_time;
    task->type = type;
    task->interval = interval;

    return task;
}

void free_task(Task* task) {
    for(int i = 0; i < MAX_ARGS && task->args[i] != NULL; ++i) {
        free(task->args[i]);
    }
    free(task);
}

