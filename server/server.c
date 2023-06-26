//
// Created by wlodi on 24.06.2023.
//

#include <stdio.h>
#include "server.h"
#include "../task/task.h"
#include <sys/msg.h>
#include <pthread.h>

#define MSG_QUEUE_KEY 1234
#define MAX_TASKS 100

typedef struct {
    Task* tasks[MAX_TASKS];
    int size;
    pthread_mutex_t mutex;
} TaskBuffer;

int initialize_message_queue(){
// Create a new message queue, or get the existing one with the specified key
    int msgqid = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget failed");
        return INIT_MSG_Q_ERR;
    }
    return msgqid;
}

void* message_listener(void* arg) {
    TaskBuffer* buffer = (TaskBuffer*)arg;

    while (1) {
        Msg msg;
        if(msgrcv(msgqid, &msg, sizeof(Msg) - sizeof(long), 0, 0) != -1) {
            // Create a new Task from the message
            Task* task = create_task_from_msg(msg);

            // Add the task to the buffer
            pthread_mutex_lock(&buffer->mutex);
            if (buffer->size < MAX_TASKS) {
                buffer->tasks[buffer->size++] = task;
            }
            pthread_mutex_unlock(&buffer->mutex);
        }
    }

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
    if(!initialize_message_queue()){
        perror("error initializing message queue");
        return INIT_MSG_Q_ERR;
    }

    //clean up
    ///get rid of the message queue
    if(destroy_message_queue() != 0){
        perror("Error destroying message queue in client");
        return -1;
    }
    return 0;
}
