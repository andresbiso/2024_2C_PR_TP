#ifndef CLIENT_H
#define CLIENT_H

// Socket libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// Constants
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "3490" // the port client will be connecting to
#define MAXDATASIZE 100     // max number of bytes we can get at once
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

#endif // CLIENT_H