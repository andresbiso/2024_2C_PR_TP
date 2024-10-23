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

void *handle_client(void *arg)
{
    char *message, *client_ipstr;
    int client_port, client_sockfd;
    Simple_Packet *send_packet, *recv_packet;
    ClientData *client_data;

    if (malloc_string(&message, DEFAULT_BUFFER_SIZE) != 0)
    {
        exit(EXIT_FAILURE);
    }

    client_data = (ClientData *)arg;
    client_sockfd = client_data->client_sockfd;
    client_ipstr = client_data->client_ipstr;
    client_port = client_data->client_port;

    printf("Thread cliente (%s:%d): comienzo\n", client_ipstr, client_port);

    simulate_work();

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

    // Print completion message
    printf("Thread cliente (%s:%d): finalizado\n", client_ipstr, client_port);

    close(client_sockfd);
    free(client_data);
    free(message);

    return NULL;
}

// Comment: make sure that the functions used by both handlers return some of the values mentioned
// either that or adjust to the values I actually return.

// void *handle_client_read(void *arg)
// {
//     ClientData *client_data = (ClientData *)arg;
//     int client_sockfd = client_data->client_sockfd;
//     char message[DEFAULT_BUFFER_SIZE];
//     Simple_Packet *recv_packet;
//     int nbytes;

//     // receive initial message from client
//     if ((nbytes = recv_simple_packet(client_sockfd, &recv_packet)) <= 0)
//     {
//         if (nbytes == 0)
//         {
//             // Connection closed by client
//             printf("server: socket %d closed by client\n", client_sockfd);
//         }
//         else
//         {
//             perror("recv");
//         }
//         close(client_sockfd);
//         client_data->client_sockfd = -1; // Signal to handle_connections
//         free_simple_packet(recv_packet);
//         return NULL;
//     }
//     printf("server: mensaje recibido: \"%s\"\n", recv_packet->data);
//     // PONG message handling
//     if (strstr(recv_packet->data, "PING") != NULL)
//     {
//         printf("server: el mensaje contiene \"PING\"\n");
//         strcpy(message, "PONG");
//         send_response(client_sockfd, message);
//     }
//     free_simple_packet(recv_packet);
//     return NULL;
// }

// void *handle_client_write(void *arg)
// {
//     ClientData *client_data = (ClientData *)arg;
//     int client_sockfd = client_data->client_sockfd;
//     char message[DEFAULT_BUFFER_SIZE];
//     Simple_Packet *send_packet;
//     ssize_t nbytes;

//     // send initial server message
//     strcpy(message, "Hola, soy el server");
//     if (create_simple_packet(&send_packet, message) < 0)
//     {
//         fprintf(stderr, "Error al crear packet\n");
//         return NULL;
//     }
//     nbytes = send_simple_packet(client_sockfd, send_packet);
//     if (nbytes < 0)
//     {
//         if (errno == EPIPE || errno == ECONNRESET)
//         {
//             // Connection closed by client
//             printf("server: socket %d closed by client during send\n", client_sockfd);
//             close(client_sockfd);
//             client_data->client_sockfd = -1; // Signal to handle_connections
//         }
//         else
//         {
//             perror("send");
//         }
//         free_simple_packet(send_packet);
//         return NULL;
//     }
//     printf("server: mensaje enviado: \"%s\"\n", send_packet->data);
//     free_simple_packet(send_packet);
//     return NULL;
// }

void handle_connections(int sockfd)
{
    char their_ipstr[INET_ADDRSTRLEN];
    int their_port, new_fd;
    pthread_t thread;
    pthread_attr_t attr;
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    ClientData *client_data;

    // Main accept() loop
    while (1)
    {
        sin_size = sizeof(their_addr);
        // new connection on new_fd
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

        if ((client_data = (ClientData *)malloc(sizeof(ClientData))) == NULL)
        {
            perror("malloc");
            close(new_fd);
            exit(EXIT_FAILURE);
        }

        memset(client_data->client_ipstr, 0, sizeof(client_data->client_ipstr));
        client_data->client_sockfd = new_fd;
        strcpy(client_data->client_ipstr, their_ipstr);
        client_data->client_port = their_port;

        pthread_attr_init(&attr);
        // detached: the system will automatically clean up the resources of the thread when it terminates.
        // no need to call pthread_join to clean up and retrieve the thread’s exit status.
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread, &attr, handle_client, client_data) != 0)
        {
            perror("pthread_create");
            close(new_fd);
            free(client_data);
        }

        pthread_attr_destroy(&attr);
    }
}

