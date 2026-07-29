/*
 * argv[1] is the name of the local datafile
 * PORT is defined in dict.h
 */

#include "dict.h"
#include <zmq.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
	Dictrec tryit;

	if (argc != 2) {
		fprintf(stderr,"Usage : %s <datafile>\n",argv[0]);
		exit(1);
	}

	/* Fill in code. */
	void *context = zmq_ctx_new();
	void *responder = zmq_socket(context, ZMQ_REP);
	
	char address[256];
	snprintf(address, sizeof(address), "tcp://*:%d", PORT);
	zmq_bind(responder, address);

    /* main loop : receive request -> lookup -> send reply */
	for (;;) {
        /* Fill in code. */
		zmq_recv(responder, &tryit, sizeof(Dictrec), 0);

        switch (lookup(&tryit, argv[1])) {
            case FOUND:
                /* Fill in code. */
				zmq_send(responder, &tryit, sizeof(Dictrec), 0);
                break;

            case NOTFOUND:
                /* Fill in code. */
				strcpy(tryit.text, "XXXX");
				zmq_send(responder, &tryit, sizeof(Dictrec), 0);
                break;

            case UNAVAIL:
                DIE(argv[1]);
        }
    }
} /* end main */
