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
#include <signal.h>
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

int recvall(int sockfd, char **buffer, size_t *buffer_size)
{
    size_t initial_size, total_received;
    ssize_t received;

    initial_size = *buffer_size;
    total_received = 0;

    while (total_received < *buffer_size)
    {
        received = recv(sockfd, *buffer + total_received, initial_size - total_received - 1, 0);
        if (received == -1)
        {
            if (errno == EINTR)
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
            break; // Connection closed by the server
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

        // Exit if no more data to read
        printf("received: %ld\n", received);
        printf("total_received: %ld\n", total_received);
        printf("buffer_size: %ld\n", *buffer_size);
        if (received == 0 || received < (*buffer_size - total_received))
        {
            break;
        }
    }

    (*buffer)[total_received] = '\0'; // Null-terminate the string
    printf("Total recibido: %ld bytes\n", total_received);
    return total_received;
}

int sendall(int sockfd, const char *buffer, size_t length)
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