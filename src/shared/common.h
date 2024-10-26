#ifndef COMMON_H
#define COMMON_H

// System headers
#include <sys/types.h>

// Constants
#define DEFAULT_BUFFER_SIZE 100 // Default max number of bytes we can get at once

// Structs
typedef struct
{
    int32_t length;
    char *data;
} Simple_Packet;

char *allocate_string(const char *source);
char *allocate_string_with_length(size_t length);
ssize_t recvall(int sockfd, void *buf, size_t len);
ssize_t recvall_with_flags(int sockfd, void *buf, size_t len, int flags);
ssize_t sendall(int sockfd, const void *buf, size_t len);
ssize_t sendall_with_flags(int sockfd, const void *buf, size_t len, int flags);
Simple_Packet *create_simple_packet(const char *data);
Simple_Packet *create_packet_with_length(int32_t length);
int free_simple_packet(Simple_Packet *packet);
ssize_t send_simple_packet(int sockfd, Simple_Packet *packet);
ssize_t recv_simple_packet(int sockfd, Simple_Packet **packet);
void simulate_work();

#endif // COMMON_H