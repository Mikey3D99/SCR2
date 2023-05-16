#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>

#define LOCKFILE "/tmp/mylockfile"

void run_client(){
    printf("Client code...");
}
void run_server(){

}
int main(int argc, char *argv[]) {
    int pid_file = open(LOCKFILE, O_CREAT | O_RDWR, 0666);
    int rc = flock(pid_file, LOCK_EX | LOCK_NB);

    if(rc) {
        if(EWOULDBLOCK == errno){
            run_client(); // another instance is running, therefore make this a client process
        }
        else{
            run_server();
        }
    }

    // Remove the PID file when your program finishes.
    remove(LOCKFILE);
    return 0;
}
