CC = gcc
FLAG = -std=c11 -Wall -O2

TARGET1 = server
TARGET2 = client


SRC1 = server.c
SRC2 = client.c
SRC3 = ex46_2.h


all: $(SRC1) $(SRC2) $(SRC3)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2)



clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
