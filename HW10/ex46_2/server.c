#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include "ex46_2.h"

static int msqid = -1;

static void cleanup(int sig)
{
    if (msqid != -1) {
        msgctl(msqid, IPC_RMID, NULL);
    }

    printf("\nServer terminated. Message queue removed.\n");
    exit(EXIT_SUCCESS);
}

int main(void)
{
    struct requestMsg req;
    struct responseMsg resp;
    int seqNum = 0;

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    msqid = msgget(SERVER_KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Server started. (Queue ID: %d)\n", msqid);
    printf("Waiting for requests (mtype=1)...\n");

    while (1) {
        if (msgrcv(msqid, &req, REQ_MSG_SIZE, SERVER_MTYPE, 0) == -1) {
            perror("msgrcv");
            continue;
        }

        printf("Received Request -> PID: %ld, Len: %d\n", (long) req.pid, req.seqLen);

        resp.mtype = req.pid;
        resp.seqNum = seqNum;

        if (msgsnd(msqid, &resp, RESP_MSG_SIZE, 0) == -1) {
            perror("msgsnd");
            continue;
        }

        seqNum += req.seqLen;
    }
}
