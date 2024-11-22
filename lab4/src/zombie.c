#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        printf("Child process (PID: %d) exiting...\n", getpid());
        exit(0); // завершаем дочерний процесс
    } else {
        // родительский процесс
        printf("Parent process (PID: %d) sleeping for 10 seconds...\n", getpid());
        sleep(10);
        printf("Parent process calling wait()...\n");
        wait(NULL);
        printf("Parent process exiting...\n");
    }

    return 0;
}
