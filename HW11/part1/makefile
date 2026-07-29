CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = zmq_ipc_server
TARGET2 = zmq_ipc_client

SRC1 = zmq_ipc_server.c lookup5.c
SRC2 = main.c lookup7.c
SRC3 = dict.h

all: $(SRC1) $(SRC2) $(SRC3)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1) -lzmq
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2) -lzmq

clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f *.exe

