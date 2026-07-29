#include "dict.h"
#include <zmq.h>
#include <string.h>
#include <stdio.h>

int lookup(Dictrec * sought, const char * resource) {
	static int first_time = 1;
	static void *context;
	static void *requester;

	if (first_time) {
		first_time = 0;

		/* Fill in code. */
		context = zmq_ctx_new();
		requester = zmq_socket(context, ZMQ_REQ);
		
		char address[256];
		snprintf(address, sizeof(address), "tcp://%s:%d", resource, PORT);
		zmq_connect(requester, address);
	}

	/* Fill in code. */
	zmq_send(requester, sought, sizeof(Dictrec), 0);
	zmq_recv(requester, sought, sizeof(Dictrec), 0);

	if (strcmp(sought->text,"XXXX") != 0) {
		return FOUND;
	}

	return NOTFOUND;
}
