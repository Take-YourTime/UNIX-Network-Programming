#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BLOCK_SIZE 1024

/* Parse a commandline string into an argv array. */
char** parse(char* line) {

	static char delim[] = " \t\n"; /* SPACE or TAB or NL */
	int count = 0;
	char* token;
	char** newArgv = NULL;
	
	/* Nothing entered. */
	if (line == NULL || strlen(line) == 0) {
		return NULL;
	}
	
    token = strtok(line, delim);
	
	while (token != NULL) {
		// re-allocate the size of newArgv
		// the new size is count+1
	    newArgv = (char **)realloc(newArgv, (count + 1) * sizeof(char *));
	    if(newArgv == NULL){ 
			perror("Realloc failed!\n");
			exit(EXIT_FAILURE);
		}
	
	    // allocate memory space to newArgv[count]
	    // the size is the length of "token"
	    newArgv[count] = (char *)malloc(strlen(token) + 1); // the size should +1, since there is a '\0' in c string, but strlen() doesn't count it.
	    if (newArgv[count] == NULL){
			perror("Malloc failured!\n");
			exit(EXIT_FAILURE);
		}
		
		// cpoy string from token to newArgv[count]
	    strcpy(newArgv[count], token);
		
		
		// While there are more tokens...
	    count++;
	    token = strtok(NULL, delim);
	}
	
	// add NULL to the end of newArgv[], 
	// because if requirement execvp()
	newArgv = (char **)realloc(newArgv, (count + 1) * sizeof(char *));
	newArgv[count] = NULL;
	
	return newArgv;
}


/*
 * Free memory associated with argv array passed in.
 * Argv array is assumed created with parse() above.
 */
void free_argv(char** oldArgv) {
	
	if(oldArgv == NULL) return;
	
	int i = 0;

	// Free each string hanging off the array.
	// Free the oldArgv array itself.
	while(oldArgv[i] != NULL){
        free(oldArgv[i]);
        i++;
    }
	free(oldArgv);
}


int main(int argc, char* argv[])
{
	if(argc > 3){
		printf("Wrong format: ./quiz2 [-f <filename>]\n");
		exit(EXIT_FAILURE);
	}
	
	
	
	int opt; // options
	int mode = 0;
	
	// get options
	while ((opt = getopt(argc, argv, "f")) != -1) {
		switch (opt) {
			case 'f':
				mode = 1;
				break;
			case '?':
				printf("Option error\n");
				exit(EXIT_FAILURE);
		    default:
		    	break;
		}
	}
	
	char* outputFile = NULL; // output file name
	if(mode == 1){
		outputFile = argv[optind];
	}

	int parentToChild[2]; // ship data from parent process to child process
	int childToParent[2]; // ship data from child process to parent process
	
	if(pipe(parentToChild) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if(pipe(childToParent) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	
	
	pid_t pid;
	char buf[BLOCK_SIZE+1]; // commander IO buffer
	char** childArgv = NULL;
	int status;
	
	// commander, 
	// read command from terminal, 
	// and send the command to executer by channel "parentToChild".
	printf("[Commander] Input command: ");
	fgets(buf, BLOCK_SIZE, stdin);
	if( write(parentToChild[1], buf, strlen(buf)+1) < strlen(buf) ){
		printf("write buf to parentToChild[1] failed\n");
		exit(EXIT_FAILURE);	
	}
	
	switch(pid = fork())
	{
		case -1:
			perror("fork");
			exit(EXIT_FAILURE);
			break;
		
		/* child, executer */
		case 0:{
			close(parentToChild[1]);
			close(childToParent[0]);
			
			
			char readBuffer[BLOCK_SIZE+1]; // executer IO buffer
			int numberRead;
			
			// get command form commander
			if((numberRead = read(parentToChild[0], readBuffer, BLOCK_SIZE)) <= 0){
				printf("read error\n");
				exit(EXIT_FAILURE);
			}
			close(parentToChild[0]);
			
			readBuffer[numberRead] = '\0'; // add \0 at the end of readBuffer[]
			printf("[Executer] Get command: %s", readBuffer);
			
			
			childArgv = parse(readBuffer); // parse command
			
			// copy childToParent[1] to STDOUT_FILENO for 
			printf("[Executer] Execute command %s\n", readBuffer);
			if(childToParent[1] != 0){
				dup2(childToParent[1], STDOUT_FILENO);
				close(childToParent[1]);
			}
			
			// execute command
			execvp(childArgv[0], childArgv);
			
			// if execvp() run successfully, here will not be run
			printf("execvp error\n");
			exit(EXIT_FAILURE);
		};
		
		/* parent, commander */
		default:
			close(parentToChild[0]);
			close(parentToChild[1]);
			close(childToParent[1]);
			
			int numberRead = 0; // the input character number
			if(mode == 0){
				printf("[Commander] Start print the command executing result\n");
				
				while( (numberRead = read(childToParent[0], buf, BLOCK_SIZE)) )
				{
					if(numberRead == -1){
						printf("Read failed\n");
						exit(EXIT_FAILURE);
					}
					
					if(numberRead > 0) {
						buf[numberRead] = '\0';
						write(STDOUT_FILENO, buf, numberRead);
					}
				}
				
				printf("[Commander] Finish\n");
			}
			else if(mode == 1){
				printf("[Commander] Start print the command executing result to file %s\n", outputFile);
				
				int fd = open(outputFile,  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
				if(fd == -1){
					printf("Open file %s failed", outputFile);
					exit(EXIT_FAILURE);
				}
				
				while( (numberRead = read(childToParent[0], buf, BLOCK_SIZE)) )
				{
					if(numberRead == -1){
						printf("Read failed\n");
						exit(EXIT_FAILURE);
					}
					
					if(numberRead > 0) {
						buf[numberRead] = '\0';
						write(fd, buf, numberRead);
					}
				}
				
				printf("[Commander] Finish print to %s\n", outputFile);
			}
			close(childToParent[0]);
			
			waitpid(pid, &status, 0);
			break;
	}
	
	free_argv(childArgv);
	return 0;
}

