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
#include "../logging_system.h"


#define MSG_QUEUE_KEY 1232
#define MAX_TASKS 100

typedef struct {
    PriorityQueue p_queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int msqqid;
    bool quit_flag;
} TaskBuffer;

char * current_state_dump;



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
    pthread_cond_destroy(&buffer->cond);
    free_priority_queue(&buffer->p_queue);

    // Free the buffer itself
    //free(buffer);
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
    Msg msg;

    pthread_mutex_lock(&buffer->mutex);
    int msqqid = buffer->msqqid;
    pthread_mutex_unlock(&buffer->mutex);

    while (true) {
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
                        // Signal the condition variable since a new task has been added
                        pthread_cond_signal(&buffer->cond);
                    }
                    pthread_mutex_unlock(&buffer->mutex);
                    break;
                }
                case MSG_TYPE_DISPLAY: {


                    // Generate a string with all tasks
                    pthread_mutex_lock(&buffer->mutex);
                    if(current_state_dump != NULL){
                        free(current_state_dump);
                        current_state_dump = NULL;
                    }
                    char * state_dump = tasks_to_string(&buffer->p_queue);
                    log_message(MIN, state_dump);
                    current_state_dump = state_dump;

                    char * tasks_str = tasks_to_string(&buffer->p_queue);
                    if(tasks_str == NULL){
                        pthread_mutex_unlock(&buffer->mutex);
                        perror("error with creating a string from tasks");
                        break;
                    }
                    pthread_mutex_unlock(&buffer->mutex);

                    // Prepare a response message
                    Msg resp;
                    resp.mtype = MSG_TYPE_RESPONSE;
                    if(buffer->p_queue.size > 0){
                        strncpy(resp.argv[0], tasks_str, MAX_ARG_LEN - 1);
                    }
                    else{
                        strncpy(resp.argv[0], "\nResponse from server: no tasks currently scheduled", MAX_ARG_LEN - 1);
                    }

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
                    printf("Server quit initialized\n");
                    fflush(stdout);
                    pthread_mutex_lock(&buffer->mutex);
                    buffer->quit_flag = true;
                    pthread_cond_signal(&buffer->cond);
                    pthread_cond_signal(&timer_cond);
                    pthread_mutex_unlock(&buffer->mutex);
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
#include <spawn.h>
#include <sys/wait.h>

pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
timer_t active_timers[MAX_TASKS];
int timer_count = 0;
pthread_mutex_t timers_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_timer_to_list(timer_t timer) {
    pthread_mutex_lock(&timers_mutex);
    if (timer_count < MAX_TASKS) {
        active_timers[timer_count++] = timer;
    }
    // Handle else case, maybe by dynamically resizing, logging an error, etc.
    pthread_mutex_unlock(&timers_mutex);
}

void remove_timer_from_list(timer_t timer) {
    pthread_mutex_lock(&timers_mutex);

    int index_to_remove = -1;
    for (int i = 0; i < timer_count; i++) {
        if (active_timers[i] == timer) {
            index_to_remove = i;
            break;
        }
    }

    // If the timer was found in the list
    if (index_to_remove != -1) {
        // Shift timers in the array to the left to fill the gap
        for (int i = index_to_remove; i < timer_count - 1; i++) {
            active_timers[i] = active_timers[i + 1];
        }

        // Decrease the timer count
        timer_count--;
    }
    // Else, maybe handle the case where the timer is not found, log an error, etc.

    pthread_mutex_unlock(&timers_mutex);
}


void delete_all_timers() {
    pthread_mutex_lock(&timers_mutex);
    for (int i = 0; i < timer_count; i++) {
        timer_delete(active_timers[i]);
    }
    timer_count = 0;  // Reset the timer count
    pthread_mutex_unlock(&timers_mutex);
}


void execute_task(union sigval sv) {

    TaskBuffer * buffer = sv.sival_ptr;
    pthread_mutex_lock(&buffer->mutex);
    printf("ENTERED EXECUTE; pq size:[%d][%p]", buffer->p_queue.size, &buffer->p_queue);
    fflush(stdout);
    // The task to execute is the first task in the priority queue
    Task* task = get_next_task(&buffer->p_queue);
    if (task == NULL) {
        // Handle error
        pthread_mutex_unlock(&buffer->mutex);
        printf("TASK IS NULL");
        fflush(stdout);
        return;
    }

    printf("ENTERED EXECUTE: task id: %d", task->id);
    fflush(stdout);
    pthread_mutex_unlock(&buffer->mutex);

    // Here, we're assuming task->cmd is the command to run and task->args is a NULL-terminated array of arguments
    char* cmd = task->cmd;
    char* const* args = task->args;
    pid_t pid;
    // Launch the process
    if (posix_spawn(&pid, cmd, NULL, NULL, args, NULL) != 0) {
        perror("posix_spawn failed");
    }

    // If the task is cyclic, adjust its exec_time and re-insert it into the queue
    if (task->type == CYCLIC) {
        pthread_mutex_lock(&buffer->mutex);
        task->exec_time += task->interval;
        add_task(&buffer->p_queue, task);
        pthread_mutex_unlock(&buffer->mutex);
    } else {
        pthread_mutex_lock(&buffer->mutex);
        delete_task(&buffer->p_queue, task->id);
        pthread_mutex_unlock(&buffer->mutex);
        free_task(task);
    }
    // Now that we're done executing the task, delete its timer
    timer_t task_timer = *((timer_t*)task->timer);

    pthread_mutex_lock(&timers_mutex);
    remove_timer_from_list(task_timer);
    pthread_mutex_unlock(&timers_mutex);

    timer_delete(task_timer);
}

void* task_worker(void* arg) {
    TaskBuffer* buffer = (TaskBuffer*)arg;

    while (true) {
        pthread_mutex_lock(&buffer->mutex);
        if(buffer->quit_flag){
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }

        // Wait if the queue is empty.
        while (buffer->p_queue.size == 0 && !buffer->quit_flag) {
            pthread_cond_wait(&buffer->cond, &buffer->mutex);
        }
        if (buffer->quit_flag) {
            break;
        }


        // Get the task with the earliest exec_time
        Task* task = peek_next_task(&buffer->p_queue);
        if (!task) {
            perror("No task retrieved from the queue.\n");
            continue;
        }

        pthread_mutex_unlock(&buffer->mutex);

        // Create a timer for this task
        timer_t timer;
        struct sigevent sev = {0};
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = execute_task;
        sev.sigev_value.sival_ptr = buffer;

        if (timer_create(CLOCK_REALTIME, &sev, &timer) == -1) {
            perror("timer_create");
            return NULL;
        }
        printf("pqsize in task worker");
        fflush(stdout);
        pthread_mutex_lock(&buffer->mutex);
        task->timer_created = true;
        task->timer = timer;
        pthread_mutex_unlock(&buffer->mutex);



        // Add created timer to the list
        // TODO: THIS SHIT BLOCKS THE CODE
        add_timer_to_list(timer);
        printf("pqsize in task worker");
        fflush(stdout);
        // Set the timer to fire at the absolute exec_time
        struct itimerspec its = {0};

        pthread_mutex_lock(&buffer->mutex);
        its.it_value.tv_sec = task->exec_time;
        pthread_mutex_unlock(&buffer->mutex);


        if (timer_settime(timer, TIMER_ABSTIME, &its, NULL) == -1) {
            perror("timer_settime");
            printf("error settime");
            fflush(stdout);
            free_task(task);
            return NULL;
        }
        printf("pqsize in task worker");
        fflush(stdout);
    }

    //delete all created timers
    pthread_mutex_lock(&timers_mutex);
    delete_all_timers();
    pthread_mutex_unlock(&timers_mutex);

    pthread_mutex_destroy(&timers_mutex);
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


void pqueue_state_callback(const char * dump_data){
    dump_data = current_state_dump;
}

int run_server(){
    printf("SERVER CRON REPLICA...\n");


    // first initialize priority queue and message queue
    int msqgid = initialize_message_queue();
    if(msqgid == INIT_MSG_Q_ERR){
        perror("error initializing message queue");
        return INIT_MSG_Q_ERR;
    }

    //initiate the state dump
    current_state_dump = NULL;


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

    //initiate logging
    log_level_t lvl = MIN;
    init_logging("log_file", lvl, pqueue_state_callback);


    //-----------------------------------------------------------

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
    close_logging();

    return 0;
}
