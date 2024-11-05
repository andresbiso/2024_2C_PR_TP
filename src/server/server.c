// Standard library headers
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
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
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// Shared headers
#include "../shared/common.h"
#include "../shared/http.h"

// Project header
#include "server.h"

volatile sig_atomic_t stop;
pthread_mutex_t lock;
pthread_mutex_t lock_file;

int main(int argc, char *argv[])
{
    char local_ip[INET_ADDRSTRLEN], local_port_tcp[PORTSTRLEN],
        local_port_tcp_http[PORTSTRLEN], local_port_udp[PORTSTRLEN];
    int ret_val;
    int sockfd_tcp, sockfd_tcp_http, sockfd_udp; // listen on these sockfd

    strcpy(local_ip, LOCAL_IP);
    strcpy(local_port_tcp, LOCAL_PORT_TCP);
    strcpy(local_port_tcp_http, LOCAL_PORT_TCP_HTTP);
    strcpy(local_port_udp, LOCAL_PORT_UDP);

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("server: error en lock init\n");
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&lock_file, NULL) != 0)
    {
        perror("server: error en lock_file init\n");
        return EXIT_FAILURE;
    }

    ret_val = parse_arguments(argc, argv, local_ip, local_port_tcp, local_port_udp, local_port_tcp_http);
    if (ret_val > 0)
    {
        return EXIT_SUCCESS;
    }
    else if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }
    sockfd_tcp = setup_server_tcp(local_ip, local_port_tcp);
    if (sockfd_tcp <= 0)
    {
        return EXIT_FAILURE;
    }
    sockfd_udp = setup_server_udp(local_ip, local_port_udp);
    if (sockfd_udp <= 0)
    {
        return EXIT_FAILURE;
    }
    sockfd_tcp_http = setup_server_tcp(LOCAL_IP_EXPOSED, local_port_tcp_http);
    if (sockfd_tcp <= 0)
    {
        return EXIT_FAILURE;
    }
    printf("server: TCP %s:%d: exclusivo para HTTP\n", LOCAL_IP_EXPOSED, atoi(local_port_tcp_http));
    setup_signals();
    ret_val = handle_connections(sockfd_tcp, sockfd_udp, sockfd_tcp_http);
    if (ret_val < 0)
    {
        return EXIT_FAILURE;
    }

    close(sockfd_tcp);
    close(sockfd_udp);
    close(sockfd_tcp_http);
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lock_file);
    puts("server: finalizando");
    return EXIT_SUCCESS;
}

int parse_arguments(int argc, char *argv[], char *local_ip, char *local_port_tcp, char *local_port_udp, char *local_port_tcp_http)
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
            else if (strcmp(argv[i], "--local-port-tcp") == 0 && i + 1 < argc)
            {
                strcpy(local_port_tcp, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--local-port-udp") == 0 && i + 1 < argc)
            {
                strcpy(local_port_udp, argv[i + 1]);
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--local-port-tcp-http") == 0 && i + 1 < argc)
            {
                strcpy(local_port_tcp_http, argv[i + 1]);
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
    puts("  --help    Muestra este mensaje de ayuda");
    puts("  --version    Muestra version del programa");
    puts("  --local-ip <ip>    Especificar el número de ip local");
    puts("  --local-port-tcp <puerto>    Especificar el número de puerto tcp local");
    puts("  --local-port-udp <puerto>    Especificar el número de puerto udp local");
    puts("  --local-port-tcp-http <puerto>    Especificar el número de puerto tcp http local");
}

void show_version()
{
    printf("Server Version %s\n", VERSION);
}

int setup_server_tcp(char *local_ip, char *local_port)
{
    int gai_ret_val, sockfd;
    char ipv4_ipstr[INET_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *ipv4;
    struct in_addr *ipv4_addr;
    socklen_t yes;

    yes = 1;
    sockfd = 0;

    // Setup addinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((gai_ret_val = getaddrinfo(local_ip, local_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "server: TCP getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("server: TCP direcciones resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        ipv4_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it
        inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));
        printf("->  IPv4: %s\n", ipv4_ipstr);
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
    ipv4_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));

    printf("server: TCP %s:%d\n", ipv4_ipstr, ntohs(ipv4->sin_port));
    puts("server: TCP esperando conexiones...");

    return sockfd;
}

int setup_server_udp(char *local_ip, char *local_port)
{
    int gai_ret_val, sockfd;
    char ipv4_ipstr[INET_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *ipv4;
    struct in_addr *ipv4_addr;
    socklen_t yes;

    yes = 1;

    // Setup addinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET to force version IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((gai_ret_val = getaddrinfo(local_ip, local_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "server: UDP getaddrinfo: %s\n", gai_strerror(gai_ret_val));
        return -1;
    }

    puts("server: UDP direcciones resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        ipv4_addr = &(ipv4->sin_addr);

        // Convert the IP to a string and print it
        inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));
        printf("->  IPv4: %s\n", ipv4_ipstr);
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

    // Show listening ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    ipv4_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, ipv4_addr, ipv4_ipstr, sizeof(ipv4_ipstr));

    printf("server: UDP %s:%d\n", ipv4_ipstr, ntohs(ipv4->sin_port));
    puts("server: UDP esperando conexiones...");

    return sockfd;
}

