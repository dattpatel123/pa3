# Compiler
CC = gcc

# Source and output
TARGET = detect_dups
SRC = detect_dups.c compute-md5.c -lcrypto -Wall -g

# Default rule (build and run)
all: build run

# Build the executable
build:
	$(CC) -o $(TARGET) $(SRC)

# Run the program with the directory
run:
	./$(TARGET) /common/home/dp1296/cs214/pa3/create_tests/test1

# Clean rule
clean:
	rm -f $(TARGET)
