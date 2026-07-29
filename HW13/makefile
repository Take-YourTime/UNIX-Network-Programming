CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = worker
TARGET2 = dispatcher
TARGET3 = broker

SRC1 = worker.c
SRC2 = dispatcher.c
SRC3 = broker.c

all: $(SRC1) $(SRC2) $(SRC3)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1) -lzmq
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2) -lzmq
	$(CC) $(FLAG) $(SRC3) -o $(TARGET3) -lzmq

clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2) 
	rm -f $(TARGET3) 

