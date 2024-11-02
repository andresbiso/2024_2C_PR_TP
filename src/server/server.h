#ifndef SERVER_H
#define SERVER_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Shared headers
#include "../shared/common.h"

// Constants
#define BACKLOG 10                 // How many pending connections queue will hold
#define LOCAL_IP "127.0.0.1"       // The ip users will be connecting to
#define LOCAL_PORT_TCP "3490"      // The port users will be connecting for TCP
#define LOCAL_PORT_UDP "3491"      // The port users will be connecting for UDP
#define LOCAL_PORT_TCP_HTTP "3492" // The port users will be connecting for TCP HTTP
#define PORTSTRLEN 6               // Enough to hold "65535" + '\0'
#define VERSION "0.0.1"

#define MAX_CLIENTS 1024

#define THREAD_RESULT_EMPTY_PACKET -2
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
    int value; // THREAD_RESULT_* values
} Thread_Result;

// Function prototypes
void *handle_client_simple_read(void *arg);
void *handle_client_simple_write(void *arg);
void *handle_client_heartbeat_read(void *arg);
void *handle_client_heartbeat_write(void *arg);
int handle_connections(int sockfd_tcp, int sockfd_udp, int sockfd_tcp_http);
int parse_arguments(int argc, char *argv[], char *local_ip, char *local_port_tcp, char *local_port_udp, char *local_port_tcp_http);
int setup_server_tcp(char *local_ip, char *local_port);
int setup_server_udp(char *local_ip, char *local_port);
void show_help(void);
void show_version(void);
Client_Tcp_Data *create_client_tcp_data(int sockfd, const char *ipstr, in_port_t port);
Client_Tcp_Data **init_clients_tcp_data(int len);
void free_clients_tcp_data(Client_Tcp_Data **clients, int len);
void free_client_tcp_data(Client_Tcp_Data **client);
void setup_signals();
void handle_sigint(int sig);

#endif // SERVER_H