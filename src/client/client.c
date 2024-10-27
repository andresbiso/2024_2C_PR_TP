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
    sockfd = setup_client(port_number, ip_number);
    if (sockfd <= 0)
    {
        return EXIT_FAILURE;
    }
    ret_val = handle_connection(sockfd);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

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
    puts("  --port <puerto> Especificar el número de puerto del servidor");
    puts("  --ip <ip> Especificar el número de ip del servidor");
}

void show_version()
{
    printf("Client Version %s\n", VERSION);
}

int setup_client(char *port_number, char *ip_number)
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
    hints.ai_socktype = SOCK_STREAM;

    if ((gai_ret_val = getaddrinfo(ip_number, port_number, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("client: direcciones resueltas");
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
    their_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, their_addr, their_ipstr, sizeof(their_ipstr));

    printf("client: conectado a %s:%s\n", their_ipstr, port_number);

    // Show client ip and port
    local_addr_len = sizeof(struct sockaddr_in);
    // Returns the current address to which the socket sockfd is bound
    getsockname(sockfd, (struct sockaddr *)&local_addr, &local_addr_len);
    inet_ntop(local_addr.sin_family, &local_addr.sin_addr, local_ip, sizeof(local_ip));
    local_port = ntohs(local_addr.sin_port);

    printf("client: dirección local %s:%d\n", local_ip, local_port);

    return sockfd;
}

int handle_connection(int server_sockfd)
{
    char message[DEFAULT_BUFFER_SIZE];
    ssize_t recv_val;
    Simple_Packet *send_packet, *recv_packet;

    // receive initial message from server
    recv_val = recv_simple_packet(server_sockfd, &recv_packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "client: conexión cerrada antes de recibir packet\n");
        free_simple_packet(recv_packet);
        return -1;
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "client: error al recibir packet\n");
        free_simple_packet(recv_packet);
        return -1;
    }

    printf("client: mensaje recibido: \"%s\"\n", recv_packet->data);
    free_simple_packet(recv_packet);

    // send PING message
    strcpy(message, "PING");
    if ((send_packet = create_simple_packet(message)) == NULL)
    {
        fprintf(stderr, "client: error al crear packet\n");
        return 1;
    }
    if (send_simple_packet(server_sockfd, send_packet) < 0)
    {
        fprintf(stderr, "client: error al enviar packet\n");
        free_simple_packet(send_packet);
        return -1;
    }
    printf("client: mensaje enviado: \"%s\"\n", send_packet->data);
    free_simple_packet(send_packet);
    // receive responmse message from server
    recv_val = recv_simple_packet(server_sockfd, &recv_packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "client: conexión cerrada antes de recibir packet\n");
        free_simple_packet(recv_packet);
        return -1;
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "client: error al recibir packet\n");
        free_simple_packet(recv_packet);
        return -1;
    }
    printf("client: mensaje recibido: \"%s\"\n", recv_packet->data);
    free_simple_packet(recv_packet);
    close(server_sockfd);
    return 0;
}