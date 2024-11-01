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
#include "client.h"

int main(int argc, char *argv[])
{
    char external_ip[INET_ADDRSTRLEN], external_port[PORTSTRLEN];
    int ret_val;
    int sockfd; // listen on sock_fd

    strcpy(external_ip, EXTERNAL_IP);
    strcpy(external_port, EXTERNAL_PORT);

    ret_val = parse_arguments(argc, argv, external_ip, external_port);
    if (ret_val > 0)
    {
        return EXIT_SUCCESS;
    }
    else if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }
    sockfd = setup_client(external_ip, external_port);
    if (sockfd <= 0)
    {
        return EXIT_FAILURE;
    }
    ret_val = handle_connection(sockfd);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

    close(sockfd);
    puts("client: finalizando");
    return EXIT_SUCCESS;
}

int parse_arguments(int argc, char *argv[], char *external_ip, char *external_port)
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
                printf("client: opción o argumento no soportado: %s\n", argv[i]);
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
    puts("Uso: client [opciones]");
    puts("Opciones:");
    puts("  --help      Muestra este mensaje de ayuda");
    puts("  --version   Muestra version del programa");
    puts("  --external-ip <ip> Especificar el número de ip externp");
    puts("  --external-port <puerto> Especificar el número de puerto externo");
}

void show_version()
{
    printf("Client Version %s\n", VERSION);
}

int setup_client(char *external_ip, char *external_port)
{
    int gai_ret_val, sockfd;
    char local_ip[INET_ADDRSTRLEN], ipv4_ipstr[INET_ADDRSTRLEN];
    struct addrinfo *servinfo, *p;
    struct sockaddr_in local_addr, *ipv4;
    struct in_addr *ipv4_addr;
    socklen_t local_addr_len;
    struct addrinfo hints;

    sockfd = 0;

    // Setup addinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((gai_ret_val = getaddrinfo(external_ip, external_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("client: direcciones externas resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        ipv4_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it:
        inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));
        printf("->  IPv4: %s\n", ipv4_ipstr);
    }

    // Loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "client: no pudo realizarse el connect\n");
        return -1;
    }

    // Show server ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    ipv4_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));

    printf("client: conectado a %s:%d\n", ipv4_ipstr, ntohs(ipv4->sin_port));

    // Show client ip and port
    local_addr_len = sizeof(struct sockaddr_in);
    // Returns the current address to which the socket sockfd is bound
    getsockname(sockfd, (struct sockaddr *)&local_addr, &local_addr_len);
    inet_ntop(local_addr.sin_family, &local_addr.sin_addr, local_ip, sizeof(local_ip));

    // No need for ntohs() for port number here since it is a local address
    printf("client: dirección local %s:%d\n", local_ip, local_addr.sin_port);

    return sockfd;
}

int handle_connection(int sockfd)
{
    char message[DEFAULT_BUFFER_SIZE];
    ssize_t recv_val;
    Simple_Packet *send_packet, *recv_packet;

    send_packet = NULL;
    recv_packet = NULL;

    if (sockfd <= 0)
    {
        return -1;
    }

    // receive initial message from server
    recv_val = recv_simple_packet(sockfd, &recv_packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "client: conexión cerrada antes de recibir packet\n");
        free_simple_packet(recv_packet);
        recv_packet = NULL;
        return 0;
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "client: error al recibir packet\n");
        free_simple_packet(recv_packet);
        recv_packet = NULL;
        return -1;
    }

    printf("client: mensaje recibido: \"%s\"\n", recv_packet->data);
    free_simple_packet(recv_packet);
    recv_packet = NULL;

    // send PING message
    strcpy(message, "PING");
    if ((send_packet = create_simple_packet(message)) == NULL)
    {
        fprintf(stderr, "client: error al crear packet\n");
        return -1;
    }
    if (send_simple_packet(sockfd, send_packet) < 0)
    {
        fprintf(stderr, "client: error al enviar packet\n");
        free_simple_packet(send_packet);
        send_packet = NULL;
        return -1;
    }
    printf("client: mensaje enviado: \"%s\"\n", send_packet->data);
    free_simple_packet(send_packet);
    send_packet = NULL;
    // receive responmse message from server
    recv_val = recv_simple_packet(sockfd, &recv_packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "client: conexión cerrada antes de recibir packet\n");
        free_simple_packet(recv_packet);
        recv_packet = NULL;
        return 0;
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "client: error al recibir packet\n");
        free_simple_packet(recv_packet);
        recv_packet = NULL;
        return -1;
    }
    printf("client: mensaje recibido: \"%s\"\n", recv_packet->data);

    free_simple_packet(recv_packet);
    recv_packet = NULL;
    return 0;
}