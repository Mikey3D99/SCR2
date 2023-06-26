#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include "server/server.h"
#include "client/client.h"

#define LOCKFILE "/tmp/mylockfile"

int main(int argc, char *argv[]) {
    int pid_file = open(LOCKFILE, O_CREAT | O_RDWR, 0666);
    int rc = flock(pid_file, LOCK_EX | LOCK_NB);

    if(rc) {
        if(EWOULDBLOCK == errno){

            /// order of the commands:
            // 1. command - programme to be scheduled
            // 2. arguments for that programme that you want to be run when the time comes
            // 3. exec time - time of execution time_t
            // 4. task type - RELATIVE, ABSOLUTE, CYCLIC
            // 5. interval - 0 if the task type is other than cyclic

            run_client(argc, argv); // another instance is running, therefore make this a client process
        }
        else{
            run_server();
        }
    }

    // Remove the PID file when your program finishes.
    remove(LOCKFILE);
    return 0;
}
