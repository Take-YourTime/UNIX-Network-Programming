#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIFO_PATH "/tmp/nonblock_test_fifo"

int main(void)
{
    int readFd;
    int writeFd;
    char buf[100];
    ssize_t numRead;
    ssize_t numWrite;

    unlink(FIFO_PATH);

    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    printf("FIFO created: %s\n\n", FIFO_PATH);

    /*
       Test 1:
       Open FIFO for reading with O_NONBLOCK.
       No writer exists, but open() should succeed immediately.
    */
    printf("[Test 1] open read-end with O_NONBLOCK, no writer\n");

    readFd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (readFd == -1) {
        perror("open read-end");
    } else {
        printf("Success: read-end opened immediately.\n");
    }

    /*
       Test 2:
       Nonblocking read when no data exists.
       Since no writer exists, read() returns 0, meaning EOF.
    */
    printf("\n[Test 2] nonblocking read, no writer\n");

    numRead = read(readFd, buf, sizeof(buf));
    if (numRead == 0) {
        printf("Success: read() returned 0, EOF because no writer exists.\n");
    } else if (numRead == -1) {
        printf("read() failed: errno=%d (%s)\n", errno, strerror(errno));
    } else {
        printf("read() returned %zd bytes.\n", numRead);
    }

    close(readFd);

    /*
       Test 3:
       Open FIFO for writing with O_NONBLOCK.
       No reader exists, so open() should fail with ENXIO.
    */
    printf("\n[Test 3] open write-end with O_NONBLOCK, no reader\n");

    writeFd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (writeFd == -1) {
        if (errno == ENXIO) {
            printf("Success: open write-end failed with ENXIO.\n");
        } else {
            printf("open write-end failed: errno=%d (%s)\n",
                   errno, strerror(errno));
        }
    } else {
        printf("Unexpected: write-end opened successfully.\n");
        close(writeFd);
    }

    /*
       Test 4:
       Open read-end first, then open write-end.
       Both use O_NONBLOCK.
    */
    printf("\n[Test 4] open read-end and write-end with O_NONBLOCK\n");

    readFd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (readFd == -1) {
        perror("open read-end");
        unlink(FIFO_PATH);
        exit(EXIT_FAILURE);
    }

    writeFd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (writeFd == -1) {
        perror("open write-end");
        close(readFd);
        unlink(FIFO_PATH);
        exit(EXIT_FAILURE);
    }

    printf("Success: both read-end and write-end opened.\n");

    /*
       Test 5:
       Nonblocking read when writer exists but no data.
       This should fail with EAGAIN.
    */
    printf("\n[Test 5] nonblocking read, writer exists but no data\n");

    numRead = read(readFd, buf, sizeof(buf));
    if (numRead == -1 && errno == EAGAIN) {
        printf("Success: read() failed with EAGAIN because no data exists.\n");
    } else if (numRead == 0) {
        printf("Unexpected: read() returned EOF.\n");
    } else if (numRead > 0) {
        printf("Unexpected: read() returned %zd bytes.\n", numRead);
    } else {
        printf("read() failed: errno=%d (%s)\n", errno, strerror(errno));
    }

    /*
       Test 6:
       Write data, then read it back.
    */
    printf("\n[Test 6] write data and read it back\n");

    numWrite = write(writeFd, "hello", 5);
    if (numWrite == -1) {
        perror("write");
    } else {
        printf("write() wrote %zd bytes.\n", numWrite);
    }

    numRead = read(readFd, buf, sizeof(buf) - 1);
    if (numRead == -1) {
        perror("read");
    } else {
        buf[numRead] = '\0';
        printf("read() got %zd bytes: %s\n", numRead, buf);
    }

    close(readFd);
    close(writeFd);
    unlink(FIFO_PATH);

    printf("\nTest finished.\n");

    return 0;
}

