#ifndef CLIENT_H
#define CLIENT_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Constants
#define EXTERNAL_IP "127.0.0.1"   // The ip client will be connecting to
#define EXTERNAL_PORT "3490"      // The port client will be connecting to
#define EXTERNAL_PORT_HTTP "3492" // The port client will be connecting to
#define PORTSTRLEN 6              // Enough to hold "65535" + '\0'
#define DEFAULT_MODE 0            // 0: test mode; 1: http mode
#define DEFAULT_RESOURCE "/server.png"
#define DEFAULT_HOST "localhost"
#define DEFAULT_HTTP_METHOD "GET"
#define DEFAULT_HTTP_VERSION "HTTP/1.1"
#define VERSION "0.0.1"

// Function prototypes
int handle_connection(int sockfd);
int handle_connection_http(int sockfd, const char *resource);
int parse_arguments(int argc, char *argv[], char *external_ip, char *external_port, int mode, char *resource);
int setup_client(char *external_ip, char *external_port);
void show_help(void);
void show_version(void);

#endif // CLIENT_H