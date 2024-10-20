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

// Function prototypes
void handle_connections(int sockfd);
void parse_arguments(int argc, char *argv[], char **port_number, char **ip_number);
int setup_server(char *port_number, char *ip_number);
void show_help(void);
void show_version(void);

#endif // SERVER_H