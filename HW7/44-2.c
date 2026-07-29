#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#define MAX_FD_NUMBER 1024
#define BLOCK_SIZE 4096

pid_t child_pid[MAX_FD_NUMBER]; // index: fd -> value: pid

// popen()
// mode: "r" for process read the output from command, "w" for process send data to command 
FILE* popen(const char *command, const char *mode) {
	// no parameters
    if(command == NULL || mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

	// wrong mode
    if(strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0) {
        errno = EINVAL;
        return NULL;
    }
	
	int pipefd[2]; // pipefd[1]: write, pipefd[0]: read
    if(pipe(pipefd) == -1) return NULL;

	pid_t pid;
    FILE* fp;
	
	switch(pid = fork())
	{
		case -1:{
			int saved = errno;
	        close(pipefd[0]);
	        close(pipefd[1]);
	        errno = saved;
	        return NULL;
			break;
		}
		case 0:{
			/* ===== CHILD ===== */
			
			// mode detection
	        if (strcmp(mode, "r") == 0) {
	            // child writes -> parent reads
	            close(pipefd[0]);
	            if (pipefd[1] != STDOUT_FILENO) {
	                if( dup2(pipefd[1], STDOUT_FILENO) == -1){
	                	perror("dup2");
	                	_exit(127);
					}
	                close(pipefd[1]);
	            }
	        }
			else{
	            // parent writes -> child reads
	            close(pipefd[1]);
	            if (pipefd[0] != STDIN_FILENO) {
	                if( dup2(pipefd[0], STDIN_FILENO) == -1 ){
	                	perror("dup2");
	                	_exit(127);
					}
	                close(pipefd[0]);
	            }
	        }
	
	        // close the fd from other popen
	        for(int fd = 0; fd < MAX_FD_NUMBER; fd++) {
	            if(child_pid[fd] > 0)
	                close(fd);
	        }
			
			
			// execute command, /bin/sh is the shell
	        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
	        
	        // execute error!
			perror("execl");
			_exit(127); // 127 means "command cannot be executed"
		}
		default:{
			/* ===== PARENT ===== */
			int parent_end; // read-end or write-end
			
			// mode detection
		    if(strcmp(mode, "r") == 0){
		        close(pipefd[1]);
		        parent_end = pipefd[0];
		    }
			else{
		        close(pipefd[0]);
		        parent_end = pipefd[1];
		    }
		
		    fp = fdopen(parent_end, mode);
		    if(fp == NULL){
		        int saved = errno;
		        close(parent_end);
		        waitpid(pid, NULL, 0);
		        errno = saved;
		        return NULL;
		    }
			
			int fd = fileno(fp);
			if(fd < 0 || fd >= MAX_FD_NUMBER){
				int saved = errno;
			    fclose(fp);
			    waitpid(pid, NULL, 0);
			    errno = saved;
			    return NULL;
			}
		    child_pid[fd] = pid;
			break;
		}
	}
    
    return fp;
}


// pclose()
int pclose(FILE* stream) {
    int fd, status;
    pid_t pid;
	pid_t wait_return = 0; // return value of waitpid()

    if(stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    fd = fileno(stream);
    if(fd < 0 || fd >= MAX_FD_NUMBER || child_pid[fd] == 0) {
        errno = EINVAL;
        return -1;
    }

    pid = child_pid[fd];
    child_pid[fd] = 0;

    int fclose_status = fclose(stream);
	
	// wait child process
    do {
	    wait_return = waitpid(pid, &status, 0);
	} while (wait_return == -1 && errno == EINTR);
	
	
    if(wait_return == -1) return -1;
    if(fclose_status == -1) return -1; // we can add fclose error message here

    return status;
}

/* ===============================
   TEST (main)
   =============================== */
int main(void) {
    FILE *fp;
    char buf[BLOCK_SIZE];
    int status;


    printf("===== Test 1: read mode =====\n");
    fp = popen("echo hello world", "r");
    if (fp == NULL) {
        perror("popen read");
        exit(1);
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        printf("child output: %s", buf);
    }

    status = pclose(fp);
    printf("exit status = %d\n\n", WEXITSTATUS(status));



    printf("===== Test 2: write mode =====\n");
    fp = popen("cat", "w");
    if (fp == NULL) {
        perror("popen write");
        exit(1);
    }

    fprintf(fp, "line 1 from parent\n");
    fprintf(fp, "line 2 from parent\n");

    status = pclose(fp);
	printf("exit status = %d\n\n", WEXITSTATUS(status));



    printf("===== Test 3: shell command support =====\n");
    fp = popen("echo hello | tr a-z A-Z", "r");
    if (fp == NULL) {
        perror("popen shell");
        exit(1);
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        printf("child output: %s", buf);
    }

    status = pclose(fp);
    printf("exit status = %d\n\n", WEXITSTATUS(status));



    printf("===== Test 4: exit status =====\n");
    fp = popen("exit 42", "r");
    if (fp == NULL) {
        perror("popen exit");
        exit(1);
    }

    status = pclose(fp);
    printf("expected exit = 42, actual exit = %d\n\n", WEXITSTATUS(status));



    printf("===== Test 5: command not found =====\n");
    fp = popen("command_that_does_not_exist_12345", "r");
    if (fp == NULL) {
        perror("popen not found");
        exit(1);
    }

    status = pclose(fp);
    printf("expected exit = 127, actual exit = %d\n\n", WEXITSTATUS(status));



    printf("===== Test 6: multiple popen =====\n");
    FILE *fp1 = popen("echo first", "r");
    FILE *fp2 = popen("echo second", "r");

    if (fp1 == NULL || fp2 == NULL) {
        perror("popen multiple");
        exit(1);
    }

    if (fgets(buf, sizeof(buf), fp1) != NULL) {
        printf("fp1 output: %s", buf);
    }

    if (fgets(buf, sizeof(buf), fp2) != NULL) {
        printf("fp2 output: %s", buf);
    }

    status = pclose(fp1);
    printf("fp1 exit status = %d\n", WEXITSTATUS(status));

    status = pclose(fp2);
    printf("fp2 exit status = %d\n\n", WEXITSTATUS(status));



    printf("===== Test 7: wrong mode =====\n");
    fp = popen("echo test", "x");
    if (fp == NULL) {
        perror("expected error");
    } else {
        printf("ERROR: popen should have failed\n");
        pclose(fp);
    }



    printf("\n===== Test 8: pclose NULL =====\n");
    if (pclose(NULL) == -1) {
        perror("expected pclose error");
    } else {
        printf("ERROR: pclose(NULL) should have failed\n");
    }

    printf("\nAll tests finished.\n");

    return 0;
}