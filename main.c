#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h> // Include the header for file and folder operations
#include "server/server.h"
#include "client/client.h"

#define LOCKFILE "/home/michaubob/CLionProjects/SCR2real/tmp/mylockfile"

// Function to create a directory if it doesn't exist
int createDirectory(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        if (mkdir(path, 0777) != 0) {
            perror("Error creating directory");
            return 0;
        }
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (!createDirectory("/home/michaubob/CLionProjects/SCR2real/tmp")) {
        return 1;
    }

    int pid_file = open(LOCKFILE, O_CREAT | O_RDWR, 0666);
    if (pid_file == -1) {
        perror("Error opening lock file");
        return 1;
    }

    int rc = flock(pid_file, LOCK_EX | LOCK_NB);

    if (rc == 0) { // Successfully acquired lock, become the server
        printf("Server running\n");
        run_server(); // Run server logic
        remove(LOCKFILE);
        printf("Lock file removed\n");
    } else {
        printf("Client running\n");
        run_client(argc, argv); // Run client logic
    }


    return 0;
}

