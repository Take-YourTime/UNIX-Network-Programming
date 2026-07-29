CC = gcc
FLAG = -std=c11 -Wall

TARGET1 = tee
TARGET2 = cp2

SRC1 = 4-1.c
SRC2 = 4-2.c

all: $(SRC1) $(SRC2)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	$(CC) $(FLAG) $(SRC2) -o $(TARGET2)
	
	
# 4-2 cp2 testing
test_cp2: cp2
	@echo "--- Create a file with hole (hole.txt) ---"
	echo "start" | dd of=hole.txt bs=1 count=5
	echo "end" | dd of=hole.txt bs=1K count=1 seek=100 conv=notrunc
	@echo ""
	@echo "--- Resource file size (hole.txt) ---"
	@du -h hole.txt --apparent-size
	@du -h hole.txt
	@echo ""
	@echo "--- Execute cp2 ---"
	./cp2 hole.txt copy.txt
	@echo ""
	@echo "--- The copy file size (copy.txt) ---"
	@du -h copy.txt --apparent-size
	@du -h copy.txt
	@echo ""
	@echo "--- Check the result, no output means success! ---"
	diff hole.txt copy.txt
	@echo ""
	@echo "Complete testing!"


clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	
