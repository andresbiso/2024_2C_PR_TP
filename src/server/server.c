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

// Shared headers
#include "../shared/common.h"

// Project header
#include "server.h"

int main(int argc, char *argv[])
{
    char *ip_number, *port_number;
    int sockfd; // listen on sock_fd

    ip_number = initialize_string(INET_ADDRSTRLEN);
    port_number = initialize_string(PORTSTRLEN);
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
    char *data, *msg;
    int msglen;                               // new connection on new_fd
    size_t buffer_size = DEFAULT_BUFFER_SIZE; // Define the initial buffer size

    msg = initialize_string(DEFAULT_BUFFER_SIZE);
    data = initialize_string(DEFAULT_BUFFER_SIZE);

    // send initial server message
    strcpy(msg, "Hola, soy el server");
    msglen = strlen(msg);
    sendall(client_sockfd, msg, msglen);
    printf("server: mensaje enviado: \"%s\"\n", msg);
    // receive initial message from client
    recvall(client_sockfd, &data, &buffer_size);
    printf("server: mensaje recibido: \"%s\"\n", data);
    // send PONG message
    if (strstr(data, "PING") != NULL)
    {
        puts("server: el mensaje contiene \"PING\"");
        strcpy(msg, "PONG");
        msglen = strlen(msg);
        if (sendall(client_sockfd, msg, msglen) > 0)
        {
            printf("server: mensaje enviado: \"%s\"\n", msg);
        }
    }
    close(client_sockfd);
    free(data);
    free(msg);
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
        perror("listen");
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