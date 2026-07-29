CC = gcc
FLAG = -std=c11 -Wall -o2

TARGET1 = 22-3_sigwaitinfo
TARGET2 = 22-4
TARGET3 = 22-3_sigsuspend


SRC1 = 22-3_sigwaitinfo.c
SRC2 = 22-4.c
SRC3 = 22-3_sigsuspend.c


all: $(SRC1) $(SRC2) $(SRC3)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2)
	$(CC) $(FLAG) $(SRC3) -o $(TARGET3)


test:
	@echo "[ 22-3 ]"
	@echo "=== sigsuspend version ==="
	time ./$(TARGET3) 1000000
	@echo ""
	@echo "=== sigwaitinfo version ==="
	time ./$(TARGET1) 1000000


clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f $(TARGET3)
	
