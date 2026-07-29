// [bad client]
// In this version, client doesn't open FIFO read-end before server opens FIFO write-end,

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

    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) getpid());

    umask(0);   // since unix default use umask(022), 
                // which will cause the created FIFO to have permission 0666 & ~022 = 0644, 
                // and server won't have write permission to client's FIFO, 
                // so we need to set umask(0) to ensure the created FIFO has permission 0666

    // 建立 client FIFO, permission = -rw-rw-rw- (0666)
    if (mkfifo(clientFifo, 0666) == -1) {
        perror("mkfifo client fifo");
        exit(EXIT_FAILURE);
    }

    // register退出時刪除 FIFO 的函數
    atexit(removeFifo);


    // We don't open FIFO read-end here to make it be a bad client.

    // If the server doesn't use non-blocking mode to open the client FIFO write-end,
    // server will be blocked at open() waiting for the client to open FIFO read-end.

    // If the server doesn't handle SIGPIPE properly, it will be terminated by SIGPIPE, 
    // and other clients won't be able to get response from server.

    /*
    clientFd = open(clientFifo, O_RDONLY | O_NONBLOCK);
    if (clientFd == -1) {
        perror("open client fifo for read");
        exit(EXIT_FAILURE);
    }
    */


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
    if (write(serverFd, &clientReq, sizeof(struct request)) != sizeof(struct request)) {
        perror("write request");
        exit(EXIT_FAILURE);
    }

    sleep(1); // wait for server to write response, and then exit without reading response

    close(serverFd);

    return 0;
}
