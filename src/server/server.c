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

volatile sig_atomic_t stop;
// pthread_mutex_t lock;

int main(int argc, char *argv[])
{
    char ip_number[INET_ADDRSTRLEN], port_number_tcp[PORTSTRLEN], port_number_udp[PORTSTRLEN];
    int ret_val;
    int sockfd_tcp; // listen on these sockfd
    Heartbeat_Data *heartbeat_data;

    strcpy(ip_number, DEFAULT_IP);
    strcpy(port_number_tcp, DEFAULT_PORT_TCP);
    strcpy(port_number_udp, DEFAULT_PORT_UDP);

    // if (pthread_mutex_init(&lock, NULL) != 0)
    // {
    //     perror("server: error en mutex init\n");
    //     return EXIT_FAILURE;
    // }

    ret_val = parse_arguments(argc, argv, ip_number, port_number_tcp, port_number_udp);
    if (ret_val > 0)
    {
        return EXIT_SUCCESS;
    }
    else if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }
    sockfd_tcp = setup_server_tcp(ip_number, port_number_tcp);
    if (sockfd_tcp <= 0)
    {
        return EXIT_FAILURE;
    }
    heartbeat_data = setup_server_udp(ip_number, port_number_udp);
    if (heartbeat_data == NULL)
    {
        return EXIT_FAILURE;
    }
    setup_signals();
    ret_val = handle_connections(sockfd_tcp, heartbeat_data);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

    close(sockfd_tcp);
    free_heartbeat_data(heartbeat_data);
    // pthread_mutex_destroy(&lock);
    puts("server: finalizando");
    return EXIT_SUCCESS;
}

int parse_arguments(int argc, char *argv[], char *ip_number, char *port_number_tcp, char *port_number_udp)
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
            else if (strcmp(argv[i], "--ip") == 0 && i + 1 < argc)
            {
                strcpy(ip_number, argv[i + 1]);
                i++; // Skip the next argument since it's the IP address
            }
            else if (strcmp(argv[i], "--port-tcp") == 0 && i + 1 < argc)
            {
                strcpy(port_number_tcp, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--port-udp") == 0 && i + 1 < argc)
            {
                strcpy(port_number_udp, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
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
    puts("  --ip <ip> Especificar el número de ip");
    puts("  --port-tcp <puerto> Especificar el número de puerto tcp");
    puts("  --port-udp <puerto> Especificar el número de puerto udp");
}

void show_version()
{
    printf("Server Version %s\n", VERSION);
}

int setup_server_tcp(char *ip_number, char *port_number)
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
        fprintf(stderr, "server: TCP getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("server: TCP direcciones resueltas");
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
            perror("server: TCP socket");
            continue;
        }

        // Ask the kernel to let me reuse the socket if already in use from previous run
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("server: TCP setsockopt");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: TCP bind");
            continue;
        }

        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: TCP no pudo realizarse el bind\n");
        return -1;
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("server: TCP listen");
        return -1;
    }

    // Show listening ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    my_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof(my_ipstr));

    printf("server: TCP %s:%s\n", my_ipstr, port_number);
    puts("server: TCP esperando conexiones...");

    return sockfd;
}

Heartbeat_Data *setup_server_udp(char *ip_number, char *port_number)
{
    int gai_ret_val, sockfd;
    char my_ipstr[INET_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *ipv4;
    struct in_addr *my_addr;
    socklen_t yes;
    Heartbeat_Data *data;

    yes = 1;
    data = NULL;

    // Setup addinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((gai_ret_val = getaddrinfo(ip_number, port_number, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "server: UDP getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("server: UDP direcciones resueltas");
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
            perror("server: UDP socket");
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
            perror("server: UDP bind");
            continue;
        }

        break;
    }

    // Free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: UDP no pudo realizarse el bind\n");
        return -1;
    }

    data = create_heartbeat_data(sockfd);
    if (data == NULL)
    {
        return NULL;
    }

    // Show listening ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    my_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof(my_ipstr));

    printf("server: UDP %s:%s\n", my_ipstr, port_number);
    puts("server: UDP esperando conexiones...");

    return data;
}

