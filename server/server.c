//
// Created by wlodi on 24.06.2023.
//

#include <stdio.h>
#include "server.h"
#include "../task/task.h"
#include "../priority_queue/priority_queue.h"
#include <pthread.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>


#define MSG_QUEUE_KEY 1234
#define MAX_TASKS 100

typedef struct {
    PriorityQueue p_queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int msqqid;
    bool quit_flag;
} TaskBuffer;



// Function to create an empty task buffer
TaskBuffer* create_task_buffer() {
    TaskBuffer* buffer = malloc(sizeof(TaskBuffer));
    buffer->p_queue.size = 0;
    // Initialize the mutex
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        fprintf(stderr, "Error: could not initialize mutex\n");
        free(buffer);
        return NULL;
    }

    if (pthread_cond_init(&buffer->cond,NULL) != 0) {
        fprintf(stderr, "Error: could not initialize mutex\n");
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer);
        return NULL;
    }

    // Initialize msqqid (or set it to a default value)
    buffer->msqqid = -1;
    buffer->quit_flag = false;
    return buffer;
}

void destroy_task_buffer(TaskBuffer* buffer) {
    // Destroy the mutex
    pthread_mutex_destroy(&buffer->mutex);
    free_priority_queue(&buffer->p_queue);

    // Free the buffer itself
    free(buffer);
}

int initialize_message_queue(){
// Create a new message queue, or get the existing one with the specified key
    int msgqid = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget failed");
        return INIT_MSG_Q_ERR;
    }
    return msgqid;
}


Task *create_task_from_msg(Msg msg) {
    // Ensure there are enough arguments
    if (msg.argc < 5) {
        fprintf(stderr, "Error: not enough arguments in the message\n");
        return NULL;
    }

    // Variables for error checking
    char *endptr;
    errno = 0;    /* To distinguish success/failure after call */

    // Extract the id, exec_time, type, and interval
    int id = strtol(msg.argv[0], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        perror("strtol");
        return NULL;
    }

    time_t exec_time = strtol(msg.argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        perror("strtol");
        return NULL;
    }

    TaskType type = strtol(msg.argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        perror("strtol");
        return NULL;
    }

    time_t interval = strtol(msg.argv[3], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        perror("strtol");
        return NULL;
    }

    // Extract the command
    char cmd[MAX_CMD_LEN];
    strncpy(cmd, msg.argv[4], MAX_CMD_LEN);
    cmd[MAX_CMD_LEN - 1] = '\0';  // Ensure null termination

    // Extract the arguments
    char* args[MAX_ARGS] = {0};
    for (int i = 5; i < msg.argc; ++i) {
        args[i-5] = malloc(MAX_ARG_LEN * sizeof(char));
        strncpy(args[i-5], msg.argv[i], MAX_ARG_LEN - 1); // THIS MAY BE OUT OF BOUNDS
        args[i-5][MAX_ARG_LEN - 1] = '\0';  // Ensure null termination
    }

    return create_task(id, cmd, args, exec_time, type, interval);
}



