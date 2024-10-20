#ifndef SERVER_H
#define SERVER_H

// Socket libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// Constants
#define BACKLOG 10 // how many pending connections queue will hold
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "3491" // the port users will be connecting to
#define PORTSTRLEN 6        // Enough to hold "65535" + '\0'
#define VERSION "0.0.1"

// Function prototypes
void handle_connections(int sockfd);
char *initialize_string(size_t size);
void parse_arguments(int argc, char *argv[], char **port_number, char **ip_number);
int setup_server(char *port_number, char *ip_number);
void setup_signal_handler(void);
void show_help(void);
void show_version(void);
void sigchld_handler(int s);

#endif // SERVER_H