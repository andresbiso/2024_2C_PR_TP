#ifndef HEARTBEAT_CLIENT_H
#define HEARTBEAT_CLIENT_H

// Networking headers
#include <sys/socket.h>
#include <netdb.h>

// System headers
#include <sys/types.h>

// Shared headers
#include "../shared/common.h"

// Constants
#define EXTERNAL_IP "127.0.0.1"
#define EXTERNAL_PORT "3491"
#define LOCAL_IP "127.0.0.1"
#define LOCAL_PORT "4000"
#define PORTSTRLEN 6 // Enough to hold "65535" + '\0'
#define VERSION "0.0.1"

// Function prototypes
int handle_connection(Heartbeat_Data *heartbeat_data);
int parse_arguments(int argc, char *argv[], char *local_ip, char *local_port, char *external_ip, char *external_port);
Heartbeat_Data *setup_heartbeat_client(char *local_ip, char *local_port, char *external_ip, char *external_port);
void show_help(void);
void show_version(void);
int set_heartbeat_timeout(int sockfd, time_t timeout_sec);

#endif // HEARTBEAT_CLIENT_H