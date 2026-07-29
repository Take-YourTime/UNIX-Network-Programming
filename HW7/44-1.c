#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BLOCK_SIZE 1024


int main()
{
	int parentToChild[2]; // transfer data from parent process to child process
	int childToParent[2]; // transfer data from child process to parent process
	
	// Open pipe 
	if(pipe(parentToChild) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if(pipe(childToParent) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	
	pid_t pid;
	int stdNumberRead, pipeNumberRead;
	char stdBuffer[BLOCK_SIZE]; // for std read
	char pipeBuffer[BLOCK_SIZE]; // for pipe read
	
	const char childTurnMessage[] = "[child turn text to lowercase]\n";
	
	switch( pid = fork() )
	{
		case -1:{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		// child
		case 0:{
			close(parentToChild[1]);
			close(childToParent[0]);
			
			// read text form parent
			while( (pipeNumberRead = read(parentToChild[0], pipeBuffer, BLOCK_SIZE)) )
			{
				
				if(pipeNumberRead < 0){
					perror("child read");
					exit(EXIT_FAILURE);
				}
				if(pipeNumberRead > 0){
					
					// turn to lowercase
					write(STDOUT_FILENO, childTurnMessage, strlen(childTurnMessage)+1);
					for(int i = 0; i < strlen(pipeBuffer); i++){
						pipeBuffer[i] = tolower(pipeBuffer[i]);
					}
					
					// send text to parent
					if( write(childToParent[1], pipeBuffer, pipeNumberRead) < pipeNumberRead){
						perror("transfer to parent");
						exit(EXIT_FAILURE);
					}
				}
			}
			
			
			close(parentToChild[0]);
			close(childToParent[1]);
			break;
		}
		// parent
		default:{
			close(parentToChild[0]);
			close(childToParent[1]);
			
			// read text form STDIN
			while( (stdNumberRead = read(STDIN_FILENO, stdBuffer, BLOCK_SIZE)) )
			{
				if(stdNumberRead < 0){
					perror("std read");
					exit(EXIT_FAILURE);
				}
				if(stdNumberRead > 0){
					// send text to child
					if( write(parentToChild[1], stdBuffer, stdNumberRead) < stdNumberRead){
						perror("transfer to child");
						exit(EXIT_FAILURE);
					}
					
					// get lowercase text from child
					while( (pipeNumberRead = read(childToParent[0], pipeBuffer, BLOCK_SIZE)) ){
						if(stdNumberRead < 0){
							perror("parent pipe read");
							exit(EXIT_FAILURE);
						}
						
						if(pipeNumberRead > 0){
							// print the lowercase text to STDOUT
							write(STDOUT_FILENO, "[parent get] \0", 13);
							write(STDOUT_FILENO, pipeBuffer, pipeNumberRead);
							break;
						}
					}
				}
			}
			close(parentToChild[1]);
			close(childToParent[0]);
			break;
		}
	}
	
	
	return 0;
}

