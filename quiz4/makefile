CC = gcc
FLAG = -std=c11 -Wall -o2

TARGET1 = cv
TARGET2 = mutex


SRC1 = cv.c
SRC2 = mutex.c


all: $(SRC1) $(SRC2)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1) -pthread
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2) -pthread


test: $(SRC1) $(SRC2)
	@echo "[condition variable]"
	./cv
	@echo ""
	@echo "[mutex]"
	./mutex


clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	
