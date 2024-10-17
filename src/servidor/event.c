//
//  event.c
//  Multithreading
//
//  Created by Diego Marafetti on 4/21/15.
//  Copyright (c) 2015 Diego Marafetti. All rights reserved.
//


#include "event.h"
#include "main.h"

#define RECV_BUFFER_SIZE 1024 * 1

/**
 * On socket accept callback
 *
 */
void onAccept(evutil_socket_t listenSocket, short eventType, void *params) {
    
    struct sockaddr_in  clientAddrInfo;
    socklen_t           clientAddrInfoLen = sizeof(clientAddrInfo);
    
    printf("onAccept event\n");
    
    
    int newCxSocket = accept(listenSocket, (struct sockaddr *) &clientAddrInfo, &clientAddrInfoLen);
    
    
    PCHAR buffer = malloc(RECV_BUFFER_SIZE);
    memset(buffer, 0, RECV_BUFFER_SIZE);
    
    ssize_t bytes = recv(newCxSocket, buffer, RECV_BUFFER_SIZE, 0);
    
    if(bytes < 0) {
        
        printf("recv error: %s \n", strerror(errno));
        goto end;
    }
    
    if(bytes == 0) {
        
        printf("Connection closed by peer.\n");
        goto end;
    }
    
    printf("%s\n", buffer);
    printf("Bytes received %lu - data: \n%s\n", bytes, buffer);
    free(buffer);
    
end:
    
    close(newCxSocket);
}


/**
 *
 * Start event loop
 *
 *
 */
void start(int listenSocket) {

    
    struct event_base *eventBase = event_base_new();
    struct event      *acceptEvent;
    
    // non blocking socket
    int flags = fcntl(listenSocket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(listenSocket, F_SETFL, flags);
    
    
    
    acceptEvent = event_new(eventBase, listenSocket, EV_READ | EV_PERSIST, onAccept, NULL);
    

    
    
    if(acceptEvent == NULL) {
    
        printf("Error creating event new\n");
        exit(EXIT_FAILURE);
    }
    
    
    event_add(acceptEvent, NULL);
    
    event_base_dispatch(eventBase);

    
    //
    // free resources
    //
    event_free(acceptEvent);
    event_base_free(eventBase);

}

