CC = gcc
FLAG = -std=c11 -Wall

TARGET1 = listPid
TARGET2 = pstree
TARGET3 = find_opener

TARGET4 = test_file
TARGET5 = test_tree

SRC1 = 12-1.c
SRC2 = 12-2.c
SRC3 = 12-3.c
SRC4 = test_file.c
SRC5 = test_process.c

all: $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2)
	$(CC) $(FLAG) $(SRC3) -o $(TARGET3)
	$(CC) $(FLAG) $(SRC4) -o $(TARGET4)
	$(CC) $(FLAG) $(SRC5) -o $(TARGET5)
	

clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f $(TARGET3)
	rm -f $(TARGET4)
	rm -f $(TARGET5)
	
