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
#include "client.h"

int main(int argc, char *argv[])
{
    char *ip_number, *port_number;
    int sockfd; // listen on sock_fd

    if ((ip_number = initialize_string(INET_ADDRSTRLEN)) == NULL)
    {
        perror("client: failed to allocate memory");
        return EXIT_FAILURE;
    }
    if ((port_number = initialize_string(PORTSTRLEN)) == NULL)
    {
        perror("client: failed to allocate memory");
        return EXIT_FAILURE;
    }
    strcpy(ip_number, DEFAULT_IP);
    strcpy(port_number, DEFAULT_PORT);

    parse_arguments(argc, argv, &port_number, &ip_number);
    sockfd = setup_client(port_number, ip_number);
    handle_connection(sockfd);

    // free allocated memory
    free(ip_number);
    free(port_number);

    return EXIT_SUCCESS;
}

void handle_connection(int sockfd)
{
    char *buf;
    char *msg;
    int msglen, numbytes;

    if ((msg = initialize_string(INET_ADDRSTRLEN)) == NULL)
    {
        perror("client: failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    if ((buf = initialize_string(MAXDATASIZE)) == NULL)
    {
        perror("client: failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strcpy(msg, "PING");
    msglen = strlen(msg);

    // main loop
    while (1)
    {
        recvall(sockfd, buf, MAXDATASIZE - 1);
        printf("client: received \"%s\"\n", buf);
        close(sockfd);
        break;
    }

    free(msg);
    free(buf);
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
                printf("client: Opción o argumento no soportado: %s\n", argv[i]);
                show_help();
                exit(EXIT_FAILURE);
            }
        }
    }
}

int setup_client(char *port_number, char *ip_number)
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
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(returned_value));
        return 1;
    }

    puts("client: direcciones resueltas");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        my_addr = &(ipv4->sin_addr);

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof my_ipstr);
        printf("->  IPv4: %s\n", my_ipstr);
    }

    // loop through all the results and connect to the first we can
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

    // free addrinfo struct allocated memory
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "client: no pudo realizarse el connect\n");
        exit(EXIT_FAILURE);
    }

    // Show connect ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    my_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof my_ipstr);

    printf("client: conectado a %s:%s\n", my_ipstr, port_number);

    return sockfd;
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