#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    void *context = zmq_ctx_new();

    void *frontend = zmq_socket(context, ZMQ_ROUTER);
    void *backend = zmq_socket(context, ZMQ_DEALER);

    int rc_frontend = zmq_bind(frontend, "tcp://*:5559");
    if (rc_frontend != 0) {
        perror("zmq_bind frontend");
        exit(1);
    }

    int rc_backend = zmq_bind(backend, "tcp://*:5560");
    if (rc_backend != 0) {
        perror("zmq_bind backend");
        exit(1);
    }

    printf("Broker started. Frontend: tcp://*:5559, Backend: tcp://*:5560\n");

    zmq_proxy(frontend, backend, NULL);

    zmq_close(frontend);
    zmq_close(backend);
    zmq_ctx_destroy(context);

    return 0;
}