int handle_connections(int sockfd_tcp, Heartbeat_Data *heartbeat_data)
{
    char their_ipstr[INET_ADDRSTRLEN];
    int i, max_fd, new_fd, ret_val, their_port, select_ret;
    fd_set master, read_fds, write_fds, except_fds;
    pthread_t thread;
    pthread_attr_t attr;
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    Client_Tcp_Data **clients;
    Thread_Result *thread_result;

    ret_val = 0;

    // Initialize sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    // Add sockfd to the master set
    FD_SET(sockfd_tcp, &master);
    FD_SET(heartbeat_data->sockfd, &master);
    max_fd = sockfd_tcp > heartbeat_data->sockfd ? sockfd_tcp : heartbeat_data->sockfd;

    clients = init_clients_tcp_data(MAX_CLIENTS);
    if (clients == NULL)
    {
        return -1;
    }

    // Initialize thread attributes
    pthread_attr_init(&attr);
    // joinable: the system will not automatically clean up the resources of the thread when it terminates.
    // We need to call pthread_join to clean up and retrieve the thread’s exit status.
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Main accept() loop
    stop = 0;
    while (!stop)
    {
        // Reset sets to master value
        read_fds = master;
        write_fds = master;
        except_fds = master;

        // First argument is always nfds = max_fd + 1
        if ((select_ret = select(max_fd + 1, &read_fds, &write_fds, &except_fds, NULL)) == -1)
        {
            perror("server: select");
            ret_val = -1;
            break;
        }

        for (i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                // if it is the TCP listening socket
                if (i == sockfd_tcp)
                {
                    // handle new connection
                    sin_size = sizeof(their_addr);
                    if ((new_fd = accept(sockfd_tcp, &their_addr, &sin_size)) == -1)
                    {
                        ret_val = -1;
                        perror("server: accept");
                        break;
                    }

                    // Add new_fd to the master set
                    FD_SET(new_fd, &master);
                    // Update fdmax value
                    if (new_fd > max_fd)
                    {
                        max_fd = new_fd;
                    }

                    inet_ntop(their_addr.sa_family,
                              &(((struct sockaddr_in *)&their_addr)->sin_addr),
                              their_ipstr,
                              sizeof(their_ipstr));
                    their_port = ((struct sockaddr_in *)&their_addr)->sin_port;

                    if (create_client_tcp_data(clients[new_fd], new_fd, their_ipstr, their_port) < 0)
                    {
                        ret_val = -1;
                        FD_CLR(new_fd, &master);
                        close(new_fd);
                        break;
                    }

                    printf("server: obtuvo conexión de %s:%d\n", their_ipstr, their_port);
                }
                // if it is the Heartbeat listening socket
                else if (i == heartbeat_data->sockfd)
                {
                    // handle read
                    if (pthread_create(&thread, &attr, handle_client_heartbeat_read, (void *)heartbeat_data) != 0)
                    {
                        perror("server: pthread_create");
                        ret_val = -1;
                        break;
                    }

                    pthread_join(thread, (void **)&thread_result);
                    if (thread_result != NULL)
                    {
                        if (thread_result->value == THREAD_RESULT_ERROR)
                        {
                            perror("server: error en lectura");
                        }
                        if (thread_result->value == THREAD_RESULT_ERROR || thread_result->value == THREAD_RESULT_CLOSED)
                        {
                            ret_val = -1;
                        }
                        free(thread_result);
                    }
                    else
                    {
                        ret_val = -1;
                    }
                    continue;
                }
                else
                {
                    // handle read
                    if (pthread_create(&thread, &attr, handle_client_simple_read, (void *)clients[i]) != 0)
                    {
                        perror("server: pthread_create");
                        ret_val = -1;
                        free_client_tcp_data(&clients[i]);
                        FD_CLR(i, &master);
                        break;
                    }

                    pthread_join(thread, (void **)&thread_result);
                    if (thread_result != NULL)
                    {
                        if (thread_result->value == THREAD_RESULT_ERROR)
                        {
                            perror("server: error en lectura");
                        }
                        if (thread_result->value == THREAD_RESULT_ERROR || thread_result->value == THREAD_RESULT_CLOSED)
                        {
                            ret_val = -1;
                            free_client_tcp_data(&clients[i]);
                            FD_CLR(i, &master);
                        }
                        free(thread_result);
                    }
                    else
                    {
                        ret_val = -1;
                        free_client_tcp_data(&clients[i]);
                        FD_CLR(i, &master);
                    }
                    continue;
                }
            }
            else if (FD_ISSET(i, &write_fds))
            {
                // if it is the Heartbeat listening socket
                if (i == heartbeat_data->sockfd)
                {
                    // handle write
                    if (pthread_create(&thread, &attr, handle_client_heartbeat_write, (void *)heartbeat_data) != 0)
                    {
                        perror("server: pthread_create");
                        ret_val = -1;
                        break;
                    }

                    pthread_join(thread, (void **)&thread_result);
                    if (thread_result != NULL)
                    {
                        if (thread_result->value == THREAD_RESULT_ERROR)
                        {
                            perror("server: error en escritura");
                        }
                        if (thread_result->value == THREAD_RESULT_ERROR || thread_result->value == THREAD_RESULT_CLOSED)
                        {
                            ret_val = -1;
                        }
                        free(thread_result);
                    }
                    else
                    {
                        ret_val = -1;
                    }
                    continue;
                }
                else
                {
                    // handle write
                    if (pthread_create(&thread, &attr, handle_client_simple_write, (void *)clients[i]) != 0)
                    {
                        perror("server: pthread_create");
                        ret_val = -1;
                        free_client_tcp_data(&clients[i]);
                        FD_CLR(i, &master);
                        break;
                    }
                    pthread_join(thread, (void **)&thread_result);
                    if (thread_result != NULL)
                    {
                        if (thread_result->value == THREAD_RESULT_ERROR)
                        {
                            perror("server: error en escritura");
                        }
                        if (thread_result->value == THREAD_RESULT_ERROR || thread_result->value == THREAD_RESULT_CLOSED)
                        {
                            ret_val = -1;
                            free_client_tcp_data(&clients[i]);
                            FD_CLR(i, &master);
                        }
                        free(thread_result);
                    }
                    else
                    {
                        ret_val = -1;
                        free_client_tcp_data(&clients[i]);
                        FD_CLR(i, &master);
                    }
                    continue;
                }
            }
            else if (FD_ISSET(i, &except_fds))
            {
                // handle exceptions on the socket
                perror("server: excepción en socket");
                if (i != sockfd_tcp && i != heartbeat_data->sockfd)
                {
                    ret_val = -1;
                    free_client_tcp_data(&clients[i]);
                    FD_CLR(i, &master);
                }
                continue;
            }
        } // end for
    } // end while

    // Cleanup after loop
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    free_clients_tcp_data(clients, MAX_CLIENTS);
    pthread_attr_destroy(&attr);
    return ret_val;
}

