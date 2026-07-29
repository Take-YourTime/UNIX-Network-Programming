CC = gcc
FLAG = -std=c11 -Wall

TARGET1 = bidirect
TARGET2 = popen

SRC1 = 44-1.c
SRC2 = 44-2.c

all: $(SRC1)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2)

clean:
	rm -f $(TARGET1)
	