int handle_connections(int sockfd_tcp, int sockfd_udp, int sockfd_tcp_http)
{
    char their_ipstr[INET_ADDRSTRLEN];
    int i, max_fd, new_fd, ret_val, their_port, select_ret;
    fd_set master, read_fds, write_fds, except_fds;
    pthread_t thread;
    pthread_attr_t attr;
    socklen_t sin_size;
    struct sockaddr their_addr; // connector's address information
    struct timeval timeout;
    Heartbeat_Data *heartbeat_data;
    Client_Tcp_Data **clients;
    Client_Http_Data **http_clients;
    Thread_Result *thread_result;
    void *result;

    ret_val = 0;

    // Initialize sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    // Add sockfd to the master set
    FD_SET(sockfd_tcp, &master);
    FD_SET(sockfd_udp, &master);
    FD_SET(sockfd_tcp_http, &master);
    max_fd = find_max(3, sockfd_tcp, sockfd_udp, sockfd_tcp_http);

    timeout.tv_sec = 1; // Wait for 1 seconds
    timeout.tv_usec = 0;

    heartbeat_data = create_heartbeat_data(sockfd_udp);
    if (heartbeat_data == NULL)
    {
        return -1;
    }

    clients = init_clients_tcp_data(MAX_CLIENTS);
    if (clients == NULL)
    {
        free_heartbeat_data(heartbeat_data);
        heartbeat_data = NULL;
        return -1;
    }

    http_clients = init_clients_http_data(MAX_CLIENTS);
    if (http_clients == NULL)
    {
        free_heartbeat_data(heartbeat_data);
        heartbeat_data = NULL;
        free_clients_tcp_data(clients, MAX_CLIENTS);
        clients = NULL;
        return -1;
    }

    // Initialize thread attributes
    pthread_attr_init(&attr);
    // joinable: the system will not automatically clean up the resources of the thread when it terminates.
    // We need to call pthread_join to clean up and retrieve the thread’s exit status.
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Main accept() loop
    stop = 0;
    while (stop == 0)
    {
        // Reset sets to master value
        read_fds = master;
        write_fds = master;
        except_fds = master;

        // Because of UDP socket sometimes select return really fast
        sleep(1); // Sleep for 1s before retry

        // First argument is always nfds = max_fd + 1
        select_ret = select(max_fd + 1, &read_fds, &write_fds, &except_fds, &timeout);
        // if select_ret = 0 -> o I/O event happened within the specified timeout period
        // if select_ret > 0 -> n file descriptors are ready
        if (select_ret > 0)
        {
            for (i = 0; i <= max_fd && select_ret > 0; i++)
            {
                if (FD_ISSET(i, &read_fds))
                {
                    select_ret--;
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

                        clients[new_fd] = create_client_tcp_data(new_fd, their_ipstr, their_port);
                        if (clients[new_fd] == NULL)
                        {
                            ret_val = -1;
                            FD_CLR(new_fd, &master);
                            close(new_fd);
                            break;
                        }

                        printf("server: obtuvo conexión de %s:%d\n", their_ipstr, their_port);
                        continue;
                    }
                    // if it is the TCP HTTP listening socket
                    else if (i == sockfd_tcp_http)
                    {
                        // handle new connection
                        sin_size = sizeof(their_addr);
                        if ((new_fd = accept(sockfd_tcp_http, &their_addr, &sin_size)) == -1)
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

                        http_clients[new_fd] = create_client_http_data(new_fd, their_ipstr, their_port);
                        if (http_clients[new_fd] == NULL)
                        {
                            ret_val = -1;
                            FD_CLR(new_fd, &master);
                            close(new_fd);
                            break;
                        }

                        printf("server: obtuvo conexión de %s:%d\n", their_ipstr, their_port);
                        continue;
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

                        if (pthread_join(thread, (void *)&result) != 0)
                        {
                            perror("server: pthread_join");
                            ret_val = -1;
                            break;
                        }

                        thread_result = (Thread_Result *)result;
                        if (thread_result != NULL)
                        {
                            if (thread_result->value == THREAD_RESULT_ERROR)
                            {
                                fprintf(stderr, "server: error en lectura de packet UDP\n");
                            }
                            free(thread_result);
                        }
                        else
                        {
                            ret_val = -1;
                            break;
                        }
                        continue;
                    }
                    else if (index_in_client_http_data_array(http_clients, MAX_CLIENTS, i))
                    {
                        // handle HTTP read
                        if (pthread_create(&thread, &attr, handle_client_http_read, (void *)http_clients[i]) != 0)
                        {
                            perror("server: pthread_create");
                            ret_val = -1;
                            free_client_http_data(&http_clients[i]);
                            FD_CLR(i, &master);
                            break;
                        }

                        if (pthread_join(thread, (void *)&result) != 0)
                        {
                            perror("server: pthread_join");
                            ret_val = -1;
                            free_client_http_data(&http_clients[i]);
                            FD_CLR(i, &master);
                            break;
                        }

                        thread_result = (Thread_Result *)result;
                        if (thread_result != NULL)
                        {
                            if (thread_result->value == THREAD_RESULT_ERROR)
                            {
                                printf("server: cliente (%s:%d) error en lectura\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                                printf("server: cliente (%s:%d) cerrando conexión\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                                free_client_http_data(&http_clients[i]);
                                FD_CLR(i, &master);
                            }
                            else if (thread_result->value == THREAD_RESULT_CLOSED)
                            {
                                printf("server: cliente (%s:%d) cerró su conexión\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                                free_client_http_data(&http_clients[i]);
                                FD_CLR(i, &master);
                            }
                            free(thread_result);
                        }
                        else
                        {
                            ret_val = -1;
                            break;
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

                        if (pthread_join(thread, (void *)&result) != 0)
                        {
                            perror("server: pthread_join");
                            ret_val = -1;
                            free_client_tcp_data(&clients[i]);
                            FD_CLR(i, &master);
                            break;
                        }

                        thread_result = (Thread_Result *)result;
                        if (thread_result != NULL)
                        {
                            if (thread_result->value == THREAD_RESULT_ERROR)
                            {
                                printf("server: cliente (%s:%d) error en lectura\n", clients[i]->client_ipstr, clients[i]->client_port);
                                printf("server: cliente (%s:%d) cerrando conexión\n", clients[i]->client_ipstr, clients[i]->client_port);
                                free_client_tcp_data(&clients[i]);
                                FD_CLR(i, &master);
                            }
                            else if (thread_result->value == THREAD_RESULT_CLOSED)
                            {
                                printf("server: cliente (%s:%d) cerró su conexión\n", clients[i]->client_ipstr, clients[i]->client_port);
                                free_client_tcp_data(&clients[i]);
                                FD_CLR(i, &master);
                            }
                            free(thread_result);
                        }
                        else
                        {
                            ret_val = -1;
                            break;
                        }
                        continue;
                    }
                }
                else if (FD_ISSET(i, &write_fds))
                {
                    select_ret--;
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

                        if (pthread_join(thread, (void *)&result) != 0)
                        {
                            perror("server: pthread_join");
                            ret_val = -1;
                            break;
                        }

                        thread_result = (Thread_Result *)result;
                        if (thread_result != NULL)
                        {
                            if (thread_result->value == THREAD_RESULT_ERROR)
                            {
                                fprintf(stderr, "server: error en escritura de packet UDP\n");
                            }
                            free(thread_result);
                        }
                        else
                        {
                            ret_val = -1;
                            break;
                        }
                        continue;
                    }
                    else if (index_in_client_http_data_array(http_clients, MAX_CLIENTS, i))
                    {
                        // handle HTTP write
                        if (pthread_create(&thread, &attr, handle_client_http_write, (void *)http_clients[i]) != 0)
                        {
                            perror("server: pthread_create");
                            ret_val = -1;
                            free_client_http_data(&http_clients[i]);
                            FD_CLR(i, &master);
                            break;
                        }

                        if (pthread_join(thread, (void *)&result) != 0)
                        {
                            perror("server: pthread_join");
                            ret_val = -1;
                            free_client_http_data(&http_clients[i]);
                            FD_CLR(i, &master);
                            break;
                        }

                        thread_result = (Thread_Result *)result;
                        if (thread_result != NULL)
                        {
                            if (thread_result->value == THREAD_RESULT_ERROR)
                            {
                                printf("server: cliente (%s:%d) error en escritura\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                                printf("server: cliente (%s:%d) cerrando conexión\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                                free_client_http_data(&http_clients[i]);
                                FD_CLR(i, &master);
                            }
                            else if (thread_result->value == THREAD_RESULT_CLOSED)
                            {
                                printf("server: cliente (%s:%d) cerró su conexión\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                                free_client_http_data(&http_clients[i]);
                                FD_CLR(i, &master);
                            }
                            free(thread_result);
                        }
                        else
                        {
                            ret_val = -1;
                            break;
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

                        if (pthread_join(thread, (void *)&result) != 0)
                        {
                            perror("server: pthread_join");
                            ret_val = -1;
                            free_client_tcp_data(&clients[i]);
                            FD_CLR(i, &master);
                            break;
                        }

                        thread_result = (Thread_Result *)result;
                        if (thread_result != NULL)
                        {
                            if (thread_result->value == THREAD_RESULT_ERROR)
                            {
                                printf("server: cliente (%s:%d) error en escritura\n", clients[i]->client_ipstr, clients[i]->client_port);
                                printf("server: cliente (%s:%d) cerrando conexión\n", clients[i]->client_ipstr, clients[i]->client_port);
                                free_client_tcp_data(&clients[i]);
                                FD_CLR(i, &master);
                            }
                            else if (thread_result->value == THREAD_RESULT_CLOSED)
                            {
                                printf("server: cliente (%s:%d) cerró su conexión\n", clients[i]->client_ipstr, clients[i]->client_port);
                                free_client_tcp_data(&clients[i]);
                                FD_CLR(i, &master);
                            }
                            free(thread_result);
                        }
                        else
                        {
                            ret_val = -1;
                            break;
                        }
                        continue;
                    }
                }
                else if (FD_ISSET(i, &except_fds))
                {
                    select_ret--;
                    // handle exceptions on the socket
                    fprintf(stderr, "server: excepción en socket\n");
                    if (i != sockfd_tcp && i != heartbeat_data->sockfd && i != sockfd_tcp_http)
                    {
                        if (index_in_client_http_data_array(http_clients, MAX_CLIENTS, i))
                        {
                            printf("server: cliente (%s:%d) problema en conexión\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                            printf("server: cliente (%s:%d) cerrando conexión\n", http_clients[i]->client_ipstr, http_clients[i]->client_port);
                            free_client_http_data(&http_clients[i]);
                            FD_CLR(i, &master);
                        }
                        else
                        {
                            printf("server: cliente (%s:%d) problema en conexión\n", clients[i]->client_ipstr, clients[i]->client_port);
                            printf("server: cliente (%s:%d) cerrando conexión\n", clients[i]->client_ipstr, clients[i]->client_port);
                            free_client_tcp_data(&clients[i]);
                            FD_CLR(i, &master);
                        }
                        continue;
                    }
                    else
                    {
                        ret_val = -1;
                        break;
                    }
                }
            } // end for
        } // end select_ret > 0
        else if (select_ret < 0)
        {
            perror("server: select");
            ret_val = -1;
            break;
        }

        if (ret_val == -1)
        {
            stop = 1;
        }
    } // end while

    // Cleanup after loop
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    free_heartbeat_data(heartbeat_data);
    free_clients_tcp_data(clients, MAX_CLIENTS);
    free_clients_http_data(http_clients, MAX_CLIENTS);
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
    pthread_mutex_lock(&lock); // Lock before malloc
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    pthread_mutex_unlock(&lock); // Unlock after malloc
    if (thread_result == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    printf("Thread cliente (%s:%d): lectura comienzo\n", client_data->client_ipstr, client_data->client_port);

    simulate_work();

    // receive initial message from client
    recv_val = recv_simple_packet(client_data->client_sockfd, &client_data->packet);
    if (recv_val == 0)
    {
        fprintf(stderr, "server: conexión finalizada antes de recibir packet\n");
        free_simple_packet(client_data->packet);
        client_data->packet = NULL;
        thread_result->value = THREAD_RESULT_CLOSED;
        pthread_exit((void *)thread_result);
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "server: error al recibir packet\n");
        free_simple_packet(client_data->packet);
        client_data->packet = NULL;
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
    pthread_mutex_lock(&lock); // Lock before malloc
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    pthread_mutex_unlock(&lock); // Unlock after malloc
    if (thread_result == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    printf("Thread cliente (%s:%d): escritura comienzo\n", client_data->client_ipstr, client_data->client_port);

    simulate_work();
    // send PONG message
    if (client_data->packet != NULL && strstr(client_data->packet->data, "PING") != NULL)
    {
        printf("Thread cliente (%s:%d): el mensaje contiene \"PING\"\n", client_data->client_ipstr, client_data->client_port);
        free_simple_packet(client_data->packet);
        client_data->packet = NULL;
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
            client_data->packet = NULL;
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        printf("Thread cliente (%s:%d): mensaje enviado: \"%s\"\n", client_data->client_ipstr, client_data->client_port, client_data->packet->data);
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
            client_data->packet = NULL;
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        printf("Thread cliente (%s:%d): mensaje enviado: \"%s\"\n", client_data->client_ipstr, client_data->client_port, client_data->packet->data);
    }

    // Print completion message
    printf("Thread cliente (%s:%d): escritura fin\n", client_data->client_ipstr, client_data->client_port);

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_heartbeat_read(void *arg)
{
    char client_ipstr[INET_ADDRSTRLEN];
    ssize_t recv_val;
    struct sockaddr_in *client_ipv4;
    struct in_addr *client_addr;
    Heartbeat_Data *heartbeat_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    heartbeat_data = (Heartbeat_Data *)arg;

    pthread_mutex_lock(&lock); // Lock before malloc
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    pthread_mutex_unlock(&lock); // Unlock after malloc
    if (thread_result == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    puts("Thread Heartbeat: lectura comienzo");

    simulate_work();

    if (heartbeat_data->packet != NULL)
    {
        free_heartbeat_packet(heartbeat_data->packet);
        heartbeat_data->packet = NULL;
    }
    // Create packet so that we can store the incoming packet
    if ((heartbeat_data->packet = create_heartbeat_packet("")) == NULL)
    {
        fprintf(stderr, "server: error al crear packet\n");
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }
    // receive HEARTBEAT message from client
    recv_val = recv_heartbeat_packet(heartbeat_data->sockfd, heartbeat_data->packet, heartbeat_data->addr, &heartbeat_data->addrlen);
    if (recv_val == 0)
    {
        fprintf(stderr, "server: conexión finalizada antes de recibir packet\n");
        free_heartbeat_packet(heartbeat_data->packet);
        heartbeat_data->packet = NULL;
        thread_result->value = THREAD_RESULT_CLOSED;
        pthread_exit((void *)thread_result);
    }
    else if (recv_val < 0)
    {
        fprintf(stderr, "server: error al recibir packet\n");
        free_heartbeat_packet(heartbeat_data->packet);
        heartbeat_data->packet = NULL;
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }

    client_ipv4 = (struct sockaddr_in *)heartbeat_data->addr;
    client_addr = &(client_ipv4->sin_addr);
    inet_ntop(client_ipv4->sin_family, client_addr, client_ipstr, sizeof(client_ipstr));

    printf("Thread Heartbeat: mensaje del cliente: %s:%d\n", client_ipstr, ntohs(client_ipv4->sin_port));
    printf("Thread Heartbeat: mensaje recibido: %s - %ld\n", heartbeat_data->packet->message, heartbeat_data->packet->timestamp);

    // Print completion message
    puts("Thread Heartbeat: lectura fin");

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_heartbeat_write(void *arg)
{
    char message[DEFAULT_BUFFER_SIZE];
    char client_ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in *client_ipv4;
    struct in_addr *client_addr;
    Heartbeat_Data *heartbeat_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    heartbeat_data = (Heartbeat_Data *)arg;
    pthread_mutex_lock(&lock); // Lock before malloc
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    pthread_mutex_unlock(&lock); // Unlock after malloc
    if (thread_result == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    if (heartbeat_data->packet == NULL)
    {
        // we can't send ACK if we haven't got a Heartbeat message first
        thread_result->value = THREAD_RESULT_EMPTY_PACKET;
        pthread_exit((void *)thread_result);
    }

    // If server already received a packet from the client and it wasn't a Heartbeat
    if (heartbeat_data->packet != NULL && strstr(heartbeat_data->packet->message, "HEARTBEAT") == NULL)
    {
        thread_result->value = THREAD_RESULT_SUCCESS;
        pthread_exit((void *)thread_result);
    }

    puts("Thread Heartbeat: escritura comienzo");

    simulate_work();

    // send ACK message
    puts("server: el mensaje contiene \"HEARTBEAT\"");
    free_heartbeat_packet(heartbeat_data->packet);
    heartbeat_data->packet = NULL;
    strcpy(message, "ACK");
    if ((heartbeat_data->packet = create_heartbeat_packet(message)) == NULL)
    {
        fprintf(stderr, "server: error al crear packet\n");
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }
    if (send_heartbeat_packet(heartbeat_data->sockfd, heartbeat_data->packet, heartbeat_data->addr, heartbeat_data->addrlen) < 0)
    {
        fprintf(stderr, "server: error al enviar packet\n");
        free_heartbeat_packet(heartbeat_data->packet);
        heartbeat_data->packet = NULL;
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }
    client_ipv4 = (struct sockaddr_in *)heartbeat_data->addr;
    client_addr = &(client_ipv4->sin_addr);
    inet_ntop(client_ipv4->sin_family, client_addr, client_ipstr, sizeof(client_ipstr));

    printf("Thread Heartbeat: mensaje al cliente: %s:%d\n", client_ipstr, ntohs(client_ipv4->sin_port));
    printf("Thread Heartbeat: mensaje enviado: %s - %ld\n", heartbeat_data->packet->message, heartbeat_data->packet->timestamp);

    free_heartbeat_packet(heartbeat_data->packet);
    heartbeat_data->packet = NULL;

    // Print completion message
    puts("Thread Heartbeat: escritura fin");

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_http_read(void *arg)
{
    Client_Http_Data *client_data;
    Thread_Result *thread_result;

    if (arg == NULL)
    {
        return NULL;
    }

    client_data = (Client_Http_Data *)arg;
    pthread_mutex_lock(&lock); // Lock before malloc
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    pthread_mutex_unlock(&lock); // Unlock after malloc
    if (thread_result == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    printf("Thread HTTP (%s:%d): lectura comienzo\n", client_data->client_ipstr, client_data->client_port);

    // Receive HTTP request
    client_data->request = receive_http_request(client_data->client_sockfd);
    if (client_data->request == NULL)
    {
        fprintf(stderr, "server: error al recibir HTTP request\n");
        thread_result->value = THREAD_RESULT_ERROR;
        pthread_exit((void *)thread_result);
    }

    printf("Thread HTTP (%s:%d): HTTP request recibido: %s %s %s\n",
           client_data->client_ipstr,
           client_data->client_port,
           client_data->request->request_line.method,
           client_data->request->request_line.uri,
           client_data->request->request_line.version);

    // Log headers
    printf("Thread HTTP (%s:%d): HTTP request headers:\n",
           client_data->client_ipstr,
           client_data->client_port);
    log_headers(client_data->request->headers, client_data->request->header_count);

    // Log body if present
    if (client_data->request->body)
    {
        printf("Thread HTTP (%s:%d): HTTP request body:\n",
               client_data->client_ipstr,
               client_data->client_port);
        printf("%s\n", client_data->request->body);
    }

    // Print completion message
    printf("Thread HTTP (%s:%d): lectura fin\n", client_data->client_ipstr, client_data->client_port);

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

void *handle_client_http_write(void *arg)
{
    char *full_path, *size_str;
    const char *content_type;
    int body_size, file_fd, header_index, header_count;
    ssize_t bytes_read, total_bytes_read;
    struct stat file_stat;
    struct dirent *entry;
    DIR *dp;
    Client_Http_Data *client_data;
    Thread_Result *thread_result;
    Header *headers;

    if (arg == NULL)
    {
        return NULL;
    }

    client_data = (Client_Http_Data *)arg;
    pthread_mutex_lock(&lock); // Lock before malloc
    thread_result = (Thread_Result *)malloc(sizeof(Thread_Result));
    pthread_mutex_unlock(&lock); // Unlock after malloc
    if (thread_result == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    if (client_data->request == NULL)
    {
        thread_result->value = THREAD_RESULT_EMPTY_REQUEST;
        pthread_exit((void *)thread_result);
    }

    printf("Thread HTTP (%s:%d): escritura comienzo\n", client_data->client_ipstr, client_data->client_port);
    header_index = 0;
    header_count = INITIAL_HEADER_COUNT;

    if (client_data->request->request_line.uri[0] == '/' && strlen(client_data->request->request_line.uri) > 1)
    {
        // Generate response for a particular file
        // Allocate memory for the file content
        full_path = (char *)malloc(sizeof(char) * (strlen(RESOURCES_FOLDER) + strlen(client_data->request->request_line.uri) + 1));
        if (full_path == NULL)
        {
            fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
            free_http_request(client_data->request);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        snprintf(full_path, strlen(RESOURCES_FOLDER) + strlen(client_data->request->request_line.uri) + 1, "%s%s", RESOURCES_FOLDER, client_data->request->request_line.uri);

        // strrchr: searches for the last occurrence of a character in a string
        content_type = get_content_type(strrchr(full_path, '.'));
        free(full_path);

        pthread_mutex_lock(&lock_file);
        file_fd = open(full_path, O_RDONLY);
        if (file_fd > 0 && fstat(file_fd, &file_stat) == 0)
        {
            // Generate response for existing file
            size_str = (char *)malloc(sizeof(char) * file_stat.st_size);
            if (size_str == NULL)
            {
                close(file_fd);
                fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
                free_http_request(client_data->request);
                thread_result->value = THREAD_RESULT_ERROR;
                pthread_exit((void *)thread_result);
            }
            headers = create_headers(header_count);
            add_header(&headers, &header_index, &header_count, "Content-Type", content_type);
            sprintf(size_str, "%ld", file_stat.st_size);
            add_header(&headers, &header_index, &header_count, "Content-Length", size_str);
            free(size_str);
            add_header(&headers, &header_index, &header_count, "Connection", "close");

            client_data->response = create_http_response(DEFAULT_HTTP_VERSION, 200, HTTP_200_PHRASE, headers, header_count, NULL);
            client_data->response->headers = headers;
            client_data->response->header_count = header_count;

            // Allocate memory for the file content
            client_data->response->body = (char *)malloc(sizeof(char) * file_stat.st_size);
            if (client_data->response->body == NULL)
            {
                close(file_fd);
                pthread_mutex_unlock(&lock_file);
                fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
                free_http_request(client_data->request);
                free_http_response(client_data->response);
                thread_result->value = THREAD_RESULT_ERROR;
                pthread_exit((void *)thread_result);
            }

            // Read the file content into response->body
            total_bytes_read = 0;
            while (total_bytes_read < file_stat.st_size)
            {
                bytes_read = read(file_fd, client_data->response->body + total_bytes_read, file_stat.st_size - total_bytes_read);
                if (bytes_read < 0)
                {
                    fprintf(stderr, "server: error al leer archivo: %s\n", strerror(errno));
                    close(file_fd);
                    pthread_mutex_unlock(&lock_file);
                    free_http_request(client_data->request);
                    free_http_response(client_data->response);
                    thread_result->value = THREAD_RESULT_ERROR;
                    pthread_exit((void *)thread_result);
                }
                total_bytes_read += bytes_read;
            }
            client_data->response->body[file_stat.st_size] = '\0'; // Null-terminate the body

            close(file_fd);
            pthread_mutex_unlock(&lock_file);

            if (send_http_response(client_data->client_sockfd, client_data->response) < 0)
            {
                fprintf(stderr, "server: error al enviar HTTP response\n");
                free_http_request(client_data->request);
                free_http_response(client_data->response);
                thread_result->value = THREAD_RESULT_ERROR;
                pthread_exit((void *)thread_result);
            }
            printf("Thread HTTP (%s:%d): response-line enviado: %s %d %s\n",
                   client_data->client_ipstr,
                   client_data->client_port,
                   client_data->response->response_line.version,
                   client_data->response->response_line.status_code,
                   client_data->response->response_line.reason_phrase);
            printf("Thread HTTP (%s:%d): headers enviados:\n",
                   client_data->client_ipstr,
                   client_data->client_port);
            log_headers(client_data->response->headers, client_data->response->header_count);
        }
        else if (file_fd < 0 || fstat(file_fd, &file_stat) != 0)
        {
            // File not found or error getting file stats
            // Generate response for file not found
            close(file_fd);
            pthread_mutex_unlock(&lock_file);
            client_data->response = create_http_response(DEFAULT_HTTP_VERSION, 404, HTTP_404_PHRASE, NULL, 0, NULL);
            if (send_http_response(client_data->client_sockfd, client_data->response) < 0)
            {
                fprintf(stderr, "server: error al enviar HTTP response\n");
                free_http_request(client_data->request);
                free_http_response(client_data->response);
                thread_result->value = THREAD_RESULT_ERROR;
                pthread_exit((void *)thread_result);
            }
            else
            {
                printf("Thread HTTP (%s:%d): response-line enviado: %s %d %s\n",
                       client_data->client_ipstr,
                       client_data->client_port,
                       client_data->response->response_line.version,
                       client_data->response->response_line.status_code,
                       client_data->response->response_line.reason_phrase);
            }
        }
    }
    else if (client_data->request->request_line.uri[0] == '/' && strlen(client_data->request->request_line.uri) == 1)
    {
        // Generate response that lists available files
        pthread_mutex_lock(&lock_file);
        dp = opendir(RESOURCES_FOLDER);
        if (dp == NULL)
        {
            fprintf(stderr, "server: error al abrir carpeta de recursos: %s\n", strerror(errno));
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }

        client_data->response = create_http_response(DEFAULT_HTTP_VERSION, 200, HTTP_200_PHRASE, NULL, 0, NULL);

        body_size = 0;
        // First pass: Calculate total length
        while ((entry = readdir(dp)))
        {
            if (entry->d_type == DT_REG)
            {
                body_size += strlen(entry->d_name) + 1; // +1 for '\n'
            }
        }

        // Allocate memory
        client_data->response->body = (char *)malloc(sizeof(char) * body_size + 1); // +1 for '\0'
        if (client_data->response->body == NULL)
        {
            closedir(dp);
            pthread_mutex_unlock(&lock_file);
            fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
            free_http_request(client_data->request);
            free_http_response(client_data->response);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }

        // Initialize the body with an empty string
        client_data->response->body[0] = '\0';

        // Rewind directory stream
        rewinddir(dp);

        // Second pass: Concatenate file names
        while ((entry = readdir(dp)))
        {
            if (entry->d_type == DT_REG)
            {
                strcat(client_data->response->body, entry->d_name);
                strcat(client_data->response->body, "\n");
            }
        }
        closedir(dp);
        pthread_mutex_unlock(&lock_file);

        size_str = (char *)malloc(sizeof(char) * strlen(client_data->response->body));
        if (size_str == NULL)
        {
            fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
            free_http_request(client_data->request);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }

        headers = create_headers(header_count);
        add_header(&headers, &header_index, &header_count, "Content-Type", "text/plain");
        sprintf(size_str, "%ld", strlen(client_data->response->body));
        add_header(&headers, &header_index, &header_count, "Content-Length", size_str);
        free(size_str);
        client_data->response->headers = headers;
        client_data->response->header_count = header_count;

        if (send_http_response(client_data->client_sockfd, client_data->response) < 0)
        {
            fprintf(stderr, "server: error al enviar HTTP response\n");
            free_http_request(client_data->request);
            free_http_response(client_data->response);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        printf("Thread HTTP (%s:%d): response-line enviado: %s %d %s\n",
               client_data->client_ipstr,
               client_data->client_port,
               client_data->response->response_line.version,
               client_data->response->response_line.status_code,
               client_data->response->response_line.reason_phrase);
        printf("Thread HTTP (%s:%d): headers enviados:\n",
               client_data->client_ipstr,
               client_data->client_port);
        log_headers(client_data->response->headers, client_data->response->header_count);
    }
    else
    {
        // Resource error
        // Generate response for resource error
        client_data->response = create_http_response(DEFAULT_HTTP_VERSION, 400, HTTP_400_PHRASE, NULL, 0, NULL);
        if (send_http_response(client_data->client_sockfd, client_data->response) < 0)
        {
            fprintf(stderr, "server: error al enviar HTTP response\n");
            free_http_request(client_data->request);
            free_http_response(client_data->response);
            thread_result->value = THREAD_RESULT_ERROR;
            pthread_exit((void *)thread_result);
        }
        else
        {
            printf("Thread HTTP (%s:%d): response-line enviado: %s %d %s\n",
                   client_data->client_ipstr,
                   client_data->client_port,
                   client_data->response->response_line.version,
                   client_data->response->response_line.status_code,
                   client_data->response->response_line.reason_phrase);
        }
    }

    // // Cleanup
    free_http_request(client_data->request);
    free_http_response(client_data->response);

    // Print completion message
    printf("Thread HTTP (%s:%d): escritura fin\n", client_data->client_ipstr, client_data->client_port);

    thread_result->value = THREAD_RESULT_SUCCESS;
    pthread_exit((void *)thread_result);
}

Client_Tcp_Data *create_client_tcp_data(int sockfd, const char *ipstr, in_port_t port)
{
    Client_Tcp_Data *data;
    if (sockfd <= 0 || ipstr == NULL || port <= 0)
    {
        return NULL;
    }

    data = (Client_Tcp_Data *)malloc(sizeof(Client_Tcp_Data));
    if (data == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(data, 0, sizeof(Client_Tcp_Data));

    data->client_sockfd = sockfd;
    strcpy(data->client_ipstr, ipstr);
    data->client_port = port;
    data->packet = NULL;

    return data;
}

Client_Tcp_Data **init_clients_tcp_data(int len)
{
    Client_Tcp_Data **clients;

    if (len <= 0)
    {
        fprintf(stderr, "Largo inválido: %d\n", len);
        return NULL;
    }

    clients = (Client_Tcp_Data **)malloc(len * sizeof(Client_Tcp_Data));
    if (clients == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(clients, 0, len * sizeof(Client_Tcp_Data));
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
    if (client != NULL && *client != NULL)
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

Client_Http_Data *create_client_http_data(int sockfd, const char *ipstr, in_port_t port)
{
    Client_Http_Data *data;
    if (sockfd <= 0 || ipstr == NULL || port <= 0)
    {
        return NULL;
    }

    data = (Client_Http_Data *)malloc(sizeof(Client_Http_Data));
    if (data == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(data, 0, sizeof(Client_Http_Data));

    data->client_sockfd = sockfd;
    strcpy(data->client_ipstr, ipstr);
    data->client_port = port;
    data->request = NULL;
    data->response = NULL;

    return data;
}

Client_Http_Data **init_clients_http_data(int len)
{
    Client_Http_Data **clients;

    if (len <= 0)
    {
        fprintf(stderr, "server: largo inválido: %d\n", len);
        return NULL;
    }

    clients = (Client_Http_Data **)malloc(len * sizeof(Client_Http_Data));
    if (clients == NULL)
    {
        fprintf(stderr, "server: error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(clients, 0, len * sizeof(Client_Http_Data));
    return clients;
}

void free_clients_http_data(Client_Http_Data **clients, int len)
{
    for (int i = 0; i < len; i++)
    {
        free_client_http_data(&clients[i]);
    }
}

void free_client_http_data(Client_Http_Data **client)
{
    if (client != NULL && *client != NULL)
    {
        if ((*client)->client_sockfd > 0)
        {
            close((*client)->client_sockfd);
        }
        if ((*client)->request != NULL)
        {
            free_http_request((*client)->request);
        }
        if ((*client)->response != NULL)
        {
            free_http_response((*client)->response);
        }
        free(*client);
        *client = NULL;
    }
}

int index_in_client_http_data_array(Client_Http_Data **array, int array_size, int index)
{
    if (index < 0 || index >= array_size || array[index] == NULL)
    {
        return 0; // Index is out of bounds or no value at index
    }
    return 1; // Index is within bounds and contains a value
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