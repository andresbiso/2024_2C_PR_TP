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
#include "heartbeat_client.h"

int main(int argc, char *argv[])
{
    char local_ip[INET_ADDRSTRLEN], local_port[PORTSTRLEN],
        external_ip[INET_ADDRSTRLEN], external_port[PORTSTRLEN];
    int ret_val;
    Heartbeat_Data *heartbeat_data;

    strcpy(external_ip, EXTERNAL_IP);
    strcpy(external_port, EXTERNAL_PORT);
    strcpy(local_ip, LOCAL_IP);
    strcpy(local_port, LOCAL_PORT);

    ret_val = parse_arguments(argc, argv, local_ip, local_port, external_ip, external_port);
    if (ret_val > 0)
    {
        return EXIT_SUCCESS;
    }
    else if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }
    heartbeat_data = setup_heartbeat_client(local_ip, local_port, external_ip, external_port);
    if (heartbeat_data == NULL)
    {
        return EXIT_FAILURE;
    }
    ret_val = handle_connection(heartbeat_data);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

    free_heartbeat_data(heartbeat_data);
    puts("heartbeat_client: finalizando");
    return EXIT_SUCCESS;
}

int parse_arguments(int argc, char *argv[], char *local_ip, char *local_port, char *external_ip, char *external_port)
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
            else if (strcmp(argv[i], "--local-ip") == 0 && i + 1 < argc)
            {
                strcpy(local_ip, argv[i + 1]);
                i++; // Skip the next argument since it's the IP address
            }
            else if (strcmp(argv[i], "--local-port") == 0 && i + 1 < argc)
            {
                strcpy(local_port, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--external-ip") == 0 && i + 1 < argc)
            {
                strcpy(external_ip, argv[i + 1]);
                i++; // Skip the next argument since it's the IP address
            }
            else if (strcmp(argv[i], "--external-port") == 0 && i + 1 < argc)
            {
                strcpy(external_port, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else
            {
                printf("heartbeat_client: opción o argumento no soportado: %s\n", argv[i]);
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
    puts("Uso: heartbeat_client [opciones]");
    puts("Opciones:");
    puts("  --help      Muestra este mensaje de ayuda");
    puts("  --version   Muestra version del programa");
    puts("  --local-ip <ip> Especificar el número de ip local");
    puts("  --local-port <puerto> Especificar el número de puerto local");
    puts("  --external-ip <ip> Especificar el número de ip externo");
    puts("  --external-port <puerto> Especificar el número de puerto externo");
}

void show_version()
{
    printf("Heartbeat Client Version %s\n", VERSION);
}

Heartbeat_Data *setup_heartbeat_client(char *local_ip, char *local_port, char *external_ip, char *external_port)
{
    int gai_ret_val, sockfd;
    char ipv4_ipstr[INET_ADDRSTRLEN];
    socklen_t yes;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in *ipv4;
    struct in_addr *ipv4_addr;
    struct addrinfo hints;
    Heartbeat_Data *heartbeat_data;

    yes = 1;
    heartbeat_data = NULL;

    // Bind to local ip and port
    // Setup addinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((gai_ret_val = getaddrinfo(local_ip, local_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "heartbeat_client: getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return NULL;
    }

    puts("heartbeat_client: direcciones locales resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        ipv4_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it:
        inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));
        printf("->  IPv4: %s\n", ipv4_ipstr);
    }

    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("heartbeat_client: socket");
            continue;
        }

        // Ask the kernel to let me reuse the socket if already in use from previous run
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("heartbeat_client: UDP setsockopt");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("heartbeat_client: UDP bind");
            continue;
        }

        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "heartbeat_client: no pudo obtenerse un file descriptor\n");
        return NULL;
    }

    heartbeat_data = create_heartbeat_data(sockfd);
    if (heartbeat_data == NULL)
    {
        return NULL;
    }

    // Get Server sockaddr
    // Setup addinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((gai_ret_val = getaddrinfo(external_ip, external_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "heartbeat_client: getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return NULL;
    }

    puts("heartbeat_client: direcciones externas resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        ipv4_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it:
        inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));
        printf("->  IPv4: %s\n", ipv4_ipstr);
    }

    // Verify if at least one of the results is not null
    // show values and add them to struct
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // Copy values into heartbeat_data
        memcpy(heartbeat_data->addr, p->ai_addr, p->ai_addrlen);
        heartbeat_data->addrlen = p->ai_addrlen;
        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "heartbeat_client: no pudo obtenerse un file descriptor\n");
        return NULL;
    }

    // Show heartbeat_client ip and port
    printf("heartbeat_client: dirección local %s:%s\n", local_ip, local_port);

    // Show server ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    ipv4_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));

    printf("heartbeat_client: dirección destino %s:%d\n", ipv4_ipstr, ntohs(ipv4->sin_port));

    return heartbeat_data;
}