void* message_listener(void* arg) {
    TaskBuffer* buffer = (TaskBuffer*)arg;

    while (true) {
        Msg msg;
        int msqqid = 0;

        pthread_mutex_lock(&buffer->mutex);
        if(buffer->quit_flag){
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }
        pthread_mutex_unlock(&buffer->mutex);

        // Receive any type of message
        if(msgrcv(msqqid, &msg, sizeof(Msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            switch(msg.mtype) {
                case MSG_TYPE_CREATE: {
                    // Create a new Task from the message
                    Task* task = create_task_from_msg(msg);
                    if(task == NULL){
                        perror("error with creating a task on server side message listener");
                        break;
                    }

                    // Add the task to the buffer and priority queue
                    pthread_mutex_lock(&buffer->mutex);
                    if (buffer->p_queue.size < MAX_TASKS) {
                        add_task(&buffer->p_queue, task);
                    }
                    pthread_mutex_unlock(&buffer->mutex);
                    break;
                }
                case MSG_TYPE_DISPLAY: {
                    // Generate a string with all tasks
                    pthread_mutex_lock(&buffer->mutex);
                    char* tasks_str = tasks_to_string(&buffer->p_queue);
                    if(tasks_str == NULL){
                        pthread_mutex_unlock(&buffer->mutex);
                        perror("error with creating a string from tasks");
                        break;
                    }
                    pthread_mutex_unlock(&buffer->mutex);

                    // Prepare a response message
                    Msg resp;
                    resp.mtype = MSG_TYPE_RESPONSE;
                    strncpy(resp.argv[0], tasks_str, MAX_ARG_LEN - 1);
                    resp.argv[0][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination
                    resp.argc = 1;

                    // Send the response message
                    if(msgsnd(msqqid, &resp, sizeof(Msg) - sizeof(long), 0) == -1) {
                        perror("Error sending response message");
                    }

                    // Free the string with tasks
                    free(tasks_str);

                    break;
                }
                case MSG_TYPE_CANCEL: {
                    // Extract task ID from the message
                    int task_id;
                    if(sscanf(msg.argv[0], "%d", &task_id) != 1) {
                        fprintf(stderr, "Error: Invalid task ID format\n");
                        break;
                    }

                    // Cancel the task
                    pthread_mutex_lock(&buffer->mutex);
                    delete_task(&buffer->p_queue, task_id);
                    pthread_mutex_unlock(&buffer->mutex);

                    // Send response message
                    Msg response;
                    response.mtype = MSG_TYPE_RESPONSE;
                    strncpy(response.argv[0], "successfully deleted", MAX_ARG_LEN - 1);
                    response.argv[0][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination
                    if(msgsnd(msqqid, &response, sizeof(Msg) - sizeof(long), 0) == -1) {
                        perror("Error sending response message");
                    }
                    break;
                }
                case MSG_TYPE_QUIT: {
                    // Set quit flag to true
                    buffer->quit_flag = true;
                    break;
                }
                default: {
                    fprintf(stderr, "Received message with unknown type: %ld\n", msg.mtype);
                    break;
                }
            }
        }
    }

    return NULL;
}

#include <string.h>

void execute_task(union sigval sv) {
    PriorityQueue* pq = sv.sival_ptr;

    // The task to execute is the first task in the priority queue
    Task* task = get_next_task(pq);

    // Initialize the command string with the command
    char cmd[MAX_CMD_LEN + MAX_ARGS * MAX_ARG_LEN] = "";
    strncat(cmd, task->cmd, MAX_CMD_LEN - 1);

    // Append each argument to the command
    for (int i = 0; i < MAX_ARGS && task->args[i] != NULL; i++) {
        strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1); // Add a space before the argument
        strncat(cmd, task->args[i], sizeof(cmd) - strlen(cmd) - 1); // Add the argument
    }

    // Execute the command
    system(cmd);

    // If the task is cyclic, adjust its exec_time and re-insert it into the queue
    if (task->type == CYCLIC) {
        task->exec_time += task->interval;
        add_task(pq, task);
    } else {
        // If the task is not cyclic, free its memory
        free(task);
    }
}


void* task_worker(void* arg) {
    TaskBuffer* buffer = (TaskBuffer*)arg;

    // Create a timer
    timer_t timer;
    struct sigevent sev = {0};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = execute_task;
    sev.sigev_value.sival_ptr = &buffer->p_queue;
    timer_create(CLOCK_REALTIME, &sev, &timer);

    while (true) {
        pthread_mutex_lock(&buffer->mutex);
        if(buffer->quit_flag){
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }

        while (buffer->p_queue.size == 0) {
            pthread_cond_wait(&buffer->cond, &buffer->mutex);
        }

        // Get the task with the earliest exec_time
        Task* task = get_next_task(&buffer->p_queue);

        pthread_mutex_unlock(&buffer->mutex);

        // Set the timer to fire at the task's exec_time
        struct itimerspec its = {0};
        its.it_value.tv_sec = task->exec_time;
        timer_settime(timer, TIMER_ABSTIME, &its, NULL);

        // Wait for the timer to fire
        pause();

        // The task has been executed by execute_task(), so we can remove it
        // Free the task
        free_task(task);
    }

    // Destroy the timer
    timer_delete(timer);

    return NULL;
}




int destroy_message_queue() {
    // Get the ID of the existing message queue
    int msgqid = msgget(MSG_QUEUE_KEY, 0);
    if (msgqid == -1) {
        perror("msgget failed");
        return -1;
    }

    // Destroy the message queue
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
        perror("msgctl failed");
        return -1;
    }

    return 0;
}

int run_server(){
    printf("SERVER CRON REPLICA...\n");


    // first initialize priority queue and message queue
    int msqgid = initialize_message_queue();
    if(!msqgid){
        perror("error initializing message queue");
        return INIT_MSG_Q_ERR;
    }


    //create the task buffer in order to launch message listener thread
    TaskBuffer * buffer = create_task_buffer();
    if(buffer == NULL){
        perror("error creating task buffer");
        return -1;
    }
    buffer->msqqid = msqgid; // assign the message queue id to task buffer

    //create message listener
    pthread_t listener_thread;
    int rc = pthread_create(&listener_thread, NULL, message_listener, (void *)buffer);
    if (rc != 0) {
        perror("Error creating listener thread");
        destroy_task_buffer(buffer);
        return -1;
    }

    //task worker
    pthread_t worker_thread;
    rc = pthread_create(&worker_thread, NULL, task_worker, (void *)buffer);
    if (rc != 0) {
        perror("Error creating listener thread");
        destroy_task_buffer(buffer);
        return -1;
    }

    // Join the listener thread (wait for it to finish)
    void* result;
    rc = pthread_join(listener_thread, &result);
    if (rc != 0) {
        perror("Error joining listener thread");
        destroy_task_buffer(buffer);
        return -1;
    }

    // Join the worker thread (wait for it to finish)
    rc = pthread_join(worker_thread, &result);
    if (rc != 0) {
        perror("Error joining listener thread");
        destroy_task_buffer(buffer);
        return -1;
    }

    // Send response message
    Msg response;
    response.mtype = MSG_TYPE_RESPONSE;
    strncpy(response.argv[0], "Server successfully shut down", MAX_ARG_LEN - 1);
    response.argv[0][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination

    if(msgsnd(buffer->msqqid, &response, sizeof(Msg) - sizeof(long), 0) == -1) {
        perror("Error sending response message");
    }

    //clean up
    ///get rid of the message queue
    if(destroy_message_queue() != 0){
        perror("Error destroying message queue in client");
        return -1;
    }
    destroy_task_buffer(buffer);


    return 0;
}
