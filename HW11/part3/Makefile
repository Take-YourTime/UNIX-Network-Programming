CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = zmq_worker
TARGET2 = zmq_client
TARGET3 = broker

SRC1 = zmq_worker.c lookup5.c
SRC2 = main.c lookup9.c
SRC3 = broker.c
SRC4 = dict.h

all: $(SRC1) $(SRC2) $(SRC3) $(SRC4)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1) -lzmq
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2) -lzmq
	$(CC) $(FLAG) $(SRC3) -o $(TARGET3) -lzmq

run_broker: $(TARGET3)
	./$(TARGET3)

run_worker: $(TARGET1)
	./$(TARGET1) fixrec

run_client: $(TARGET2)
	./$(TARGET2) localhost

clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f $(TARGET3)
	rm -f *.exe

