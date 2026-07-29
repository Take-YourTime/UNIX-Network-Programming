CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = server
TARGET2 = client
TARGET3 = bad_client


SRC1 = server.c
SRC2 = client.c
SRC3 = bad_client.c
SRC4 = fifo_seqnum.h


all: $(SRC1) $(SRC2) $(SRC3) $(SRC4)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2)
	$(CC) $(FLAG) $(SRC3) -o $(TARGET3)



clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f $(TARGET3)
