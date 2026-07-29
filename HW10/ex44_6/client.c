// [good client]
// ask to server for a sequence of numbers, and print the first number in the sequence

#define _POSIX_C_SOURCE 200809L
#include "fifo_seqnum.h"

static char clientFifo[CLIENT_FIFO_NAME_LEN];

static void removeFifo(void)
{
    unlink(clientFifo);
}

int main(int argc, char *argv[])
{
    int serverFd;
    int clientFd;

    struct request clientReq; // the request send to server
    struct response serverResp; // the response received from server

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [seqLen]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	
	printf("Connecting to server...\n");	
	
    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) getpid());
	
    umask(0);   // since unix default use umask(022), 
                // which will cause the created FIFO to have permission 0666 & ~022 = 0644, 
                // and server won't have write permission to client's FIFO, 
                // so we need to set umask(0) to ensure the created FIFO has permission 0666 (-rw-rw-rw-)

    // 建立 client FIFO, permission = -rw-rw-rw- (0666)
    if (mkfifo(clientFifo, 0666) == -1) {
        perror("mkfifo client fifo");
        exit(EXIT_FAILURE);
    }

    // register退出時刪除 FIFO 的函數
    atexit(removeFifo);

    // open FIFO read-end, client will read response from server through this FIFO
        // O_NONBLOCK: if server hasn't written response yet, 
        // read() will return -1 with errno EAGAIN instead of busy-waiting
    clientFd = open(clientFifo, O_RDONLY | O_NONBLOCK);
    if (clientFd == -1) {
        perror("open client fifo for read");
        exit(EXIT_FAILURE);
    }

    // store client pid in request structure
    clientReq.pid = getpid();


    // get the number of sequence numbers requested from command line argument
    if (argc == 2)
        clientReq.seqLen = atoi(argv[1]);
    else
        clientReq.seqLen = 1; // default sequence length is 1
    
    if (clientReq.seqLen <= 0) {
        // wrong sequence length, exit with error
        fprintf(stderr, "seqLen must be > 0\n");
        exit(EXIT_FAILURE);
    }


    // open server FIFO write-end, server will write response to client through this FIFO
    // client will write request (sequence length) to server through this FIFO
    serverFd = open(SERVER_FIFO, O_WRONLY);
    if (serverFd == -1) {
        perror("open server fifo error");
        exit(EXIT_FAILURE);
    }

    // send request to server
    printf("Sending request (PID=%d, SeqLen=%d).\n", clientReq.pid, clientReq.seqLen);	
    if (write(serverFd, &clientReq, sizeof(struct request)) != sizeof(struct request)) {
        perror("write request");
        exit(EXIT_FAILURE);
    }
	printf("Request sent!\n");

    // main loop
    while (1) {
        ssize_t numRead;

        // read response from server by non-blocking read
        // if there is nonthing form server, read() will return -1 with errno EAGAIN,
        // and we can do another thing
        numRead = read(clientFd, &serverResp, sizeof(struct response));

        if (numRead == sizeof(struct response)) {
            break;
        }
        else if ( (numRead == -1 && errno == EAGAIN) || numRead == 0) {
            // no response from server yet, 
            // we can do something else here, 
            // but in this exercise, just sleep for a while and retry
            
            struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 100000000;   /* 0.1 second */
			nanosleep(&ts, NULL);
			
            continue;
        }
        else{
            perror("read response error");
            exit(EXIT_FAILURE);
        }
    }
	
	int seqNumber = serverResp.seqNum;
    printf("Assigned sequence number = %d ", seqNumber++);
	for(int i = 0; i < clientReq.seqLen-1; i++) printf("%d ", seqNumber++);
	printf("\n");
	
    close(serverFd);
    close(clientFd);

    return 0;
}
