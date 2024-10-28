#ifndef SERVER_H
#define SERVER_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Constants
#define BACKLOG 10              // How many pending connections queue will hold
#define DEFAULT_IP "127.0.0.1"  // The ip users will be connecting to
#define DEFAULT_PORT_TCP "3490" // The port users will be connecting for TCP
#define DEFAULT_PORT_UDP "3491" // The port users will be connecting for UDP
#define PORTSTRLEN 6            // Enough to hold "65535" + '\0'
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
} Client_Tcp_Data;

typedef struct
{
    int client_sockfd;
    char client_ipstr[INET_ADDRSTRLEN];
    in_port_t client_port;
    Udp_Packet *packet;
} Client_Udp_Data;

typedef struct
{
    int value; // THREAD_RESULT_* values
} Thread_Result;

// Function prototypes
void *handle_client_tcp_write(void *arg);
void *handle_client_tcp_read(void *arg);
void *handle_client_udp_write(void *arg);
void *handle_client_udp_read(void *arg);
int handle_connections(int sockfd_tcp, int sockfd_udp);
int parse_arguments(int argc, char *argv[], char *ip_number, char *port_number_tcp, char *port_number_udp);
int setup_server_tcp(char *ip_number, char *port_number);
int setup_server_udp(char *ip_number, char *port_number);
void show_help(void);
void show_version(void);
int create_client_tcp_data(Client_Tcp_Data *data, int sockfd, const char *ipstr, in_port_t port);
Client_Tcp_Data **init_clients_tcp_data(int len);
void cleanup_clients_tcp_data(Client_Tcp_Data **clients, int len);
void cleanup_client_tcp_data(Client_Tcp_Data **client);
int create_client_udp_data(Client_Udp_Data *data, int sockfd, const char *ipstr, in_port_t port);
Client_Udp_Data *init_client_udp_data(int len);
void cleanup_client_udp_data(Client_Udp_Data *client);
void setup_signals();
void handle_sigint(int sig);

#endif // SERVER_H