# Compiler
CC = gcc

# Flags and libraries
CFLAGS = -Wall -g
LDFLAGS = -lcrypto

# Source files and target
SRC = detect_dups.c compute-md5.c
TARGET = detect_dups

# Default rule
all: $(TARGET)

# Build rule
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Optional run rule (for convenience, but not required by Gradescope)
run:
	./$(TARGET) test1

# Clean rule
clean:
	rm -f $(TARGET)
