CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = zmq

SRC1 = zmq.c

all: $(SRC1)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1) -lzmq

clean:
	rm -f $(TARGET1) 
