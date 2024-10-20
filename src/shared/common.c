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

void recvall(int socket, char *buffer, size_t buffer_size)
{
    size_t total_received = 0;
    ssize_t received;

    while (total_received < buffer_size - 1)
    {
        received = recv(socket, buffer + total_received, buffer_size - total_received - 1, 0);
        if (received == -1)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        else if (received == 0)
        {
            break; // Connection closed by the server
        }
        total_received += received;
    }
    buffer[total_received] = '\0'; // Null-terminate the string
}

void sendall(int socket, const char *buffer, size_t length)
{
    size_t total_sent = 0;
    while (total_sent < length)
    {
        ssize_t sent = send(socket, buffer + total_sent, length - total_sent, 0);
        if (sent == -1)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
        total_sent += sent;
    }
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
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}