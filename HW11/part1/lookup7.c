/*
 * lookup7 : does no looking up locally, but instead asks
 * a server for the answer. Communication is by ZeroMQ IPC REQ/REP sockets
 * The name of the socket is passed as resource.
 */

#include <zmq.h>
#include <string.h>
#include "dict.h"

int lookup(Dictrec * sought, const char * resource) {
	static int first_time = 1;
	int n;

	static void *context;
	static void *requester;

	if (first_time) {
		first_time = 0;

		/* Fill in code. */
		context = zmq_ctx_new();
		requester = zmq_socket(context, ZMQ_REQ);
		zmq_connect(requester, resource);
	}

	/* Fill in code. */
	zmq_send(requester, sought->word, WORD, 0);
	zmq_recv(requester, sought->text, TEXT, 0);

	if (strcmp(sought->text,"XXXX") != 0) {
		return FOUND;
	}

	return NOTFOUND;
}
