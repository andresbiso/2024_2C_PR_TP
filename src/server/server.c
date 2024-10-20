// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Socket libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Other system libraries
#include <sys/wait.h>
#include <signal.h>

// Project header
#include "server.h"

int main(int argc, char *argv[])
{
    char *ip_number, *port_number;
    int sockfd; // listen on sock_fd

    if ((ip_number = initialize_string(INET_ADDRSTRLEN)) == NULL)
    {
        perror("Failed to allocate memory");
        return EXIT_FAILURE;
    }
    if ((port_number = initialize_string(PORTSTRLEN)) == NULL)
    {
        perror("Failed to allocate memory");
        return EXIT_FAILURE;
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

void handle_connections(int sockfd)
{
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    char their_ipstr[INET_ADDRSTRLEN];
    char *msg;
    int msglen, new_fd; // new connection on new_fd

    if ((msg = initialize_string(INET_ADDRSTRLEN)) == NULL)
    {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strcpy(msg, "Hola, soy el server");
    msglen = strlen(msg);

    // main accept() loop
    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, &their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.sa_family,
                  &(((struct sockaddr_in *)&their_addr)->sin_addr),
                  their_ipstr,
                  sizeof their_ipstr);
        printf("server: obtuvo conexión de %s\n", their_ipstr);

        if (fork() == 0)
        {
            // this is the child process
            // child doesn't need the listener
            close(sockfd);
            if (send(new_fd, msg, msglen, 0) == -1)
                perror("send");
            close(new_fd);
            exit(EXIT_SUCCESS);
        }
        // parent doesn't need this
        close(new_fd);
    }

    free(msg);
}

char *initialize_string(size_t size)
{
    char *p = (char *)malloc(size * sizeof(char));
    if (p == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }
    memset(p, 0, size);
    return p;
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
                printf("Error: Opción o argumento no soportado: %s\n", argv[i]);
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
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((returned_value = getaddrinfo(ip_number, port_number, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(returned_value));
        return 1;
    }

    puts("Direcciones IP para el \"Server\"");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        my_addr = &(ipv4->sin_addr);

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof my_ipstr);
        printf("->  IPv4: %s\n", my_ipstr);
    }

    // loop through all the results and bind to the first we can
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
            perror("setsockopt");
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

    // free addrinfo struct allocated memory
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

// Setup signal handler for reaping zombie processes that appear as the fork()ed child processes exit.
// This code ensures that terminated child processes are cleaned up immediately,
// preventing resource leaks and keeping the process table uncluttered.
void setup_signal_handler()
{
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
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

// Function to reap all dead processes
void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}