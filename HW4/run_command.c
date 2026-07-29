/*
 * run_command.c :    do the fork, exec stuff, call other functions
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include "shell.h"

#define BLOCK_SIZE 4096

void run_command(char** myArgv) {
    if(myArgv == NULL) return;
    
    pid_t pid;
    int stat;
    
    
    /* Create a new child process.
     * Fill in code.
	 */
	
	int back = is_background(myArgv); // check if the command should run in background or not
	char buffer[BLOCK_SIZE];

	fflush(NULL);
    switch (pid = fork()) {

        /* Error. */
        case -1 :
            perror("fork");
            exit(errno);

        /* Parent. */
        default :
            /* Wait for child to terminate.
             * Fill in code.
			 */
			
			if(!back){
				// foreground
				waitpid(pid, &stat, 0);
			}
			else{
				// background
				printf("[Background process PID: %d]\n", pid);
				return;
			}
			
            /* Optional: display exit status.  (See wstat(5).)
             * Fill in code.
			 */
			 
			int len = snprintf(buffer, sizeof(buffer), "( Process %d exit status = %d )\n", pid, stat); // turn %d into string
			
			// output status message
			if( len > 0 && len <= sizeof(buffer) ){
				if(write(STDOUT_FILENO, buffer, len) != len){
					perror("Write failed!\n");
				}
			}
            return;

        /* Child. */
        case 0 :
            /* Run command in child process.
             * Fill in code.
			 */
			if( execvp(myArgv[0], myArgv) == -1 ){
				perror("Execvp failed!\n");
			}

            /* Handle error return from exec */
			exit(errno);
    }
}