int handle_connection(Heartbeat_Data *heartbeat_data)
{
    int retries, ret_value;
    ssize_t bytes_sent, bytes_received;

    retries = 0;
    ret_value = 0;

    if (set_heartbeat_timeout(heartbeat_data->sockfd, HEARTBEAT_TIMEOUT_SEC) < 0)
    {
        return -1;
    }

    // Loop to send packet and wait for response, retry if necessary
    while (1)
    {
        // Create a heartbeat packet
        heartbeat_data->packet = create_heartbeat_packet("HEARTBEAT");
        if (heartbeat_data->packet == NULL)
        {
            ret_value = -1;
            break;
        }

        bytes_sent = send_heartbeat_packet(heartbeat_data->sockfd, heartbeat_data->packet, heartbeat_data->addr, heartbeat_data->addrlen);
        if (bytes_sent < 0)
        {
            free_heartbeat_packet(heartbeat_data->packet);
            heartbeat_data->packet = NULL;
            ret_value = -1;
            break;
        }
        printf("heartbeat_client: mensaje enviado: %s - %ld\n", heartbeat_data->packet->message, heartbeat_data->packet->timestamp);
        free_heartbeat_packet(heartbeat_data->packet);
        heartbeat_data->packet = NULL;

        // Allocate memory for recv_packet
        heartbeat_data->packet = (Heartbeat_Packet *)malloc(sizeof(Heartbeat_Packet));
        if (heartbeat_data->packet == NULL)
        {
            perror("Error allocating memory for recv_packet");
            ret_value = -1;
            break;
        }
        bytes_received = recv_heartbeat_packet(heartbeat_data->sockfd, heartbeat_data->packet, heartbeat_data->addr, &heartbeat_data->addrlen);

        if (bytes_received > 0 && strcmp(heartbeat_data->packet->message, "ACK") == 0)
        {
            printf("heartbeat_client: mensaje recibido: %s - %ld\n", heartbeat_data->packet->message, heartbeat_data->packet->timestamp);
            free_heartbeat_packet(heartbeat_data->packet);
            heartbeat_data->packet = NULL;
            retries = 0; // Reset retries on successful response
        }
        else
        {
            retries++;
            if (retries >= HEARTBEAT_MAX_RETRIES)
            {
                printf("heartbeat_client: no hubo respuesta luego de %d intentos. Finalizando...\n", HEARTBEAT_MAX_RETRIES);
                ret_value = -1;
                break;
            }
            printf("heartbeat_client: no hubo respuesta. Reintentando...\n");
        }
    }

    return ret_value;
}

// Set timeout for receive
// configures the socket to wait only a specified amount of time (in this case, 5 seconds)
// for data to be received. If no data is received within that timeframe,
// the recvfrom function will return an error
int set_heartbeat_timeout(int sockfd, time_t timeout_sec)
{
    int result;
    struct timeval tv;

    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    result = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (result < 0)
    {
        perror("heartbeat_client: error configurando opción del socket");
        return -1;
    }
    return 0;
}