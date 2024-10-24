// Standard library headers
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Networking headers
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

// System headers
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

// Other project headers
#include "pack.h"

// Project header
#include "common.h"

int malloc_string(char **s, int32_t size)
{
    *s = (char *)malloc(size * sizeof(char));
    if (*s == NULL)
    {
        puts("Error al asignar memoria");
        perror("malloc");
        return -1;
    }
    memset(*s, 0, size);
    return 0;
}

// use this function when you know the size the data

/* recvall:
 * Continuously receive data until the entire buffer is filled or the connection is closed.
 * We keep track of the total received data, and adjust the buffer pointer to avoid overwriting existing data.
 * `bytesleft` tracks how much of the buffer is yet to be filled, although `recv` manages the data reception amount.
 */

long recvall(int sockfd, char *buffer, int32_t buffer_size)
{
    int32_t bytesleft, received, total_received;

    total_received = 0;
    bytesleft = buffer_size;

    while (total_received < buffer_size)
    {
        received = recv(sockfd, buffer + total_received, bytesleft, 0);
        if (received == -1)
        {
            free(buffer);
            puts("Error al querer recibir datos");
            perror("recv");
            return -1;
        }
        else if (received == 0)
        {
            // received = 0: connection closed by the server
            break;
        }

        total_received += received;
    }

    buffer[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %d bytes\n", total_received);
    return total_received;
}

// use this function if you don't know the real size of the data
long recvall_dynamic(int sockfd, char **buffer, int32_t *buffer_size)
{
    int32_t initial_size, total_received;
    int32_t received;

    initial_size = *buffer_size;
    total_received = 0;

    while (1)
    {
        received = recv(sockfd, *buffer + total_received, initial_size - total_received - 1, 0);
        if (received == -1)
        {
            free(*buffer);
            puts("Error al querer recibir datos");
            perror("recv");
            return -1;
        }
        else if (received == 0)
        {
            // received = 0: connection closed by the server
            break;
        }

        total_received += received;

        if (total_received >= initial_size - 1)
        {
            initial_size *= 2; // Double the buffer size
            if ((*buffer = (char *)realloc(*buffer, initial_size)) == NULL)
            {
                puts("Error al reasignar memoria");
                perror("realloc");
                return -1;
            }
            printf("Buffer incrementado de %d bytes a %d bytes\n", *buffer_size, initial_size);
            *buffer_size = initial_size;
        }
    }

    (*buffer)[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %d bytes\n", total_received);
    return total_received;
}

// recvall_dynamic and also prevents deadlocks
long recvall_dynamic_timeout(int sockfd, char **buffer, int32_t *buffer_size)
{
    int flags, timeout;
    int32_t initial_size, total_received;
    int32_t received;

    timeout = 1;
    initial_size = *buffer_size;
    total_received = 0;

    struct timeval begin, now;
    double timediff;

    // Set socket to non-blocking mode
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        free(*buffer);
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        free(*buffer);
        return -1;
    }

    // beginning time
    gettimeofday(&begin, NULL);

    while (1)
    {
        gettimeofday(&now, NULL);

        // time elapsed in seconds
        timediff = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);

        // if you got some data, then break after timeout
        if (total_received > 0 && timediff > timeout)
        {
            break;
        }
        else if (timediff > timeout * 2)
        {
            // if you got no data at all, wait a little longer, twice the timeout
            break;
        }

        received = recv(sockfd, *buffer + total_received, initial_size - total_received - 1, 0);
        if (received == -1)
        {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                continue; // if interrupted by signal, just continue
            }
            free(*buffer);
            puts("Error al querer recibir datos");
            perror("recv");
            return -1;
        }
        else if (received == 0)
        {
            // received = 0: connection closed by the server
            break;
        }

        total_received += received;

        // reset beginning time
        gettimeofday(&begin, NULL);

        if (total_received >= initial_size - 1)
        {
            initial_size *= 2; // Double the buffer size
            if ((*buffer = (char *)realloc(*buffer, initial_size)) == NULL)
            {
                puts("Error al reasignar memoria");
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            printf("Buffer incrementado de %d bytes a %d bytes\n", *buffer_size, initial_size);
            *buffer_size = initial_size;
        }
    }

    (*buffer)[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %d bytes\n", total_received);

    // Set socket back to blocking mode
    if (fcntl(sockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl");
        free(*buffer);
        return -1;
    }
    return total_received;
}

/* sendall:
 * Continuously send data until the entire buffer is transmitted.
 * This ensures partial sends are handled by tracking how much data is left to send.
 * We adjust the buffer pointer to the correct offset to avoid resending the same data.
 * The expression `length - total_sent` calculates the remaining amount of data to be sent.
 */

long sendall(int sockfd, const char *buffer, int32_t length)
{
    int32_t total_sent = 0;
    while (total_sent < length)
    {
        int32_t sent = send(sockfd, buffer + total_sent, length - total_sent, 0);
        if (sent == -1)
        {
            puts("Error al querer enviar datos");
            perror("send");
            return -1;
        }
        total_sent += sent;
    }
    printf("Total enviado: %d bytes\n", total_sent);
    return total_sent;
}

int create_simple_packet(Simple_Packet **packet, const char *data)
{
    *packet = (Simple_Packet *)malloc(sizeof(Simple_Packet));
    if (*packet == NULL)
    {
        puts("Error al asignar memoria");
        perror("malloc");
        return -1; // Memory allocation failed
    }

    (*packet)->length = strlen(data);
    if (malloc_string(&((*packet)->data), (*packet)->length) != 0)
    {
        free(*packet); // Free the struct if string allocation fails
        return -1;
    }

    strcpy((*packet)->data, data);
    return 0; // Success
}

int free_simple_packet(Simple_Packet *packet)
{
    if (packet == NULL)
    {
        return -1; // Error: Packet is NULL
    }

    if (packet->data != NULL)
    {
        free(packet->data);
    }
    free(packet); // Free the packet struct itself

    return 0; // Success
}

int send_simple_packet(int sockfd, Simple_Packet *packet)
{
    char *buffer;
    unsigned char *ubuffer;
    int packetsize;

    malloc_string(&buffer, sizeof(int32_t));            // Allocate for int32_t length
    ubuffer = (unsigned char *)malloc(sizeof(int32_t)); // Allocate for int32_t length
    packetsize = pack(ubuffer, "l", packet->length);

    printf("packet es de %d bytes\n", packetsize);
    memcpy(buffer, ubuffer, sizeof(int32_t));

    // Send the length first
    if (sendall(sockfd, buffer, sizeof(int32_t)) == -1)
    {
        free(buffer);
        return -1;
    }

    // Send the data
    if (sendall(sockfd, packet->data, packet->length) == -1)
    {
        return -1;
    }

    free(buffer);
    free(ubuffer);
    return 0; // Success
}

int recv_simple_packet(int sockfd, Simple_Packet **packet)
{
    char *buffer;
    int32_t *buffer_size;
    unsigned char *ubuffer;

    malloc_string(&buffer, sizeof(int32_t));
    ubuffer = (unsigned char *)malloc(sizeof(int32_t));
    buffer_size = (int32_t *)malloc(sizeof(int32_t));

    // Receive the length first
    if (recvall(sockfd, buffer, sizeof(int32_t)) <= 0)
    {
        return -1;
    }

    // Create packet
    if (create_simple_packet(packet, "") != 0)
    {
        return -1;
    }

    memcpy(ubuffer, buffer, sizeof(int32_t));
    unpack(ubuffer, "l", &((*packet)->length));

    // Allocate memory for the data
    if (malloc_string(&((*packet)->data), (*packet)->length) != 0)
    {
        free_simple_packet(*packet);
        return -1;
    }

    if (recvall(sockfd, (*packet)->data, (*packet)->length) <= 0)
    {
        return -1;
    }
    free(buffer);
    free(buffer_size);
    return 0; // Success
}

// ONLY USE IF USING FORK()
// Setup signal handler for reaping zombie processes that appear as the fork()ed child processes exit.
// This code ensures that terminated child processes are cleaned up immediately,
// preventing resource leaks and keeping the process table uncluttered.
int setup_signal_handler()
{
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        return -1;
    }
    return 0;
}

// ONLY USE IF USING FORK()
// Function to reap all dead processes
void sigchld_handler(int s)
{
    (void)s; // Quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

void simulate_work()
{
    // Simulate work by sleeping
    struct timespec delay;
    delay.tv_sec = 2; // 2 seconds
    delay.tv_nsec = 0;
    nanosleep(&delay, NULL);
}