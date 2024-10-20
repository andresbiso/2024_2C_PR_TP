#ifndef CLIENT_H
#define CLIENT_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Constants
#define DEFAULT_IP "127.0.0.1" // the ip client will be connecting to
#define DEFAULT_PORT "3490"    // the port client will be connecting to
#define MAXDATASIZE 100        // max number of bytes we can get at once
#define PORTSTRLEN 6           // Enough to hold "65535" + '\0'
#define VERSION "0.0.1"

// Function prototypes
void handle_connection(int sockfd);
char *initialize_string(size_t size);
void parse_arguments(int argc, char *argv[], char **port_number, char **ip_number);
int setup_client(char *port_number, char *ip_number);
void show_help(void);
void show_version(void);

#endif // CLIENT_H