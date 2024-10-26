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

    len = strlen(source) + 1; // +1 for the null terminator
    str = (char *)malloc(len);
    if (str == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(str, 0, len); // Initialize allocated memory to zero
    strcpy(str, source);
    return str;
}

char *allocate_string_with_length(size_t length)
{
    char *str;
    size_t len;

    len = length + 1; // +1 for the null terminator
    str = (char *)malloc(len);
    if (str == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(str, 0, len); // Initialize allocated memory to zero
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

int create_packet_with_length(Simple_Packet **packet, int32_t length)
{
    *packet = (Simple_Packet *)malloc(sizeof(Simple_Packet));
    if (*packet == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return -1; // Memory allocation failed
    }
    (*packet)->length = length;
    if (((*packet)->data = allocate_string_with_length(length)) == NULL)
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
    unsigned char net_length[sizeof(int32_t)];
    int packet_size;
    ssize_t sent_bytes;

    packet_size = sizeof(int32_t) + packet->length; // sizeof(length) + data length
    buffer = allocate_string_with_length(packet_size);
    if (buffer == NULL)
    {
        return -1;
    }

    // Pack the length into network byte order using pack()
    pack(net_length, "l", packet->length);
    memcpy(buffer, net_length, sizeof(int32_t));
    memcpy(buffer + sizeof(int32_t), packet->data, packet->length);

    // Send the entire packet
    if ((sent_bytes = sendall(sockfd, buffer, packet_size)) < 0)
    {
        return sent_bytes;
    }

    if (sent_bytes != packet_size)
    {
        fprintf(stderr, "Error al enviar el packet\n");
        return -1;
    }

    free(buffer);
    return sent_bytes;
}

ssize_t recv_simple_packet(int sockfd, Simple_Packet **packet)
{
    int32_t length;
    unsigned char net_length[sizeof(int32_t)];
    char length_buffer[sizeof(int32_t)];
    ssize_t recv_bytes, total_recvbytes;

    total_recvbytes = 0;

    // Receive the length first
    if (recv_bytes = recvall(sockfd, length_buffer, sizeof(int32_t)) <= 0)
    {
        return recv_bytes;
    }

    if (recv_bytes != sizeof(int32_t))
    {
        fprintf(stderr, "Error al recibir el length del packet\n");
        return -1;
    }
    total_recvbytes += recv_bytes;
    memcpy(net_length, length_buffer, sizeof(int32_t));

    unpack(net_length, "l", &length); // Convert from network byte order to host byte order

    // Allocate and initialize the packet
    *packet = create_packet_with_length(packet, length);
    if (*packet == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return -1;
    }

    // Receive the actual data
    if (recv_bytes = recvall(sockfd, (*packet)->data, length) <= 0)
    {
        return recv_bytes;
    }

    if (recv_bytes != length)
    {
        fprintf(stderr, "Error al recibir el data del packet\n");
        return -1;
    }
    total_recvbytes += recv_bytes;

    (*packet)->data[length] = '\0'; // Null-terminate the string

    return recv_bytes;
}

void simulate_work()
{
    // Simulate work by sleeping
    struct timespec delay;
    delay.tv_sec = 2; // 2 seconds
    delay.tv_nsec = 0;
    nanosleep(&delay, NULL);
}