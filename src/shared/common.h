#ifndef COMMON_H
#define COMMON_H

// System headers
#include <sys/types.h>

// Constants
#define DEFAULT_BUFFER_SIZE 100 // Default max number of bytes we can get at once

char *initialize_string(size_t size);
int recvall(int sockfd, char **buffer, size_t *buffer_size);
int sendall(int sockfd, const char *buffer, size_t length);
void setup_signal_handler(void);
void sigchld_handler(int s);

#endif // COMMON_H