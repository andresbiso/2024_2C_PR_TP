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

// Shared headers
#include "../shared/common.h"

// Project header
#include "udp_client.h"

int main(int argc, char *argv[])
{
    char ip_number[INET_ADDRSTRLEN], port_number[PORTSTRLEN];
    int ret_val;
    int sockfd; // listen on sock_fd

    strcpy(ip_number, DEFAULT_IP);
    strcpy(port_number, DEFAULT_PORT);

    ret_val = parse_arguments(argc, argv, port_number, ip_number);
    if (ret_val > 0)
    {
        return EXIT_SUCCESS;
    }
    else if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }
    sockfd = setup_udp_client(port_number, ip_number);
    if (sockfd <= 0)
    {
        return EXIT_FAILURE;
    }
    ret_val = handle_connection(sockfd);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

    puts("udp_client: finalizando");
    return EXIT_SUCCESS;
}

int parse_arguments(int argc, char *argv[], char *port_number, char *ip_number)
{
    int ret_val;

    ret_val = 0;
    if (argc >= 2)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "--help") == 0)
            {
                show_help();
                ret_val = 1;
                break;
            }
            else if (strcmp(argv[i], "--version") == 0)
            {
                show_version();
                ret_val = 1;
                break;
            }
            else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            {
                strcpy(port_number, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--ip") == 0 && i + 1 < argc)
            {
                strcpy(ip_number, argv[i + 1]);
                i++; // Skip the next argument since it's the IP address
            }
            else
            {
                printf("udp_client: opción o argumento no soportado: %s\n", argv[i]);
                show_help();
                ret_val = -1;
                break;
            }
        }
    }
    return ret_val;
}

void show_help()
{
    puts("Uso: udp_client [opciones]");
    puts("Opciones:");
    puts("  --help      Muestra este mensaje de ayuda");
    puts("  --version   Muestra version del programa");
    puts("  --port <puerto> Especificar el número de puerto del servidor");
    puts("  --ip <ip> Especificar el número de ip del servidor");
}

void show_version()
{
    printf("UDP Client Version %s\n", VERSION);
}

int setup_udp_client(char *port_number, char *ip_number)
{
    int gai_ret_val, local_port, sockfd;
    char local_ip[INET_ADDRSTRLEN], their_ipstr[INET_ADDRSTRLEN];
    struct addrinfo *servinfo, *p;
    struct sockaddr_in local_addr, *ipv4;
    struct in_addr *their_addr;
    socklen_t local_addr_len;
    struct addrinfo hints;

    sockfd = 0;

    // Setup addinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((gai_ret_val = getaddrinfo(ip_number, port_number, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "udp_client: getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("udp_client: direcciones resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        their_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it:
        inet_ntop(p->ai_family, their_addr, their_ipstr, sizeof(their_ipstr));
        printf("->  IPv4: %s\n", their_ipstr);
    }

    // Loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("udp_client: socket");
            continue;
        }

        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "udp_client: no pudo obtenerse un file descriptor\n");
        return -1;
    }

    // Show server ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    their_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, their_addr, their_ipstr, sizeof(their_ipstr));

    printf("udp_client: dirección destino %s:%s\n", their_ipstr, port_number);

    // Show udp_client ip and port
    local_addr_len = sizeof(struct sockaddr_in);
    // Returns the current address to which the socket sockfd is bound
    getsockname(sockfd, (struct sockaddr *)&local_addr, &local_addr_len);
    inet_ntop(local_addr.sin_family, &local_addr.sin_addr, local_ip, sizeof(local_ip));
    local_port = ntohs(local_addr.sin_port);

    printf("udp_client: dirección local %s:%d\n", local_ip, local_port);

    return sockfd;
}

int handle_connection(int sockfd)
{
    int retries, ret_value;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    Udp_Packet *send_packet, *recv_packet;
    ssize_t bytes_sent, bytes_received;

    retries = 0;
    ret_value = 0;

    // Loop to send packet and wait for response, retry if necessary
    while (1)
    {
        // Create a UDP packet
        send_packet = create_udp_packet("HEARTBEAT");
        if (send_packet == NULL)
        {
            ret_value = -1;
            break;
        }

        bytes_sent = send_udp_packet(sockfd, send_packet, (struct sockaddr *)&server_addr, addr_len);
        if (bytes_sent < 0)
        {
            free_udp_packet(send_packet);
            ret_value = -1;
            break;
        }
        printf("udp_client: mensaje enviado: %s - %ld\n", send_packet->message, send_packet->timestamp);
        free_udp_packet(send_packet);

        if (set_udp_timeout(sockfd, UDP_TIMEOUT_SEC) < 0)
        {
            ret_value = -1;
            break;
        }

        // Allocate memory for recv_packet
        recv_packet = (Udp_Packet *)malloc(sizeof(Udp_Packet));
        if (recv_packet == NULL)
        {
            perror("Error allocating memory for recv_packet");
            ret_value = -1;
            break;
        }
        bytes_received = recv_udp_packet(sockfd, recv_packet, (struct sockaddr *)&client_addr, &addr_len);

        if (bytes_received > 0 && strcmp(recv_packet->message, "ACK") == 0)
        {
            printf("udp_client: mensaje recibido: %s - %ld\n", recv_packet->message, recv_packet->timestamp);
            free_udp_packet(recv_packet);
            retries = 0; // Reset retries on successful response
        }
        else
        {
            retries++;
            if (retries >= UDP_MAX_RETRIES)
            {
                printf("udp_client: no hubo respuesta luego de %d intentos. Finalizando...\n", UDP_MAX_RETRIES);
                ret_value = -1;
                break;
            }
            printf("udp_client: no hubo respuesta. Reintentando...\n");
        }
    }

    // Clean up
    close(sockfd);
    return ret_value;
}

// Set timeout for receive
// configures the socket to wait only a specified amount of time (in this case, 5 seconds)
// for data to be received. If no data is received within that timeframe,
// the recvfrom function will return an error
int set_udp_timeout(int sockfd, time_t timeout_sec)
{
    int result;
    struct timeval tv;

    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    result = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (result < 0)
    {
        perror("udp_client: error configurando opción del socket");
        return -1;
    }
    return 0;
}