CC = gcc
FLAG = -std=c11 -Wall

TARGET1 = tail

SRC1 = 13-5.c

all: $(SRC1)
	$(CC) $(FLAG) $(SRC1) -o $(TARGET1)
	
test:
	@echo "--- Context of file test.txt ---"
	cat test.txt
	@echo ""
	@echo ""
	@echo "--- Output the last ten line of test.txt by default ---"
	./tail test.txt
	@echo ""
	@echo ""
	@echo "--- Output the last five line of test.txt by option ---"
	./tail -n 5 test.txt
	@echo ""
	

clean:
	rm -f $(TARGET1)
	
