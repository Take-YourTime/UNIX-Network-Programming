#include <zmq.h>
#include <string.h>
#include "dict.h"

int main(int argc, char **argv) {
	Dictrec dr, *tryit = &dr;

	if (argc != 2) {
		fprintf(stderr,"Usage : %s <datafile>\n",argv[0]);
		exit(1);
	}

	/* Fill in code. */
	void *context = zmq_ctx_new();
	void *responder = zmq_socket(context, ZMQ_REP);
	zmq_connect(responder, "tcp://localhost:5560");

	for (;;) {
        /* Fill in code. */
		zmq_recv(responder, tryit->word, WORD, 0);

        switch (lookup(tryit, argv[1])) {
            case FOUND:
                /* Fill in code. */
				zmq_send(responder, tryit->text, TEXT, 0);
                break;

            case NOTFOUND:
                /* Fill in code. */
				strcpy(tryit->text, "XXXX");
				zmq_send(responder, tryit->text, TEXT, 0);
                break;

            case UNAVAIL:
                DIE(argv[1]);
        }
    }
} /* end main */
