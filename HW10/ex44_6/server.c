#define _POSIX_C_SOURCE 200809L
#include "fifo_seqnum.h"

int main(void)
{
    int serverFd;
    int clientFd;
    int dummyFd; // dummy fd to prevent server from seeing EOF when all clients close their write end of FIFO
    int seqNum = 0; // the next sequence number to be sent to client

    struct request clientReq;
    struct response serverResp;

    char clientFifo[CLIENT_FIFO_NAME_LEN];

    umask(0);   // since unix default use umask(022), 
                // which will cause the created FIFO to have permission 0666 & ~022 = 0644, 
                // and server won't have write permission to client's FIFO, 
                // so we need to set umask(0) to ensure the created FIFO has permission 0666
    
    // 建立 server FIFO, permission = -rw-rw-rw- (0666)
    if (mkfifo(SERVER_FIFO, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo server fifo");
            exit(EXIT_FAILURE);
        }
    }
   	
   	printf("Waiting for client join...\n");
	
    // open server FIFO read-end, server will read request from client through this FIFO
    // busy-waiting is fine for server, since server is dedicated to handle client requests
    serverFd = open(SERVER_FIFO, O_RDONLY);
    if (serverFd == -1) {
        perror("open server fifo for read");
        exit(EXIT_FAILURE);
    }
    
    // 防止所有 client 關閉後，server 讀到 EOF 而結束
    dummyFd = open(SERVER_FIFO, O_WRONLY);
    if (dummyFd == -1) {
        perror("open server fifo for dummy write");
        exit(  EXIT_FAILURE);
    }
    
    
    // 避免 write 到沒有 reader 的 FIFO 時，server 被 SIGPIPE 結束。
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // ignore
    sigemptyset(&sa.sa_mask); // no additional signals blocked during execution of handler
    sa.sa_flags = 0; // no special flags
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // SIGPIPE
    // 當程式對pipe、FIFO、socket進行 write() 時，若另一端已關閉或沒有reader，kernel就送出SIGPIPE
    
    printf("Server started...\n");

    while (1) {
        ssize_t numRead;

        // read request from client by busy-waiting
        numRead = read(serverFd, &clientReq, sizeof(struct request));

        if (numRead == -1) {
            perror("read request error");
            continue;
        }

        if (numRead != sizeof(struct request)) {
            fprintf(stderr, "bad request, discard\n");
            continue;
        }

        // construct client FIFO name based on client pid
        snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) clientReq.pid);
        
        
        // 使用 O_NONBLOCK，避免惡意 client 沒有開 FIFO 讀端時，server 卡在 open()
        clientFd = open(clientFifo, O_WRONLY | O_NONBLOCK);

        if (clientFd == -1) {
            if (errno == ENXIO) {
                fprintf(stderr, "client FIFO has no reader, discard request: %s\n", clientFifo);
            }
            else {
                perror("open client fifo");
            }
            continue;
        }

        serverResp.seqNum = seqNum;

        // send response to client, 
            // if client has already closed FIFO read-end, 
            // write() will cause SIGPIPE, but we have set SIGPIPE to be ignored, 
            // so write() will return -1 with errno EPIPE, and we can just discard this request
        if (write(clientFd, &serverResp, sizeof(struct response)) != sizeof(struct response)) {
            perror("write response");
            close(clientFd);
            continue;
        }

        close(clientFd);

        printf("Received Request -> PID: %ld, SeqLen: %d, FirstSeqNum=%d\n", (long)clientReq.pid, clientReq.seqLen, serverResp.seqNum);

        seqNum += clientReq.seqLen;
    }

    close(serverFd);
    close(dummyFd);
    unlink(SERVER_FIFO);

    return 0;
}
