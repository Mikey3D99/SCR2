#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>

#define LOCKFILE "/tmp/mylockfile"

int main() {
    int pid_file = open(LOCKFILE, O_CREAT | O_RDWR, 0666);
    int rc = flock(pid_file, LOCK_EX | LOCK_NB);

    if(rc) {
        if(EWOULDBLOCK == errno)
            exit(EXIT_FAILURE); // another instance is running, therefore make this a client process
    }

    /* Continue your program here. */

    // Remove the PID file when your program finishes.
    remove(LOCKFILE);
    return 0;
}
