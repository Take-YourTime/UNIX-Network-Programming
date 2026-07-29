#include <fcntl.h>   // open()
#include <unistd.h>  // read(), write(), close()

#include <stdio.h>

#define BLOCK_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: mycat filename\n");
        return 1;
    }

    char buf[BLOCK_SIZE]; // reading buffer

    int fd = open(argv[1], O_RDONLY); // file descriptor for the file to be read
    if (fd == -1) {
        perror("open");
        return 1;
    }

    int number_read;

    while ((number_read = read(fd, buf, BLOCK_SIZE)) > 0) {
        ssize_t total_written = 0; // the number of bytes that is already written to standard output
		
		// Write data to standard output
		// Since it may not write all the data in one call, we need to loop until all data is written
        do {
            ssize_t number_write = write(STDOUT_FILENO, buf + total_written, number_read - total_written);

            if (number_write == -1) {
                perror("write");
                close(fd);
                return 1;
            }

            total_written += number_write;
        }while (total_written < number_read);
    }

    close(fd);
    return 0;
}
