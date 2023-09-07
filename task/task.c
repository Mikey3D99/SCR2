//
// Created by wlodi on 16.05.2023.
//
#include "task.h"
#define MAX_STRING_SIZE 1024

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
    task->timer_created = false;
    if(type != CYCLIC){
        task->interval = 0;
    }


    return task;
}

void task_to_string(Task* task, char* output) {
    // Format the execution time as a string
    char time_str[26];
    struct tm* tm_info;
    tm_info = localtime(&task->exec_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    // Get a string representation of the TaskType
    char* task_type_str;
    switch (task->type) {
        case RELATIVE:
            task_type_str = "RELATIVE";
            break;
        case ABSOLUTE:
            task_type_str = "ABSOLUTE";
            break;
        case CYCLIC:
            task_type_str = "CYCLIC";
            break;
        default:
            task_type_str = "UNKNOWN";
    }

    // Generate the output string
    snprintf(output, MAX_STRING_SIZE, "ID: %d, Command: %s, Execution Time: %s, Type: %s",
             task->id, task->cmd, time_str, task_type_str);
}


void free_task(Task* task) {
    for(int i = 0; i < MAX_ARGS && task->args[i] != NULL; ++i) {
        free(task->args[i]);
    }
    free(task);
}

