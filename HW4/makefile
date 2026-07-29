CC = gcc
CFLAGS = -Wall -g -std=gnu11

TARGET = myshell


SOURCE = builtin.c is_background.c parse.c run_command.c shell.c

# turn .c files into .o files
OBJS = $(SOURCE:.c=.o)


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 將每個 .c 編譯成 .o，並相依於 shell.h
%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
