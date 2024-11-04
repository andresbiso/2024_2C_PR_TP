#ifndef COMMON_H
#define COMMON_H

// Standard library headers
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <time.h>

// System headers
#include <sys/types.h>

// Constants
#define DEFAULT_BUFFER_SIZE 4096 // Default max number of bytes we can get at once
#define HEARTBEAT_TIMEOUT_SEC 5
#define HEARTBEAT_MAX_RETRIES 3
#define HEARTBEAT_BUF_SIZE 1024

// Structs
typedef struct
{
    int32_t length;
    char *data;
} Simple_Packet;

typedef struct
{
    char message[HEARTBEAT_BUF_SIZE];
    time_t timestamp;
} Heartbeat_Packet;

typedef struct
{
    int sockfd;
    struct sockaddr *addr; // Address structure
    socklen_t addrlen;     // Address length
    Heartbeat_Packet *packet;
} Heartbeat_Data;

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
Heartbeat_Packet *create_heartbeat_packet(const char *message);
int free_heartbeat_packet(Heartbeat_Packet *packet);
ssize_t send_heartbeat_packet(int sockfd, Heartbeat_Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recv_heartbeat_packet(int sockfd, Heartbeat_Packet *packet, struct sockaddr *src_addr, socklen_t *addrlen);
Heartbeat_Data *create_heartbeat_data(int sockfd);
void free_heartbeat_data(Heartbeat_Data *data);
void simulate_work();
int find_max(int num, ...);
void print_buffer(const char *buffer);

#endif // COMMON_H