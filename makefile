CC = gcc
CFLAGS = -Wall -g
TARGET = bootloader
SRC = bootloader.c
OBJ = $(SRC:.c=.o)

# Link libyaml statically, others dynamically
LIBS = -Wl,-Bstatic -lyaml -Wl,-Bdynamic -lncurses

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
