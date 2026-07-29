/* 
 * pipe_command.c  :  deal with pipes
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "shell.h"

#define STD_OUTPUT 1
#define STD_INPUT  0

void pipe_and_exec(int parse_argc, char** myArgv) {
  	int pipe_argv_index = pipe_present(parse_argc, myArgv);
  	int pipefds[2];

  	switch (pipe_argv_index) {

    	case -1:	/* Pipe at beginning or at end of argv;  See pipe_present(). */
      		fputs ("Missing command next to pipe in commandline.\n", stderr);
      		errno = EINVAL;	/* Note this is NOT shell exit. */
      		break;

    	case 0:	/* No pipe found in argv array or at end of argv array.
			See pipe_present().  Exec with whole given argv array. */
      		execvp(myArgv[0], myArgv);
      		break;

    	default:	/* Pipe in the middle of argv array.  See pipe_present(). */

      		/* Split arg vector into two where the pipe symbol was found.
       		 * Terminate first half of vector.
			 *
       		 * Fill in code. */
			myArgv[pipe_argv_index] = NULL;
			
      		/* Create a pipe to bridge the left and right halves of the vector. 
			 *
			 * Fill in code. */
			if (pipe(pipefds) == -1) {
                perror("pipe");
    			exit(EXIT_FAILURE);
            }

      		/* Create a new process for the left side of the pipe.
			 * Fill in code to replace the underline. */
			
			pid_t left_pid = fork();
      		switch(left_pid) {
        		case -1 :
					perror("fork left child");
					exit(EXIT_FAILURE);

        		// left command
        		case 0 :
					// Redirect stdout to the write end of the pipe
	  				if (dup2(pipefds[1], STD_OUTPUT) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }

                    close(pipefds[0]);
                    close(pipefds[1]);

					// run the left command
                    execvp(myArgv[0], myArgv);

					// It should not run here!
					perror("execvp");
					exit(EXIT_FAILURE);

        		default :
                    break;
			}

			// Create a new process for the right side of the pipe.
			pid_t right_pid = fork();
			switch(right_pid) {
				case -1 :
					perror("fork right child");
					exit(EXIT_FAILURE);
				
				// right command
				case 0 :
					// Redirect stdin to the read end of the pipe
					if (dup2(pipefds[0], STD_INPUT) == -1) {
						perror("dup2");
						exit(EXIT_FAILURE);
					}
					close(pipefds[0]);
					close(pipefds[1]);
					
					// run the right command recursively
					pipe_and_exec(parse_argc - pipe_argv_index - 1, &myArgv[pipe_argv_index + 1]);
					
					// It should not run here!
					perror("pipe_and_exec");
					exit(EXIT_FAILURE);
				
				default :
					close(pipefds[0]);
					close(pipefds[1]);
					// wait for both child processes to finish
					waitpid(left_pid, NULL, 0);
					waitpid(right_pid, NULL, 0);
					exit(EXIT_SUCCESS);
			}
	}

	perror("Couldn't fork or exec child process");
  	exit(errno);
}
