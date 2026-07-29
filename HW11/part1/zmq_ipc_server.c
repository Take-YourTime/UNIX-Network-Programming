/*
 * argv[1] is the name of the local datafile
 * PORT is defined in dict.h
 */

#include <zmq.h>
#include <string.h>
#include "dict.h"

int main(int argc, char **argv) {
    Dictrec tryit;

    if (argc != 3) {
        fprintf(stderr,"Usage : %s <datafile> <socket>\n",argv[0]);
        exit(1);
    }

    /* Fill in code. */
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    int rc = zmq_bind(responder, argv[2]);
    if (rc != 0) {
        perror("zmq_bind");
        exit(1);
    }

    /* main loop : receive request -> lookup -> send reply */
    for (;;) {
        /* Fill in code. */
        zmq_recv(responder, tryit.word, WORD, 0);

        switch (lookup(&tryit, argv[1])) {
            case FOUND:
                /* Fill in code. */
                zmq_send(responder, tryit.text, TEXT, 0);
                break;

            case NOTFOUND:
                /* Fill in code. */
                strcpy(tryit.text, "XXXX");
                zmq_send(responder, tryit.text, TEXT, 0);
                break;

            case UNAVAIL:
                DIE(argv[1]);
        }
    }
} /* end main */
