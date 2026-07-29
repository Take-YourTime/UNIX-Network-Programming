CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC1 = shell.h
SRC2 = shell.c
SRC3 = parse.c
SRC4 = run_command.c
SRC5 = builtin.c
SRC6 = is_background.c
SRC7 = pipe_command.c
SRC8 = pipe_present.c
SRC9 = redirect_in.c
SRC10 = redirect_out.c

TARGET = myshell

all: $(TARGET)

$(TARGET): $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5) $(SRC6) $(SRC7) $(SRC8) $(SRC9) $(SRC10)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5) $(SRC6) $(SRC7) $(SRC8) $(SRC9) $(SRC10) 

clean:
	rm -f $(TARGET)
