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

#define MAX_CLIENTS 1024

#define THREAD_RESULT_ERROR -1
#define THREAD_RESULT_SUCCESS 0
#define THREAD_RESULT_CLOSED 1

typedef struct
{
    int client_sockfd;
    char client_ipstr[INET_ADDRSTRLEN];
    in_port_t client_port;
    Simple_Packet *packet;
} Client_Data;

typedef struct
{
    int value; // THREAD_RESULT_* values
} Thread_Result;

// Function prototypes
void *handle_client_write(void *arg);
void *handle_client_read(void *arg);
int handle_connections(int sockfd);
int parse_arguments(int argc, char *argv[], char *port_number, char *ip_number);
int setup_server(char *port_number, char *ip_number);
void show_help(void);
void show_version(void);
Client_Data *create_client_data(int sockfd, const char *ipstr, in_port_t port);
Client_Data **init_clients(int len);
void cleanup_clients(Client_Data **clients, int len);
void cleanup_client(Client_Data **client);
void setup_signals();
void handle_sigint(int sig);

#endif // SERVER_H