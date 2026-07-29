#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>

#define BLOCK_SIZE 4096

int main(int argc, char *argv[])
{
	int opt; // options
	int outputLine = 10;
	
	// get options
	while ((opt = getopt(argc, argv, "n:")) != -1) {
		switch (opt) {
		    case 'n':
		    	// set outputLine by pointer optarg
		    	// WATCHOUT! optarg is effective only in this while loop
		        if(optarg){
		            outputLine = atoi(optarg);
		        }
		        else{
		        	printf("Fail to get optarg!\n");
		        	exit(EXIT_FAILURE);
		        }
		        break;
		    case '?':
		        printf("Usage: %s [-a] file\n", argv[0]);
		        exit(EXIT_FAILURE);
		    default:
		    	break;
		}
	}
	
	// check remaining arguments
	if(optind >= argc){
		printf("Usage: %s [-n <output line number>] <file name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// get filename
	char *filename = argv[optind];
	
	int fd = open(filename, O_RDONLY);
	if(fd == -1){
		printf("Open %s failure\n", filename);
		exit(EXIT_FAILURE);
	}
	
	
	int blockStart; // the start offset of current block
	int blockEnd = lseek(fd, 0, SEEK_END); // the end offset of current block
	if(blockEnd == -1){
		printf("Get blockEnd offset of %s failured\n", filename);
		close(fd);
		exit(EXIT_FAILURE);
	}
	
	int count = 0; // the number of '\n' in the reading file
	int tailStart = -1; // the starting offset of command "tail"
	int numberRead; // the input element number
	int toRead; // the number to read
	char buffer[BLOCK_SIZE]; // input buffer
	
	if((blockEnd - BLOCK_SIZE) > 0){
		blockStart = blockEnd - BLOCK_SIZE;
		toRead = BLOCK_SIZE;
	}
	else{
		blockStart = 0;
		toRead = blockEnd; // blockEnd - 0
	}
	
	
	
	while(tailStart == -1)
	{
		// move fd to the start of current block
		if(lseek(fd, blockStart, SEEK_SET) == -1){
			printf("Get blockStart offset of %s failured\n", filename);
			close(fd);
			exit(EXIT_FAILURE);
		}
	
		// read data from fd
		numberRead = read(fd, buffer, toRead);
		if(numberRead == -1){
			printf("read failure: %d\n", fd);
			close(fd);
			exit(EXIT_FAILURE);
		}
		else if(numberRead == 0 || blockStart == 0){
			// this means the row number of file may be smaller than outputLine
			// let tailStart = 0
			tailStart = 0;
		}
		
		// count the number of '\n' in the block from end to begining
		for(int i = numberRead-1; i >= 0; i--){
			if(buffer[i] == '\n'){
				// ignore the '\n' at the end of file
				if (blockStart + i == blockEnd - 1 && blockEnd == lseek(fd, 0, SEEK_END)) continue;
				
				count++;
				if(count == outputLine){
					tailStart = blockStart + i + 1;
					break;
				}
			}
		}
		
		// update block range and byte number to read
		blockEnd = blockStart;
		if((blockEnd - BLOCK_SIZE) > 0){
			blockStart = blockEnd - BLOCK_SIZE;
			toRead = BLOCK_SIZE;
		}
		else{
			blockStart = 0;
			toRead = blockEnd; // blockEnd - 0
		}
	}
	
	// move fd to tailStart
	if(lseek(fd, tailStart, SEEK_SET) == -1){
		printf("Get tailStart offset of %s failured\n", filename);
		close(fd);
		exit(EXIT_FAILURE);
	}
	
	// read file and output to standard output
	while( (numberRead = read(fd, buffer, BLOCK_SIZE)) )
	{
		if(numberRead == -1){
			printf("read failure: %d\n", fd);
			close(fd);
			exit(EXIT_FAILURE);
		}
		else if(numberRead == 0){
			break;
		}
		
		// output
		if( write(STDOUT_FILENO, buffer, numberRead) < numberRead ) {
			printf("writing error!\n");
			close(fd);
			exit(EXIT_FAILURE);
		}
	}
	
	
	close(fd);
	return 0;
}