void *handle_client_simple_read(void *arg)
{
    ssize_t recv_val;
    Client_Tcp_Data *client_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    client_data = (Client_Tcp_Data *)arg;
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    if (thread_result == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    printf("Thread cliente (%s:%d): lectura comienzo\n", client_data->client_ipstr, client_data->client_port);

    simulate_work();

    if (client_data->packet != NULL)
    {
        free_simple_packet(client_data->packet);
    }

    // receive initial message from client
    recv_val = recv_simple_packet(client_data->client_sockfd, &client_data->packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "server: conexión cerrada antes de recibir packet\n");
        free_simple_packet(client_data->packet);
        thread_result->value = THREAD_RESULT_CLOSED;
        pthread_exit((void *)thread_result);
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "server: error al recibir packet\n");
        free_simple_packet(client_data->packet);
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }
    printf("server: mensaje recibido: \"%s\"\n", client_data->packet->data);

    // Print completion message
    printf("Thread cliente (%s:%d): lectura fin\n", client_data->client_ipstr, client_data->client_port);

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_simple_write(void *arg)
{
    char message[DEFAULT_BUFFER_SIZE];
    Client_Tcp_Data *client_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    client_data = (Client_Tcp_Data *)arg;
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    if (thread_result == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    printf("Thread cliente (%s:%d): escritura comienzo\n", client_data->client_ipstr, client_data->client_port);

    simulate_work();
    // send PONG message
    if (client_data->packet != NULL && strstr(client_data->packet->data, "PING") != NULL)
    {
        puts("server: el mensaje contiene \"PING\"");
        free_simple_packet(client_data->packet);
        strcpy(message, "PONG");
        if ((client_data->packet = create_simple_packet(message)) == NULL)
        {
            fprintf(stderr, "server: error al crear packet\n");
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        if (send_simple_packet(client_data->client_sockfd, client_data->packet) < 0)
        {
            fprintf(stderr, "server: error al enviar packet\n");
            free_simple_packet(client_data->packet);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        printf("server: mensaje enviado: \"%s\"\n", client_data->packet->data);
    }
    else
    {
        // send initial server message
        strcpy(message, "Hola, soy el server");
        if ((client_data->packet = create_simple_packet(message)) == NULL)
        {
            fprintf(stderr, "server: error al crear packet\n");
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        if (send_simple_packet(client_data->client_sockfd, client_data->packet) < 0)
        {
            fprintf(stderr, "server: Error al enviar packet\n");
            free_simple_packet(client_data->packet);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        printf("server: mensaje enviado: \"%s\"\n", client_data->packet->data);
    }

    // Print completion message
    printf("Thread cliente (%s:%d): escritura fin\n", client_data->client_ipstr, client_data->client_port);

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_heartbeat_read(void *arg)
{
    char *client_ipstr;
    socklen_t addrlen;
    ssize_t recv_val;
    struct sockaddr src_addr;
    struct sockaddr_in *client_ipv4;
    struct sin_addr *client_addr;
    Heartbeat_Data *heartbeat_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    heartbeat_data = (Heartbeat_Data *)arg;
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    if (thread_result == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    puts("Thread Heartbeat: lectura comienzo");

    simulate_work();

    if (heartbeat_data->packet != NULL)
    {
        free_heartbeat_packet(heartbeat_data->packet);
    }

    // receive HEARTBEAT message from client
    recv_val = recv_heartbeat_packet(heartbeat_data->sockfd, heartbeat_data->packet, &heartbeat_data->addr, heartbeat_data->addrlen);
    if (recv_val == 0)
    {
        fprintf(stderr, "server: conexión cerrada antes de recibir packet\n");
        free_heartbeat_packet(heartbeat_data->packet);
        thread_result->value = THREAD_RESULT_CLOSED;
        pthread_exit((void *)thread_result);
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "server: error al recibir packet\n");
        free_heartbeat_packet(heartbeat_data->packet);
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }

    client_ipv4 = (struct sockaddr_in *)&heartbeat_data->addr;
    client_addr = &(client_ipv4->sin_addr);
    inet_ntop(client_ipv4->sin_family, client_addr, client_ipstr, sizeof(client_ipstr));

    printf("server: mensaje del cliente: %s:%ld\n", client_ipstr, ntohs(client_ipv4->sin_port));
    printf("server: mensaje recibido: %s - %ld\n", heartbeat_data->packet->message, heartbeat_data->packet->timestamp);

    // Print completion message
    puts("Thread Heartbeat: lectura fin");

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_heartbeat_write(void *arg)
{
    char message[DEFAULT_BUFFER_SIZE];
    char *client_ipstr;
    socklen_t addrlen;
    ssize_t recv_val;
    struct sockaddr src_addr;
    struct sockaddr_in *client_ipv4;
    struct sin_addr *client_addr;
    Heartbeat_Data *heartbeat_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    heartbeat_data = (Heartbeat_Data *)arg;
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    if (thread_result == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    puts("Thread Heartbeat: escritura comienzo");

    simulate_work();

    // send ACK message
    if (heartbeat_data->packet != NULL && strstr(heartbeat_data->packet->message, "HEARTBEAT") != NULL)
    {
        puts("server: el mensaje contiene \"HEARTBEAT\"");
        free_heartbeat_packet(heartbeat_data->packet);
        strcpy(message, "ACK");
        if ((heartbeat_data->packet = create_simple_packet(message)) == NULL)
        {
            fprintf(stderr, "server: error al crear packet\n");
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        if (send_heartbeat_packet(heartbeat_data->sockfd, heartbeat_data->packet, &heartbeat_data->addr, heartbeat_data->addrlen) < 0)
        {
            fprintf(stderr, "server: error al enviar packet\n");
            free_heartbeat_packet(heartbeat_data->packet);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        client_ipv4 = (struct sockaddr_in *)&heartbeat_data->addr;
        client_addr = &(client_ipv4->sin_addr);
        inet_ntop(client_ipv4->sin_family, client_addr, client_ipstr, sizeof(client_ipstr));

        printf("server: mensaje al cliente: %s:%ld\n", client_ipstr, ntohs(client_ipv4->sin_port));
        printf("server: mensaje enviado: %s - %ld\n", heartbeat_data->packet->message, heartbeat_data->packet->timestamp);
    }

    // Print completion message
    puts("Thread Heartbeat: escritura fin");

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

int create_client_tcp_data(Client_Tcp_Data *data, int sockfd, const char *ipstr, in_port_t port)
{
    if (data == NULL || sockfd <= 0 || ipstr == NULL || port <= 0)
    {
        return -1;
    }

    // Initialize allocated memory to zero
    data->client_sockfd = sockfd;
    strcpy(data->client_ipstr, ipstr);
    data->client_port = port;
    data->packet = NULL;

    return 0;
}

Client_Tcp_Data **init_clients_tcp_data(int len)
{
    Client_Tcp_Data **clients;
    clients = (Client_Tcp_Data **)malloc(len * sizeof(Client_Tcp_Data *));
    if (clients == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(clients, 0, len * sizeof(Client_Tcp_Data *));
    return clients;
}

void free_clients_tcp_data(Client_Tcp_Data **clients, int len)
{
    for (int i = 0; i < len; i++)
    {
        free_client_tcp_data(&clients[i]);
    }
}

void free_client_tcp_data(Client_Tcp_Data **client)
{
    if (*client != NULL)
    {
        if ((*client)->client_sockfd > 0)
        {
            close((*client)->client_sockfd);
        }
        free_simple_packet((*client)->packet);
        free(*client);
        *client = NULL;
    }
}

void setup_signals()
{
    signal(SIGINT, handle_sigint);
}

void handle_sigint(int sig)
{
    puts("Ctrl+C presionado. Finalizando...");
    stop = 1;
}