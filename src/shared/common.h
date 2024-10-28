#ifndef COMMON_H
#define COMMON_H

// Standard library headers
#include <stdint.h>
#include <stddef.h>
#include <time.h>

// System headers
#include <sys/types.h>

// Constants
#define DEFAULT_BUFFER_SIZE 100 // Default max number of bytes we can get at once
#define UDP_TIMEOUT_SEC 5
#define UDP_MAX_RETRIES 3
#define UDP_BUF_SIZE 1024

// Structs
typedef struct
{
    int32_t length;
    char *data;
} Simple_Packet;

typedef struct
{
    char message[UDP_BUF_SIZE];
    time_t timestamp;
} Udp_Packet;

char *allocate_string(const char *source);
char *allocate_string_with_length(size_t length);
ssize_t recvall(int sockfd, void *buf, size_t len);
ssize_t recvall_with_flags(int sockfd, void *buf, size_t len, int flags);
ssize_t sendall(int sockfd, const void *buf, size_t len);
ssize_t sendall_with_flags(int sockfd, const void *buf, size_t len, int flags);
Simple_Packet *create_simple_packet(const char *data);
Simple_Packet *create_simple_packet_with_length(int32_t length);
int free_simple_packet(Simple_Packet *packet);
ssize_t send_simple_packet(int sockfd, Simple_Packet *packet);
ssize_t recv_simple_packet(int sockfd, Simple_Packet **packet);
Udp_Packet *create_udp_packet(const char *message);
int free_udp_packet(Udp_Packet *packet);
ssize_t send_udp_packet(int sockfd, Udp_Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recv_udp_packet(int sockfd, Udp_Packet *packet, struct sockaddr *src_addr, socklen_t *addrlen);
void simulate_work();

#endif // COMMON_H