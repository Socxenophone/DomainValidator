# Makefile for C API Server using Mongoose and cJSON

# Define the C compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -pedantic

# Libraries needed: Mongoose doesn't usually require external libs beyond standard C
# cJSON is included directly as .c and .h files.
LIBS =

# Source files for the project
SRCS = main.c router.c handlers.c utils.c mongoose.c cJSON.c

# Object files will be created from source files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = api_server

.PHONY: all clean

# Default target: build the server
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

# To build: make
# To run: ./api_server
# To clean: make clean

