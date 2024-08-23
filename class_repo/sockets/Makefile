# Define the compiler
CC = gcc
CFLAGS = -Wall -Wno-unused-variable
LDFLAGS = -lpthread

# Define the source files
SRCS = accept.c connect.c poll.c recv-q.c select.c structs.c

# Define the output directory
DIST_DIR = dist

# Define the binary names (one per source file)
BINS = $(patsubst %.c,$(DIST_DIR)/%,$(SRCS))

# Rule for all targets
all: $(BINS)

# Create the /dist directory if it doesn't exist
$(DIST_DIR):
	mkdir -p $(DIST_DIR)

# Rule for building each binary
$(DIST_DIR)/%: %.c main.h | $(DIST_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Individual targets for each program
accept: $(DIST_DIR)/accept
connect: $(DIST_DIR)/connect
recvq: $(DIST_DIR)/recvq
poll: $(DIST_DIR)/poll
select: $(DIST_DIR)/select
structs: $(DIST_DIR)/structs

# Clean up the binaries
clean:
	rm -rf $(DIST_DIR)

.PHONY: all clean accept connect recvq poll select structs