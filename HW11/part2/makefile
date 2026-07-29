CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = zmq_tcp_server
TARGET2 = zmq_tcp_client

SRC1 = zmq_tcp_server.c lookup5.c
SRC2 = main.c lookup8.c
SRC3 = dict.h

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SRC1) $(SRC3)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1) -lzmq

$(TARGET2): $(SRC2) $(SRC3)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2) -lzmq

run_server: $(TARGET1)
	./$(TARGET1) fixrec

run_client: $(TARGET2)
	./$(TARGET2) localhost

clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f *.exe

