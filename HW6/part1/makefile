CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET1 = hostinfo
TARGET2 = mydate
TARGET3 = printdir
TARGET4 = mycat
TARGET5 = pipe_ls

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5)
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET1).c
	$(CC) $(CFLAGS) -o $(TARGET2) $(TARGET2).c
	$(CC) $(CFLAGS) -o $(TARGET3) $(TARGET3).c
	$(CC) $(CFLAGS) -o $(TARGET4) $(TARGET4).c
	$(CC) $(CFLAGS) -o $(TARGET5) $(TARGET5).c

test:
	./hostinfo
	@echo ""
	@echo ""
	./mydate
	@echo ""
	@echo ""
	./printdir
	@echo ""
	@echo ""
	./mycat 123
	@echo ""
	@echo ""
	./pipe_ls
	

clean:
	rm -f $(TARGET1)
	rm -f $(TARGET2)
	rm -f $(TARGET3)
	rm -f $(TARGET4)
	rm -f $(TARGET5)
