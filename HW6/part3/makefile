CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGETS = convert file_lookup

.PHONY: all clean test

all: $(TARGETS)

convert: convert.c dict.h
	$(CC) $(CFLAGS) -o convert convert.c

file_lookup: lookup1.c main.c dict.h
	$(CC) $(CFLAGS) -o file_lookup lookup1.c main.c

test: all
	@echo "========================================"
	@echo "Part 3 Test 1: convert dict to fixrec"
	@echo "========================================"
	@echo '$$ ./convert dict myfixrec'
	@./convert dict myfixrec
	@echo
	@echo '$$ ls -l dict myfixrec'
	@ls -l dict myfixrec

	@echo
	@echo "========================================"
	@echo "Part 3 Test 2: lookup words"
	@echo "========================================"
	@echo '$$ ./file_lookup myfixrec'
	@printf "work\ncynic\nbeauty\nhomework\n" | ./file_lookup myfixrec || true

clean:
	rm -f $(TARGETS) myfixrec
	
