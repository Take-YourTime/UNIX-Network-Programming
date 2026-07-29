#ifndef EX46_2_H
#define EX46_2_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SERVER_KEY 0x1234
#define SERVER_MTYPE 1

struct requestMsg {
    long mtype;
    pid_t pid;
    int seqLen;
};

struct responseMsg {
    long mtype;
    int seqNum;
};

#define REQ_MSG_SIZE   (sizeof(struct requestMsg) - sizeof(long))
#define RESP_MSG_SIZE  (sizeof(struct responseMsg) - sizeof(long))

#endif