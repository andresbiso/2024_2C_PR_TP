#ifndef SERVER_H
#define SERVER_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Constants
#define BACKLOG 10             // How many pending connections queue will hold
#define DEFAULT_IP "127.0.0.1" // The ip users will be connecting to
#define DEFAULT_PORT "3490"    // The port users will be connecting to
#define PORTSTRLEN 6           // Enough to hold "65535" + '\0'
#define VERSION "0.0.1"

typedef struct
{
    int client_sockfd;
    char client_ipstr[INET_ADDRSTRLEN];
    in_port_t client_port;
} Client_Data;

// Function prototypes
void *handle_client(void *arg);
int handle_connections(int sockfd);
int parse_arguments(int argc, char *argv[], char *port_number, char *ip_number);
int setup_server(char *port_number, char *ip_number);
void show_help(void);
void show_version(void);
Client_Data *create_client_data(int sockfd, const char *ipstr, in_port_t port);

#endif // SERVER_H