// void handle_connections(int sockfd)
// {
//     char their_ipstr[INET_ADDRSTRLEN];
//     int their_port, new_fd, i, rv;
//     struct sockaddr their_addr;
//     socklen_t sin_size;
//     ClientData *client_data;
//     pthread_t thread;
//     pthread_attr_t attr;
//     fd_set master, read_fds, write_fds, except_fds;
//     int fdmax;

//     FD_ZERO(&master);
//     FD_ZERO(&read_fds);
//     FD_ZERO(&write_fds);
//     FD_ZERO(&except_fds);
//     FD_SET(sockfd, &master);
//     fdmax = sockfd;

//     // Inicializar atributos del hilo
//     pthread_attr_init(&attr);
//     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

//     while (1)
//     {
//         read_fds = master;
//         write_fds = master;
//         except_fds = master;

//         if ((rv = select(fdmax + 1, &read_fds, &write_fds, &except_fds, NULL)) == -1)
//         {
//             perror("select");
//             exit(EXIT_FAILURE);
//         }

//         for (i = 0; i <= fdmax; i++)
//         {
//             if (FD_ISSET(i, &read_fds))
//             {
//                 if (i == sockfd)
//                 {
//                     sin_size = sizeof(their_addr);
//                     if ((new_fd = accept(sockfd, &their_addr, &sin_size)) == -1)
//                     {
//                         perror("accept");
//                     }
//                     else
//                     {
//                         FD_SET(new_fd, &master);
//                         if (new_fd > fdmax)
//                         {
//                             fdmax = new_fd;
//                         }
//                         inet_ntop(their_addr.sa_family,
//                                   &(((struct sockaddr_in *)&their_addr)->sin_addr),
//                                   their_ipstr, sizeof their_ipstr);
//                         their_port = ((struct sockaddr_in *)&their_addr)->sin_port;
//                         printf("server: obtained connection from %s:%d\n", their_ipstr, their_port);
//                     }
//                 }
//                 else
//                 {
//                     if ((client_data = (ClientData *)malloc(sizeof(ClientData))) == NULL)
//                     {
//                         perror("malloc");
//                         close(i);
//                         FD_CLR(i, &master);
//                         continue;
//                     }
//                     client_data->client_sockfd = i;
//                     inet_ntop(their_addr.sa_family,
//                               &(((struct sockaddr_in *)&their_addr)->sin_addr),
//                               client_data->client_ipstr,
//                               sizeof(client_data->client_ipstr));
//                     client_data->client_port = their_port;

//                     if (pthread_create(&thread, &attr, handle_client_read, (void *)client_data) != 0)
//                     {
//                         perror("pthread_create");
//                         close(i);
//                         FD_CLR(i, &master);
//                         free(client_data);
//                     }

//                     // Check if socket needs to be removed (closed by client)
//                     if (client_data->client_sockfd == -1)
//                     {
//                         FD_CLR(i, &master);
//                     }
//                 }
//             }
//             else if (FD_ISSET(i, &write_fds))
//             {
//                 if ((client_data = (ClientData *)malloc(sizeof(ClientData))) == NULL)
//                 {
//                     perror("malloc");
//                     close(i);
//                     FD_CLR(i, &master);
//                     continue;
//                 }
//                 client_data->client_sockfd = i;
//                 inet_ntop(their_addr.sa_family,
//                           &(((struct sockaddr_in *)&their_addr)->sin_addr),
//                           client_data->client_ipstr,
//                           sizeof(client_data->client_ipstr));
//                 client_data->client_port = their_port;

//                 if (pthread_create(&thread, &attr, handle_client_write, (void *)client_data) != 0)
//                 {
//                     perror("pthread_create");
//                     close(i);
//                     FD_CLR(i, &master);
//                     free(client_data);
//                 }

//                 // Check if socket needs to be removed (closed by client)
//                 if (client_data->client_sockfd == -1)
//                 {
//                     FD_CLR(i, &master);
//                 }
//             }
//             else if (FD_ISSET(i, &except_fds))
//             {
//                 // handle exceptions on the socket
//                 perror("exception on socket");
//                 close(i);
//                 FD_CLR(i, &master);
//             }
//         }
//     }

//     // Destruir atributos del hilo
//     pthread_attr_destroy(&attr);
// }

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
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof(my_ipstr));

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