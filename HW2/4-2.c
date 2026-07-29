#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_READ_LENGTH 1024

int main(int argc, char *argv[])
{
	if(argc < 3){
		printf("Too less arguments!\n");
		exit(EXIT_FAILURE);
	}

	int opt; // options
	
	// get options
	while ((opt = getopt(argc, argv, "a")) != -1) {
		switch (opt) {
		    case '?':
		        printf("Wrong command option!\n");
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
	else if (optind < argc-2){
		printf("[Waring] Exist redundant arguments!\n");
	}

	// get filename
	char *filename1 = argv[optind];
	char *filename2 = argv[optind+1];
	
	// open file
	int fd1, fd2; // file descripter, read fd1 and copy to fd2
	fd1 = open(filename1, O_RDONLY);
	if(fd1 == -1){
		printf("Open %s failure\n", filename1);
		exit(EXIT_FAILURE);
	}
	
	fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if(fd2 == -1){
		printf("Open %s failure\n", filename2);
		exit(EXIT_FAILURE);
	}
	
	
	const int end = lseek(fd1, 0, SEEK_END); // the end offset of reading file
	if(end == -1){
		printf("Get end offste of %s failured\n", filename1);
		close(fd1);
		close(fd2);
		exit(EXIT_FAILURE);
	}
	
	int data; // current data offset
	int hole = 0; // hole offset
	
	int numberRead; // the input element number
	char buffer[MAX_READ_LENGTH]; // input buffer
	
	lseek(fd1, 0, SEEK_SET); // reset fd1 to file head
	while(1)
	{
		// find the starting offset of a data sequence
		data = lseek(fd1, hole, SEEK_DATA);
		if(data == -1){
			if(errno == ENXIO) break; // all the data is copied, end program~
			
			printf("Get data offset of %s failured\n", filename1);
			exit(EXIT_FAILURE);
		}
		
		// find the next hole offset
		hole = lseek(fd1, data, SEEK_HOLE);
		if(hole == -1){
			printf("Get hole offset of %s failured\n", filename1);
			close(fd1);
			close(fd2);
			exit(EXIT_FAILURE);
		}
		
		// reset data offset
		if(lseek(fd1, data, SEEK_SET) == -1){
			printf("Reset data offset of %s failured\n", filename1);
			close(fd1);
			close(fd2);
			exit(EXIT_FAILURE);
		}
		
		// synchronous fd2, which means skip the hole in file1 
		if(lseek(fd2, data, SEEK_SET) == -1) {
		    printf("Update fd2 offset of %s failured\n", filename2);
			close(fd1);
			close(fd2);
			exit(EXIT_FAILURE);
		}
		
		while( data < hole )
		{
			int toRead = (hole - data) > MAX_READ_LENGTH ? MAX_READ_LENGTH : (hole - data);
		
			// read data from fd1
			numberRead = read(fd1, buffer, toRead);
			if(numberRead == -1){
				printf("read failure: STDIN_FILENO\n");
				close(fd1);
				close(fd2);
				exit(EXIT_FAILURE);
			}
			else if(numberRead == 0) break;			
			
			// copy data to fd2
			if( write(fd2, buffer, numberRead) < numberRead ) {
				printf("writing error!\n");
				close(fd1);
				close(fd2);
				exit(EXIT_FAILURE);
			}
			
			// update current data position
			data += numberRead;
		}
	}
	
	// move the end offset if there is a hole in the end of file
	ftruncate(fd2, end);
	
	close(fd1);
	close(fd2);
	return 0;
}

