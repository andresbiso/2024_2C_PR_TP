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
#include "server.h"

int main(int argc, char *argv[])
{
    char *ip_number, *port_number;
    int sockfd; // listen on sock_fd

    if (malloc_string(&ip_number, INET_ADDRSTRLEN) != 0)
    {
        exit(EXIT_FAILURE);
    }
    if (malloc_string(&port_number, PORTSTRLEN) != 0)
    {
        exit(EXIT_FAILURE);
    }
    strcpy(ip_number, DEFAULT_IP);
    strcpy(port_number, DEFAULT_PORT);

    parse_arguments(argc, argv, &port_number, &ip_number);
    sockfd = setup_server(port_number, ip_number);
    setup_signal_handler();
    handle_connections(sockfd);

    // free allocated memory
    free(ip_number);
    free(port_number);

    return EXIT_SUCCESS;
}

void handle_client(int client_sockfd)
{
    char *message; // new connection on new_fd
    Simple_Packet *send_packet, *recv_packet;

    if (malloc_string(&message, DEFAULT_BUFFER_SIZE) != 0)
    {
        exit(EXIT_FAILURE);
    }

    // send initial server message
    strcpy(message, "Hola, soy el server");
    if (create_simple_packet(&send_packet, message) < 0)
    {
        fprintf(stderr, "Error al crear packet\n");
        free(message);
        exit(EXIT_FAILURE);
    }
    if (send_simple_packet(client_sockfd, send_packet) < 0)
    {
        fprintf(stderr, "Error al enviar packet\n");
        free_simple_packet(send_packet);
        free(message);
        exit(EXIT_FAILURE);
    }
    printf("server: mensaje enviado: \"%s\"\n", send_packet->data);
    free_simple_packet(send_packet);
    // receive initial message from client
    if (recv_simple_packet(client_sockfd, &recv_packet) < 0)
    {
        fprintf(stderr, "Error al recibir packet\n");
        free_simple_packet(recv_packet);
        free(message);
        exit(EXIT_FAILURE);
    }
    printf("server: mensaje recibido: \"%s\"\n", recv_packet->data);
    // send PONG message
    if (strstr(recv_packet->data, "PING") != NULL)
    {
        free_simple_packet(recv_packet);
        puts("server: el mensaje contiene \"PING\"");
        strcpy(message, "PONG");
        if (create_simple_packet(&send_packet, message) < 0)
        {
            fprintf(stderr, "Error al crear packet\n");
            free(message);
            exit(EXIT_FAILURE);
        }
        if (send_simple_packet(client_sockfd, send_packet) < 0)
        {
            fprintf(stderr, "Error al enviar packet\n");
            free_simple_packet(send_packet);
            free(message);
            exit(EXIT_FAILURE);
        }
        printf("server: mensaje enviado: \"%s\"\n", send_packet->data);
        free_simple_packet(send_packet);
    }
    close(client_sockfd);

    free(message);
}

void handle_connections(int sockfd)
{
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    char their_ipstr[INET_ADDRSTRLEN];
    int their_port;
    int new_fd; // new connection on new_fd

    // Main accept() loop
    while (1)
    {
        sin_size = sizeof their_addr;
        if ((new_fd = accept(sockfd, &their_addr, &sin_size)) == -1)
        {
            perror("server: accept");
            continue;
        }

        inet_ntop(their_addr.sa_family,
                  &(((struct sockaddr_in *)&their_addr)->sin_addr),
                  their_ipstr,
                  sizeof their_ipstr);
        their_port = ((struct sockaddr_in *)&their_addr)->sin_port;
        printf("server: obtuvo conexión de %s:%d\n", their_ipstr, their_port);

        if (fork() == 0)
        {
            // This is the child process
            // Child doesn't need the listener (sockfd)
            close(sockfd);
            handle_client(new_fd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // Parent doesn't need new_fd
            close(new_fd);
        }
    }
}

void parse_arguments(int argc, char *argv[], char **port_number, char **ip_number)
{
    if (argc >= 2)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "--help") == 0)
            {
                show_help();
                exit(EXIT_SUCCESS);
            }
            else if (strcmp(argv[i], "--version") == 0)
            {
                show_version();
                exit(EXIT_SUCCESS);
            }
            else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            {
                strcpy(*port_number, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--ip") == 0 && i + 1 < argc)
            {
                strcpy(*ip_number, argv[i + 1]);
                i++; // Skip the next argument since it's the IP address
            }
            else
            {
                printf("server: Opción o argumento no soportado: %s\n", argv[i]);
                show_help();
                exit(EXIT_FAILURE);
            }
        }
    }
}

int setup_server(char *port_number, char *ip_number)
{
    int returned_value, sockfd, yes = 1;
    char my_ipstr[INET_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *ipv4;
    void *my_addr;

    // Setup addinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((returned_value = getaddrinfo(ip_number, port_number, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(returned_value));
        return 1;
    }

    puts("server: direcciones resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        my_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it
        inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof my_ipstr);
        printf("->  IPv4: %s\n", my_ipstr);
    }

    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        // Ask the kernel to let me reuse the socket if already in use from previous run
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("server: setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: no pudo realizarse el bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("server: listen");
        exit(EXIT_FAILURE);
    }

    // Show listening ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    my_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof my_ipstr);

    printf("server: %s:%s\n", my_ipstr, port_number);
    puts("server: esperando conexiones...");

    return sockfd;
}

void show_help()
{
    puts("Uso: server [opciones]");
    puts("Opciones:");
    puts("  --help      Muestra este mensaje de ayuda");
    puts("  --version   Muestra version del programa");
    puts("  --port <puerto> Especificar el número de puerto");
    puts("  --ip <ip> Especificar el número de ip");
}

void show_version()
{
    printf("Server Version %s\n", VERSION);
}