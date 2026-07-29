#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BLOCK_SIZE 4096

int main(void) {
    // Create a pipe
    int p[2];
    if (pipe(p) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    switch(pid_t pid = fork()){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        case 0: // child process
            close(p[0]);

            // Redirect stdout to the write-end of the pipe
            if (dup2(p[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            close(p[1]);

            execlp("ls", "ls", "-l", (char *)NULL);

            // It should not reach here if execlp is successful
            perror("execlp");
            exit(EXIT_FAILURE);

        default: // parent process
            close(p[1]);

            char buffer[BLOCK_SIZE];
            ssize_t number_read;
            
            // read from the read-end of the pipe and write to standard output
            while ((number_read = read(p[0], buffer, sizeof(buffer))) > 0) {
                if (write(STDOUT_FILENO, buffer, number_read) == -1) {
                    perror("write");
                    close(p[0]);
                    exit(EXIT_FAILURE);
                }
            }

            if (number_read < 0) {
                perror("read");
                close(p[0]);
                exit(EXIT_FAILURE);
            }

            close(p[0]);
            wait(NULL);
            break;
    }
    return 0;
}
