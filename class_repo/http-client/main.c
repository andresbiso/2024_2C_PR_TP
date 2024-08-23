//
//  main.c
//  HttpClient
//
//  Created by Diego Marafetti on 5/17/15.
//  Copyright (c) 2015 diego. All rights reserved.
//

#include "main.h"
#define CHUNK_SIZE 1024

/**
 * Get arguments from command line
 *
 */
PAPP_ARGUMENTS getProgramArguments(char **arguments) {

    PAPP_ARGUMENTS pArguments = malloc(sizeof(APP_ARGUMENTS));
    pArguments->address = arguments[1];
    pArguments->port    = arguments[2];
    
    return pArguments;
}


/**
 * Display help
 *
 */
VOID displayUsage() {

    fprintf(stdout, "Usage: client [host] [port]\n");
    fprintf(stdout, "EXAMPLES:\n");
    fprintf(stdout, "%-20s\n", "client host 80");
}


/**
 * Entry point
 *
 */
int main(int argc, char * argv[]) {

    int ret;
    struct addrinfo     hints;                      // template
    struct addrinfo     *serverInfo = NULL;         // results
    struct addrinfo     *targetServerInfo = NULL;   // target
    PAPP_ARGUMENTS      arguments;                  // arguments
    
    // arguments
    if(argc < 3) {
    
        displayUsage();
        return 0;
        
    } else {
    
        arguments = getProgramArguments(argv);
    }

    
    // prepare template server info struct

    memset(&hints, 0, sizeof(hints));
    hints.ai_family	= AF_INET; // AF_UNSPEC
	hints.ai_socktype = SOCK_STREAM;
    // IPv4 / TCP
    
    
    arguments->address = "neverssl.com";  // yahoo multiple ips
    arguments->port = "80";
    
    //
    // get remote information
    //
    printf("Reaching %s...\n", arguments->address);
    
    
    
    // AF_INET (ipv4), AF_INET6
    // SOCKT_STREAM (TCP)
    
    
    ret = getaddrinfo(arguments->address, // yahoo.com
                      arguments->port,    // 80
                      &hints,
                      &serverInfo);
    
    
    
    if(ret != 0) {
    
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }

    
    //
    // iterate over all serverinfo results
    //
    for (struct addrinfo *currentAddr = serverInfo; currentAddr != NULL; currentAddr = currentAddr->ai_next) {
        
        char ipAddress[100];
        void *rawAddress = NULL;
        
        if(currentAddr->ai_family == AF_INET) {
        
            rawAddress = &((struct sockaddr_in *) currentAddr->ai_addr)->sin_addr;
            
            if(targetServerInfo == NULL) {
                
                targetServerInfo = currentAddr;
            }
            
        } else {
        
            rawAddress = &((struct sockaddr_in6 *) currentAddr->ai_addr)->sin6_addr;
        }
        
        printf("IP: %s\n", inet_ntop(currentAddr->ai_family,
                                       rawAddress,
                                       ipAddress,
                                       sizeof(ipAddress)));
    }
    
    
    //
    // connect to remote server
    //
    INT cxSocket = socket(targetServerInfo->ai_family,
                              targetServerInfo->ai_socktype,
                              targetServerInfo->ai_protocol);
    
  
    ret = connect(cxSocket, targetServerInfo->ai_addr, targetServerInfo->ai_addrlen);
    
    if(ret == -1) {
    
        close(cxSocket);
        perror("client: connect");
        exit(1);
    }
    
    
    //
    // prepare http request
    //
    PCHAR httpHeader    = "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
    ULONG httpRequestSize = strlen(httpHeader) + strlen(arguments->address);
    PCHAR httpRequest   = malloc(httpRequestSize);
    memset(httpRequest, 0, httpRequestSize);
    sprintf(httpRequest, httpHeader, arguments->address);
    
    
    //
    // send request
    //
    ssize_t rxBytes, cxBytes, rxTotal = 0;
    
    cxBytes = send(cxSocket, httpRequest, httpRequestSize, 0);
    printf("REQUEST\n%s\n%lu bytes sended.\n\n", httpRequest, cxBytes);
    
    
    
    //
    // receive response
    //
    PCHAR chunk     = NULL;
    
    // init with chunk size
    chunk = malloc(CHUNK_SIZE);
    memset(chunk, 0, CHUNK_SIZE);
    
    do {
    
        rxBytes = recv(cxSocket, chunk, CHUNK_SIZE, 0);
        
        if(rxBytes == -1) {
            
            printf("recv error: ");
            perror("client: connect");
            break;
        }
        
        if(rxBytes == 0) {
        
            printf("\nConnection closed by peer. \n");
            break;
        }
        
        rxTotal = rxTotal + rxBytes;
    
        printf("%s", chunk);
        printf("\n\n-------------------------\n");
        memset(chunk, 0, CHUNK_SIZE);
        
    } while(TRUE);
    

    printf("bytes: %lu\n", rxTotal);
    
   
    
    // dealloc memory and finish
    close(cxSocket);
    free(chunk);
    free(httpRequest);
    free(arguments);
    freeaddrinfo(serverInfo);
    return 0;
}

