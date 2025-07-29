CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra
TARGET := aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)

all: $(TARGET)

default: all

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o