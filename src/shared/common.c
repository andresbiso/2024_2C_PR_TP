// Standard library headers
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Networking headers
#include <netinet/in.h>
#include <sys/socket.h>

// Other project headers
#include "pack.h"

// Project header
#include "common.h"

char *allocate_string(const char *source)
{
    char *str;
    size_t len;

    size_t len = strlen(source) + 1; // +1 for the null terminator
    *str = (char *)malloc(len);
    if (str == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(str, 0, len); // Initialize allocated memory to zero
    strcpy(str, source);
    return str;
}

/* recvall:
 * Continuously receive data until the entire buffer is filled or the connection is closed.
 * We keep track of the total received data, and adjust the buffer pointer to avoid overwriting existing data.
 * `bytes_left` tracks how much of the buffer is yet to be filled, although `recv` manages the data reception amount.
 */
ssize_t recvall(int sockfd, void *buf, size_t len)
{
    return recvall_with_flags(sockfd, buf, len, 0);
}

ssize_t recvall_with_flags(int sockfd, void *buf, size_t len, int flags)
{
    char *buffer;
    size_t bytes_left;
    ssize_t total_received, received;

    *buffer = (char *)buf;
    bytes_left = len;
    total_received = 0;

    while (total_received < len)
    {
        received = recv(sockfd, buffer + total_received, bytes_left, flags);
        if (received < 0)
        {
            fprintf(stderr, "Error receiving data: %s\n", strerror(errno));
            return received;
        }
        else if (received == 0)
        {
            // Connection closed by the server
            break;
        }
        total_received += received;
        bytes_left -= received;
    }

    buffer[total_received] = '\0';                         // Null-terminate the string
    printf("Total recibido: %zd bytes\n", total_received); // zd is used for printing variables of type ssize_t
    return total_received;
}

/* sendall:
 * Continuously send data until the entire buffer is transmitted.
 * This ensures partial sends are handled by tracking how much data is left to send.
 * We adjust the buffer pointer to the correct offset to avoid resending the same data.
 * The expression `length - total_sent` calculates the remaining amount of data to be sent.
 */
ssize_t sendall(int sockfd, const void *buf, size_t len)
{
    return sendall_with_flags(sockfd, buf, len, 0);
}

ssize_t sendall_with_flags(int sockfd, const void *buf, size_t len, int flags)
{
    const char *data;
    size_t total_sent;
    ssize_t bytes_sent;

    data = (const char *)buf;
    total_sent = 0;

    while (total_sent < len)
    {
        bytes_sent = send(sockfd, data + total_sent, len - total_sent, flags);
        if (bytes_sent < 0)
        {
            fprintf(stderr, "Error al querer enviar datos: %s\n", strerror(errno));
            return bytes_sent;
        }
        total_sent += bytes_sent;
    }

    printf("Total enviado: %d bytes\n", total_sent);
    return total_sent;
}

int create_simple_packet(Simple_Packet **packet, const char *data)
{
    *packet = (Simple_Packet *)malloc(sizeof(Simple_Packet));
    if (*packet == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return -1; // Memory allocation failed
    }

    (*packet)->length = strlen(data);
    if (((*packet)->data = allocate_string(data)) == NULL)
    {
        free(*packet); // Free the struct if string allocation fails
        return -1;
    }

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
    return 0;     // Success
}

ssize_t send_simple_packet(int sockfd, Simple_Packet *packet)
{
    char *buffer;
    unsigned char *ubuffer;
    ssize_t sent_bytes, total_sentbytes;

    total_sentbytes = 0;

    // Allocate memory for the length
    buffer = (char *)malloc(sizeof(int32_t));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return -1;
    }

    // Allocate memory for packing the length
    ubuffer = (unsigned char *)malloc(sizeof(int32_t));
    if (ubuffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        free(buffer);
        return -1;
    }

    if (pack(ubuffer, "l", packet->length) != sizeof(int32_t))
    {
        fprintf(stderr, "Error al pack() el tamaño de la data\n");
        free(buffer);
        free(ubuffer);
        return -1;
    }

    printf("Packet size: %d bytes\n", packet->length);
    memcpy(buffer, ubuffer, sizeof(int32_t));

    // Send the length first
    if ((sent_bytes = sendall(sockfd, buffer, sizeof(int32_t))) < 0)
    {
        free(buffer);
        free(ubuffer);
        return sent_bytes;
    }
    total_sentbytes += sent_bytes;

    // Send the data
    if ((sent_bytes = sendall(sockfd, packet->data, packet->length)) < 0)
    {
        free(buffer);
        free(ubuffer);
        return sent_bytes;
    }
    total_sentbytes += sent_bytes;

    free(buffer);
    free(ubuffer);
    return total_sentbytes;
}

ssize_t recv_simple_packet(int sockfd, Simple_Packet **packet)
{
    char *buffer;
    unsigned char *ubuffer;
    ssize_t recv_bytes, ret_value, total_recvbytes;

    total_recvbytes = 0;

    buffer = (char *)malloc(sizeof(int32_t));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return -1;
    }

    ubuffer = (unsigned char *)malloc(sizeof(int32_t));
    if (ubuffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        free(buffer);
        return -1;
    }

    // Receive the length first
    if ((recv_bytes = recvall(sockfd, buffer, sizeof(int32_t))) <= 0)
    {
        free(buffer);
        free(ubuffer);
        return recv_bytes;
    }
    total_recvbytes += recv_bytes;

    // Create packet
    if ((ret_value = (ssize_t)create_simple_packet(packet, "")) < 0)
    {
        free(buffer);
        free(ubuffer);
        return ret_value;
    }

    memcpy(ubuffer, buffer, sizeof(int32_t));
    unpack(ubuffer, "l", &((*packet)->length));

    // Allocate memory for the data
    (*packet)->data = (char *)malloc((*packet)->length + 1); // +1 for the null terminator
    if ((*packet)->data == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        free(buffer);
        free(ubuffer);
        return -1;
    }

    if ((recv_bytes = recvall(sockfd, (*packet)->data, (*packet)->length)) <= 0)
    {
        free(buffer);
        free(ubuffer);
        return recv_bytes;
    }
    total_recvbytes += recv_bytes;

    free(buffer);
    free(ubuffer);
    return total_recvbytes;
}

void simulate_work()
{
    // Simulate work by sleeping
    struct timespec delay;
    delay.tv_sec = 2; // 2 seconds
    delay.tv_nsec = 0;
    nanosleep(&delay, NULL);
}