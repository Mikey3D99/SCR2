//
// Created by wlodi on 24.06.2023.
//
#include "client.h"
#include "../task/task.h"

#define MSG_QUEUE_KEY 1234
#define MAX_ARG_LEN 100

int connect_to_message_queue() {
    // Get the ID of the existing message queue
    int msgqid = msgget(MSG_QUEUE_KEY, 0);
    if (msgqid == -1) {
        perror("msgget failed");
        return -1;
    }

    return msgqid;
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

typedef struct {
    long mtype;  // Message type, must be first
    int argc;
    char argv[MAX_ARGS][MAX_ARG_LEN];
} Msg;

//this function has to take in arguments that will be later send to server
//no need to check for server's pid since the only way to open client is for the server to run first
int run_client(int argc, char *argv[]) {
    printf("CLIENT CRON REPLICA...\n");

    int msgqid = connect_to_message_queue();
    if (msgqid < 0) {
        perror("Error connecting to the message queue");
        return -1;
    }

    // Create the message
    Msg msg;
    msg.mtype = 1; // Message type
    msg.argc = argc;
    for (int i = 0; i < argc && i < MAX_ARGS; i++) {
        strncpy(msg.argv[i], argv[i], MAX_ARG_LEN - 1);
        msg.argv[i][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination
    }

    // Send the message
    if (msgsnd(msgqid, &msg, sizeof(Msg) - sizeof(long), 0) == -1) {
        perror("Error sending message");
        return -1;
    }

    return 0;
}
