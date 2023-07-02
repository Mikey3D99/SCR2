//
// Created by wlodi on 24.06.2023.
//
#include "client.h"
#include "../task/task.h"

#define MSG_QUEUE_KEY 1234


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

//this function has to take in arguments that will be later send to server
//no need to check for server's pid since the only way to open client is for the server to run first
int run_client(int argc, char *argv[]) {
    printf("CLIENT CRON REPLICA...\n");


    int msgqid = connect_to_message_queue();
    if (msgqid < 0) {
        perror("Error connecting to the message queue");
        return -1;
    }

    if (argc < 2) {
        fprintf(stderr, "Error: No operation specified\n");
        return -1;
    }

    if (strcmp(argv[1], "create") == 0) {
        // Create a new task
        if (argc < 6) {
            fprintf(stderr, "Error: Not enough arguments to create a task\n");
            return -1;
        }

        // Prepare the message
        Msg msg;
        msg.mtype = MSG_TYPE_CREATE;
        msg.argc = argc - 1; // Exclude the "create" command itself from the argument count
        for (int i = 1; i < argc && i < MAX_ARGS; i++) {
            strncpy(msg.argv[i-1], argv[i], MAX_ARG_LEN - 1);
            msg.argv[i-1][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination
        }

        // Send the message
        if (msgsnd(msgqid, &msg, sizeof(Msg) - sizeof(long), 0) == -1) {
            perror("Error sending message");
            return -1;
        }

    } else if (strcmp(argv[1], "display") == 0) {
        // Display all tasks
        // Send a message to request all tasks
        // Wait for and handle the response
    } else if (strcmp(argv[1], "cancel") == 0) {
        // Cancel a task
        if (argc < 3) {
            fprintf(stderr, "Error: No task ID specified to cancel\n");
            return -1;
        }
        // Send a message to cancel task
    } else {
        fprintf(stderr, "Error: Invalid operation\n");
        return -1;
    }

    return 0;
}
