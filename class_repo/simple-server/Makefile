CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpthread

# Source files
SRC = main.c

# Output directory and binary name
OUT_DIR = dist
OUT_BIN = $(OUT_DIR)/server

# Target to build the binary
all: $(OUT_BIN)

# Rule to compile the source file
$(OUT_BIN): $(SRC)
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -o $(OUT_BIN) $(SRC) $(LDFLAGS)

# Clean the build artifacts
clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean
