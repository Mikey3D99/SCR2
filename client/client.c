//
// Created by wlodi on 24.06.2023.
//
#include <unistd.h>
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


//this function has to take in arguments that will be later send to server
//no need to check for server's pid since the only way to open client is for the server to run first
int run_client(int argc, char *argv[]) {
    printf("CLIENT CRON REPLICA... (YOU NEED TO PROVIDE: exec_time, type(relative, absolute, cyclic), interval, command or program to execute, program arguments )\n");


    int msgqid = connect_to_message_queue();
    if (msgqid < 0) {
        perror("Error connecting to the message queue");
        return -1;
    }

    if (argc < 2) {
        fprintf(stderr, "Error: No operation specified\n");
        return -1;
    }

    Msg msg;
    msg.argc = argc - 1; // Exclude the "create" command itself from the argument count
    char pid_str[MAX_ARG_LEN];

    // Get the current process ID
    pid_t pid = getpid();

    // Convert the PID to a string
    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    if (strcmp(argv[1], "create") == 0) {
        // Create a new task
        if (argc < 5) {
            fprintf(stderr, "Error: Not enough arguments to create a task\n");
            return -1;
        }

        // Prepare the message
        msg.mtype = MSG_TYPE_CREATE;

        for (int i = 1; i < argc && i < MAX_ARGS; i++) {
            if(i == 1){
                strncpy(msg.argv[i-1], pid_str , MAX_ARG_LEN - 1);
                msg.argv[i-1][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination
            }
            else{
                strncpy(msg.argv[i-1], argv[i], MAX_ARG_LEN - 1);
                msg.argv[i-1][MAX_ARG_LEN - 1] = '\0'; // Ensure null termination
            }

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
        printf("display command requested\n");
        msg.mtype = MSG_TYPE_DISPLAY;
        if (msgsnd(msgqid, &msg, sizeof(Msg) - sizeof(long), 0) == -1) {
            perror("Error sending message");
            return -1;
        }

        // Wait for the response from the server
        Msg resp;
        if(msgrcv(msgqid, &resp, sizeof(Msg) - sizeof(long), MSG_TYPE_RESPONSE, 0) != -1) {
            // Print the tasks string
            printf("%s\n", resp.argv[0]);
        } else {
            perror("Error receiving response message");
            return -1;
        }

    } else if (strcmp(argv[1], "quit") == 0) {

        printf("server quit requested");

        // Send a message to trigger a server shutdown
        msg.mtype = MSG_TYPE_QUIT;
        if (msgsnd(msgqid, &msg, sizeof(Msg) - sizeof(long), 0) == -1) {
            perror("Error sending quit message\n");
            return -1;
        }

        // Wait for the response from the server
        printf("waiting for server response...");
        Msg resp;
        if(msgrcv(msgqid, &resp, sizeof(Msg) - sizeof(long), MSG_TYPE_RESPONSE, 0) != -1) {
            // Print the response message
            printf("%s\n", resp.argv[0]);
        } else {
            perror("Error receiving response message");
            return -1;
        }
    }
    else {
        fprintf(stderr, "Error: Invalid operation\n");
        return -1;
    }

    return 0;
}
