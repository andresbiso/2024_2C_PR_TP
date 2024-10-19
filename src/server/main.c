#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define DEFAULT_PORT "3491" // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold

#define VERSION "0.0.1"

#define DEFAULT_IP "127.0.0.1"

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

void show_help()
{
    puts("Uso: server [opciones] [argumentos]");
    puts("Opciones:");
    puts("  --help      Muestra este mensaje de ayuda");
    puts("  --version   Muestra version del programa");
    puts("Argumentos:");
    puts("  --port <puerto> Especificar el número de puerto");
    puts("  --ip <ip> Especificar el número de ip");
}

void show_version()
{
    printf("Server Version %s\n", VERSION);
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    int returned_value;
    char *msg = "¡Hola mundo!";
    int msglen = strlen(msg);
    char *port_number = DEFAULT_PORT;
    char *ip_number = DEFAULT_IP;
    void *my_addr;
    char my_ipstr[INET_ADDRSTRLEN];
    char their_ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in *ipv4;

    if (argc >= 2)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "--help") == 0)
            {
                show_help();
                return 0;
            }
            else if (strcmp(argv[i], "--version") == 0)
            {
                show_version();
                return 0;
            }
            else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            {
                port_number = argv[i + 1];
                i++; // Skip the next argument since it's the port number
            }
            else if (strcmp(argv[i], "--ip") == 0 && i + 1 < argc)
            {
                ip_number = argv[i + 1];
                i++; // Skip the next argument since it's the port number
            }
            else
            {
                printf("Error: Opción o argumento no soportado: %s\n", argv[i]);
                show_help();
                return 1;
            }
        }
    }

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
        fprintf(stderr, "server: no pude realizarse el bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Setup signal handler for reaping dead processes
    // This code ensures that terminated child processes are cleaned up immediately,
    // preventing resource leaks and keeping the process table uncluttered.
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Show listening ip and port
    ipv4 = (struct sockaddr_in *)p->ai_addr;
    my_addr = &(ipv4->sin_addr);
    inet_ntop(p->ai_family, my_addr, my_ipstr, sizeof my_ipstr);

    printf("server: %s:%s\n", my_ipstr, port_number);
    puts("server: esperando conexiones...");

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

    return EXIT_SUCCESS;
}
