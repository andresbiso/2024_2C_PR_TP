//
//  main.c
//  Socket-selects
//
//  Created by Diego Marafetti on 9/19/16.
//  Copyright © 2016 diego. All rights reserved.
//

#include "main.h"

int main(int argc, const char * argv[]) {
    
    
    char buffer[256];
    
    struct addrinfo     hints;
    struct addrinfo     *serverInfo;

    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family 	= AF_INET;
    hints.ai_socktype 	= SOCK_STREAM;
    hints.ai_flags 		= AI_PASSIVE;
    
    getaddrinfo(NULL, "8081", &hints, &serverInfo);
    
    
    if(serverInfo == NULL) {
        
        printf("Local ip address coudn't be obtained.\n");
        exit(EXIT_FAILURE);
    }
    
    
    int socketListen = socket(serverInfo->ai_family,
                              serverInfo->ai_socktype,
                              serverInfo->ai_protocol);
    
    
    int yes=1;
    setsockopt(socketListen,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    fcntl(socketListen, F_SETFL, O_NONBLOCK);
    
    // get socket options
    unsigned int bufferSize = 0;
    unsigned int bufferSizeLenght = sizeof(bufferSize);
    
    getsockopt(socketListen, SOL_SOCKET, SO_RCVBUF, &bufferSize, &bufferSizeLenght);
    printf("recv buffer size: %u\n", bufferSize);
    
    getsockopt(socketListen, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferSizeLenght);
    printf("send buffer size: %u\n", bufferSize);
    
    getsockopt(socketListen, SOL_SOCKET, SO_REUSEADDR, &bufferSize, &bufferSizeLenght);
    printf("reuseaddr: %u\n", bufferSize);
    
    
    bind(socketListen, serverInfo->ai_addr, serverInfo->ai_addrlen);
    listen(socketListen, 2);
    
    
    struct timeval tv;
    

    // A file descriptor is considered ready for reading if a read call will not block.
    // This usually includes the read offset being at the end of the file or there is
    // an error to report. A server socket is considered ready for reading if there is
    // a pending connection which can be accepted with accept
    fd_set readfds;
    fd_set tempreadfds;
    
    
    // A client socket is ready for writing when its connection is fully established
    fd_set writefds;
    
    
    // "Exceptional conditions” does not mean errors—errors are reported immediately when
    // an erroneous system call is executed, and do not constitute a state of the descriptor.
    // Rather, they include conditions such as the presence of an urgent message on a socket.
    // See Sockets, for information on urgent messages.
    fd_set exceptionfds;
    
  
    int fdmax = socketListen;

    
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(socketListen, &readfds);
    
    
    while(TRUE) {
    
        tempreadfds = readfds;

        // specifies the minimum interval that select() should block
        // waiting for a file descriptor to become ready.
        // If both fields of the timeval structure are zero, then select() returns immediately.
        // If timeout is NULL (no timeout), select() can block indefinitely.
        tv.tv_sec = 2;
        tv.tv_usec = 500000;
        
        int ret = select(fdmax + 1, &tempreadfds, NULL, NULL, &tv);
        
        printf("select return: %d.\n", ret);

        if(ret == -1) {
        
            perror("select");
        }
        
        
        for(int fd = 0; fd <= fdmax; fd++) {
            
            
            if(FD_ISSET(fd, &tempreadfds)) {
                
                printf("event on descriptor: %d OK\n", fd);
                
                if(fd == 0) {
                    
                    // The console is just sending the input to thee program and you program is not reading it.
                    // The input is in a buffer until it is read by your code
                    memset(&buffer, 0, 256);
                    ssize_t readed = read(0, &buffer, 256);
                    printf("console %ld bytes readed: %s\n", readed, buffer);
                    continue;
                    
                }
                
                if(fd == socketListen) {
                    
                    struct sockaddr sockaddClient;
                    socklen_t sockaddClientLength;
                    
                    int socketNewCx = accept(socketListen, &sockaddClient, &sockaddClientLength);
                    
                    FD_SET(socketNewCx, &readfds);
                    
                    if(socketNewCx > fdmax) {
                    
                        fdmax = socketNewCx;
                    }
                    
                    
                } else {
                    
                    memset(&buffer, 0, 256);
                    ssize_t readed = recv(fd, &buffer, 256, 0);
                        
                    // 5 seg
                    
                    if(readed < 1) {
                        
                        FD_CLR(fd, &readfds);
                        
                        // actualizar el fdmax
                        
                        continue;
                        
                    } else {
                        
                        printf("socket %ld bytes readed: %s\n", readed, buffer);
                    }
    
                }
            
            } else {
            
                printf("%d, ", fd);
            }
            
        }
        
        printf("\n");
        
    }
    
    return 0;
}
