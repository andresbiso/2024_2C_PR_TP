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

int malloc_string(char **s, int32_t size);
long recvall(int sockfd, char *buffer, int32_t buffer_size);
long recvall_dynamic(int sockfd, char **buffer, int32_t *buffer_size);
long recvall_dynamic_timeout(int sockfd, char **buffer, int32_t *buffer_size);
long sendall(int sockfd, const char *buffer, int32_t length);
int create_simple_packet(Simple_Packet **packet, char const *data);
int free_simple_packet(Simple_Packet *packet);
int send_simple_packet(int sockfd, Simple_Packet *packet);
int recv_simple_packet(int sockfd, Simple_Packet **packet);
int setup_signal_handler(void);
void sigchld_handler(int s);
void simulate_work();

#endif // COMMON_H