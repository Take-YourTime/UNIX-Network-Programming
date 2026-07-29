#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>


#define MAX_READ_LENGTH 1024

int main(int argc, char *argv[])
{
	int opt; // options
	int appendMode = 0; // command mode

	// get options
	while ((opt = getopt(argc, argv, "a")) != -1) {
		switch (opt) {
		    case 'a':
		        appendMode = 1;
		        break;
		    case '?':
		        printf("Usage: %s [-a] file\n", argv[0]);
		        exit(EXIT_FAILURE);
		    default:
		    	break;
		}
	}

	// check remaining arguments
	if (optind >= argc) {
		printf("Usage: %s [-a] file\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	else if (optind < argc-1){
		printf("[Waring] Exist redundant arguments!\n");
	}

	// get filename
	char *filename = argv[optind];
	
	// open file
	int fd; // file descripter
	if (appendMode == 1) // -a
		fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	else
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	
	if(fd == -1){
		printf("Open %s failure\n", filename);
		exit(EXIT_FAILURE);
	}
	
	
	int numberRead; // the input element number
	char buffer[MAX_READ_LENGTH]; // input buffer
	
	// input form standard input, and output to standard output and file
	while( (numberRead = read(STDIN_FILENO, buffer, MAX_READ_LENGTH)) )
	{
		if(numberRead == -1){
			printf("read failure: STDIN_FILENO\n");
			exit(EXIT_FAILURE);
		}
		
		if( write(STDOUT_FILENO, buffer, numberRead) < numberRead ) {
			printf("writing error!\n");
			exit(EXIT_FAILURE);
		}
		if( write(fd, buffer, numberRead) < numberRead )  {
			printf("writing error!\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}
