CC = gcc
CFLAGS = -Wall -g
LIBS = -lncurses -lyaml
TARGET = bootloader
SRC = bootloader.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
		$(CC) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

clean:
		rm -f $(OBJ) $(TARGET)

.PHONY: all clean
