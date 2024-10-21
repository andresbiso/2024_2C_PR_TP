// Standard library headers
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Project header
#include "common.h"

char *initialize_string(size_t size)
{
    char *p = (char *)malloc(size * sizeof(char));
    if (p == NULL)
    {
        puts("Error al asignar memoria");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(p, 0, size);
    return p;
}

long recvall(int sockfd, char *buffer, size_t buffer_size)
{
    ssize_t bytesleft, received, total_received;

    total_received = 0;
    bytesleft = buffer_size;

    while (total_received < buffer_size)
    {
        received = recv(sockfd, buffer + total_received, bytesleft - total_received - 1, 0);
        if (received == -1)
        {
            free(buffer);
            puts("Error al querer recibir datos");
            perror("recv");
            exit(EXIT_FAILURE);
        }
        else if (received == 0)
        {
            // received = 0: connection closed by the server
            break;
        }

        total_received += received;
    }

    buffer[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %ld bytes\n", total_received);
    return total_received;
}

long recvall_dynamic(int sockfd, char **buffer, size_t *buffer_size)
{
    size_t initial_size, total_received;
    ssize_t received;

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
            exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
            }
            printf("Buffer incrementado de %ld bytes a %ld bytes\n", *buffer_size, initial_size);
            *buffer_size = initial_size;
        }
    }

    (*buffer)[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %ld bytes\n", total_received);
    return total_received;
}

long recvall_dynamic_timeout(int sockfd, char **buffer, size_t *buffer_size)
{
    int flags, timeout;
    size_t initial_size, total_received;
    ssize_t received;

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
        exit(EXIT_FAILURE);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        free(*buffer);
        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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
            printf("Buffer incrementado de %ld bytes a %ld bytes\n", *buffer_size, initial_size);
            *buffer_size = initial_size;
        }
    }

    (*buffer)[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %ld bytes\n", total_received);

    // Set socket back to blocking mode
    if (fcntl(sockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl");
        free(*buffer);
        exit(EXIT_FAILURE);
    }
    return total_received;
}

long sendall(int sockfd, const char *buffer, size_t length)
{
    size_t total_sent = 0;
    while (total_sent < length)
    {
        ssize_t sent = send(sockfd, buffer + total_sent, length - total_sent, 0);
        if (sent == -1)
        {
            puts("Error al querer enviar datos");
            perror("send");
            exit(EXIT_FAILURE);
        }
        total_sent += sent;
    }
    printf("Total enviado: %ld bytes\n", total_sent);
    return total_sent;
}

// ONLY USE IF USING FORK()
// Setup signal handler for reaping zombie processes that appear as the fork()ed child processes exit.
// This code ensures that terminated child processes are cleaned up immediately,
// preventing resource leaks and keeping the process table uncluttered.
void setup_signal_handler()
{
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
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