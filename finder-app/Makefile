CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra
TARGET := writer
SRC := writer.c
OBJ := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	chmod +x $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
# write.o : writer.c
#	$(CC) $(CFLAGS) writer.c -o writer.o
#	chmod +x writer.o

clean:
	rm -f $(TARGET) *.o
