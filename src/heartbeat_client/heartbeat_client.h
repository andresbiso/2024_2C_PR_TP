#ifndef HEARTBEAT_CLIENT_H
#define HEARTBEAT_CLIENT_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Constants
#define DEFAULT_IP "127.0.0.1" // The ip heartbeat_client will be connecting to
#define DEFAULT_PORT "3490"    // The port heartbeat_client will be connecting to
#define PORTSTRLEN 6           // Enough to hold "65535" + '\0'
#define VERSION "0.0.1"

// Function prototypes
int handle_connection(int sockfd);
int parse_arguments(int argc, char *argv[], char *port_number, char *ip_number);
int setup_heartbeat_client(char *port_number, char *ip_number);
void show_help(void);
void show_version(void);
int set_heartbeat_timeout(int sockfd, time_t timeout_sec);

#endif // HEARTBEAT_CLIENT_H