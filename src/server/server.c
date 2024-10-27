// Standard library headers
#include <errno.h>
#include <pthread.h>
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
    sockfd = setup_server(port_number, ip_number);
    if (sockfd <= 0)
    {
        return EXIT_FAILURE;
    }
    ret_val = handle_connections(sockfd);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

    puts("server: finalizando");
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
                printf("server: opción o argumento no soportado: %s\n", argv[i]);
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

int setup_server(char *port_number, char *ip_number)
{
    int gai_ret_val, sockfd;
    char my_ipstr[INET_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *ipv4;
    struct in_addr *my_addr;
    socklen_t yes;

    yes = 1;
    sockfd = 0;

    // Setup addinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((gai_ret_val = getaddrinfo(ip_number, port_number, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("server: direcciones resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        my_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it
        inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof(my_ipstr));
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
            continue;
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
        return -1;
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("server: listen");
        return -1;
    }

    // Show listening ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    my_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof(my_ipstr));

    printf("server: %s:%s\n", my_ipstr, port_number);
    puts("server: esperando conexiones...");

    return sockfd;
}

int handle_connections(int sockfd)
{
    char their_ipstr[INET_ADDRSTRLEN];
    int their_port, new_fd, num_accept, ret_val;
    pthread_t thread;
    pthread_attr_t attr;
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    Client_Data *client_data;

    num_accept = 0;
    ret_val = 0;
    // Initialize thread attributes
    pthread_attr_init(&attr);
    // detached: the system will automatically clean up the resources of the thread when it terminates.
    // no need to call pthread_join to clean up and retrieve the thread’s exit status.
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Main accept() loop
    while (num_accept < 2)
    {
        sin_size = sizeof(their_addr);
        // new connection on new_fd
        if ((new_fd = accept(sockfd, &their_addr, &sin_size)) == -1)
        {
            ret_val = -1;
            perror("server: accept");
            break;
        }
        num_accept++;
        inet_ntop(their_addr.sa_family,
                  &(((struct sockaddr_in *)&their_addr)->sin_addr),
                  their_ipstr,
                  sizeof(their_ipstr));
        their_port = ((struct sockaddr_in *)&their_addr)->sin_port;
        printf("server: obtuvo conexión de %s:%d\n", their_ipstr, their_port);

        client_data = create_client_data(new_fd, their_ipstr, their_port);
        if (client_data == NULL)
        {
            ret_val = -1;
            close(new_fd);
            break;
        }

        if (pthread_create(&thread, &attr, handle_client, client_data) != 0)
        {
            ret_val = -1;
            perror("server: pthread_create");
            free(client_data);
            close(new_fd);
            break;
        }
    }

    // Cleanup after loop
    pthread_attr_destroy(&attr);
    return ret_val;
}

void *handle_client(void *arg)
{
    char message[DEFAULT_BUFFER_SIZE];
    ssize_t recv_val;
    Simple_Packet *send_packet, *recv_packet;
    Client_Data *client_data;

    send_packet = NULL;
    recv_packet = NULL;

    if (arg == NULL)
    {
        return NULL;
    }

    client_data = (Client_Data *)arg;

    printf("Thread cliente (%s:%d): comienzo\n", client_data->client_ipstr, client_data->client_port);

    simulate_work();

    // send initial server message
    strcpy(message, "Hola, soy el server");
    if ((send_packet = create_simple_packet(message)) == NULL)
    {
        fprintf(stderr, "server: error al crear packet\n");
        close(client_data->client_sockfd);
        free(client_data);
        return NULL;
    }
    if (send_simple_packet(client_data->client_sockfd, send_packet) < 0)
    {
        fprintf(stderr, "server: Error al enviar packet\n");
        free_simple_packet(send_packet);
        close(client_data->client_sockfd);
        free(client_data);
        return NULL;
    }
    printf("server: mensaje enviado: \"%s\"\n", send_packet->data);
    free_simple_packet(send_packet);
    // receive initial message from client
    recv_val = recv_simple_packet(client_data->client_sockfd, &recv_packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "server: conexión cerrada antes de recibir packet\n");
        free_simple_packet(recv_packet);
        close(client_data->client_sockfd);
        free(client_data);
        return NULL;
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "server: error al recibir packet\n");
        free_simple_packet(recv_packet);
        close(client_data->client_sockfd);
        free(client_data);
        return NULL;
    }
    printf("server: mensaje recibido: \"%s\"\n", recv_packet->data);
    // send PONG message
    if (strstr(recv_packet->data, "PING") != NULL)
    {
        free_simple_packet(recv_packet);
        puts("server: el mensaje contiene \"PING\"");
        strcpy(message, "PONG");
        if ((send_packet = create_simple_packet(message)) == NULL)
        {
            fprintf(stderr, "server: error al crear packet\n");
            close(client_data->client_sockfd);
            free(client_data);
            return NULL;
        }
        if (send_simple_packet(client_data->client_sockfd, send_packet) < 0)
        {
            fprintf(stderr, "server: error al enviar packet\n");
            free_simple_packet(send_packet);
            close(client_data->client_sockfd);
            free(client_data);
            return NULL;
        }
        printf("server: mensaje enviado: \"%s\"\n", send_packet->data);
        free_simple_packet(send_packet);
    }

    // Print completion message
    printf("Thread cliente (%s:%d): finalizado\n", client_data->client_ipstr, client_data->client_port);

    close(client_data->client_sockfd);
    free(client_data);
    return NULL;
}

Client_Data *create_client_data(int sockfd, const char *ipstr, in_port_t port)
{
    Client_Data *data;

    if (sockfd <= 0 || ipstr == NULL || port <= 0)
    {
        return NULL;
    }

    data = (Client_Data *)malloc(sizeof(Client_Data));
    if (data == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL; // Memory allocation failed
    }
    // Initialize allocated memory to zero
    memset(data, 0, sizeof(Client_Data));

    data->client_sockfd = sockfd;
    strcpy(data->client_ipstr, ipstr);
    data->client_port = port;

    return data;
}