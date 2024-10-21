# Define the compiler
CC = gcc

# Define compiler flags (CFLAGS)
CFLAGS = -Wall

# Define linker flags (LDFLAGS)
LDFLAGS = -lpthread

# Define the output directory
DIST_DIR = dist

# Define the output binary
TARGET = $(DIST_DIR)/server

# Define the source files
SRCS = server.c	../shared/common.c ../shared/pack.c

# Define the header files (for dependency tracking)
HEADERS = server.h ../shared/common.h ../shared/pack.h

# Define the object files
OBJS = $(SRCS:.c=.o)

# Rule for all targets (build the binary)
all: $(TARGET)

# Create the /dist directory if it doesn't exist
$(DIST_DIR):
	mkdir -p $(DIST_DIR)

# Rule for building the target binary
$(TARGET): $(OBJS) | $(DIST_DIR)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# Rule for compiling .c files into .o files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the binaries and object files
clean:
	rm -rf $(DIST_DIR) $(OBJS)

clean_obj:
	rm -rf $(OBJS)

build: all clean_obj

.PHONY: all clean
