#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include "ex46_2.h"

int main(int argc, char *argv[])
{
    int msqid;
    int seqLen;
    struct requestMsg req;
    struct responseMsg resp;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s seq-len\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    seqLen = atoi(argv[1]);
    if (seqLen <= 0) {
        fprintf(stderr, "seq-len must be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    msqid = msgget(SERVER_KEY, S_IWUSR | S_IRUSR);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    req.mtype = SERVER_MTYPE;
    req.pid = getpid();
    req.seqLen = seqLen;

    if (msgsnd(msqid, &req, REQ_MSG_SIZE, 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    printf("Request sent (PID=%ld, Len=%d). Waiting for response...\n", (long) req.pid, seqLen);

    if (msgrcv(msqid, &resp, RESP_MSG_SIZE, getpid(), 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }

    printf("Success! Assigned Sequence Number: %d\n", resp.seqNum);

    exit(EXIT_SUCCESS);
}
