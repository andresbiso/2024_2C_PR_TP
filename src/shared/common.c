// Standard library headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Networking headers
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// System headers
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// Project header
#include "common.h"

char *initialize_string(size_t size)
{
    char *p = (char *)malloc(size * sizeof(char));
    if (p == NULL)
    {
        perror("malloc: fallo al asignar memoria");
        exit(EXIT_FAILURE);
    }
    memset(p, 0, size);
    return p;
}

int recvall(int socket, char **buffer, size_t *buffer_size)
{
    size_t total_received = 0;
    ssize_t received;
    size_t initial_size = *buffer_size;

    while (1)
    {
        received = recv(socket, *buffer + total_received, initial_size - total_received - 1, 0);
        if (received == -1)
        {
            free(*buffer);
            perror("recv: fallo al querer recibir datos");
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
            *buffer = (char *)realloc(*buffer, initial_size);
            if (*buffer == NULL)
            {
                perror("realloc: fallo al reasignar memoria");
                exit(EXIT_FAILURE);
            }
            printf("buffer incrementado de %ld bytes a %ld bytes\n", *buffer_size, initial_size);
            *buffer_size = initial_size;
        }
    }

    (*buffer)[total_received] = '\0'; // Null-terminate the string
    printf("total recibido: %ld bytes\n", total_received);
    return total_received;
}

int sendall(int socket, const char *buffer, size_t length)
{
    size_t total_sent = 0;
    while (total_sent < length)
    {
        ssize_t sent = send(socket, buffer + total_sent, length - total_sent, 0);
        if (sent == -1)
        {
            perror("send: fallo al querer enviar datos");
            exit(EXIT_FAILURE);
        }
        total_sent += sent;
    }
    printf("total enviado: %ld bytes\n", total_sent);
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