//
//  server.c
//  Multithreading
//
//  Created by Diego Marafetti on 4/6/15.
//  Copyright (c) 2015 Diego Marafetti. All rights reserved.
//

#include "server.h"
#include "event.h"




//
// Server ops
//
//

PVOID handleRequest(PVOID params) {
    
    PCONNECTION_PARAMS cxParams = params;
    
    cxParams->callback(cxParams->connection);
    
    freeConnectionParams(cxParams);
    
    return 0;
}


VOID serverConnect(PSERVER_OPS ctx, PCHAR ip, USHORT port) {

    
}


BOOLEAN serverIsConnected(PSERVER_OPS ctx){
    
    return TRUE;
}



VOID serverDisconnect (PSERVER_OPS ctx) {

    
}


//
// Start server in listen mode
//
VOID serverListen (PSERVER_OPS ctx, PCHAR port, ON_CONNECTION_CALLBACK callback) {
    
    
    struct addrinfo     hints;
	struct addrinfo     *serverInfo;
    struct addrinfo     *serverAddrInfo = NULL;
    struct sockaddr_in  clientAddrInfo;
    socklen_t           clientAddrInfoLen = sizeof(clientAddrInfo);
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;
	hints.ai_flags 		= AI_PASSIVE;
    
	getaddrinfo(NULL, port, &hints, &serverInfo);
    
    
    // iterate over all serverinfo results
    for (struct addrinfo *currentAddr = serverInfo; currentAddr != NULL; currentAddr = currentAddr->ai_next) {
        
        if(currentAddr->ai_family == AF_INET) {
            
            serverAddrInfo = currentAddr;
        }
    }
    
    if(serverAddrInfo == NULL) {
        
        printf("Local ip address coudn't be obtained.\n");
        exit(EXIT_FAILURE);
    }
    
    
    ctx->socket = socket(serverAddrInfo->ai_family,
                         serverAddrInfo->ai_socktype,
                         serverAddrInfo->ai_protocol);
    
    if(ctx->socket < 0) {
        
        printf("socket error: %s \n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    bind(ctx->socket, serverAddrInfo->ai_addr, serverAddrInfo->ai_addrlen);
    
    
    // change to passive mode
    listen(ctx->socket, 20);
    
    
    //start(ctx->socket);
    
    
    while(TRUE) {
        
        
        INT newCxSocket = accept(ctx->socket, (struct sockaddr *) &clientAddrInfo, &clientAddrInfoLen);
        
        if(newCxSocket < 0) {
            
            printf("Accept error: %s \n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        
        // create new thread for request handling
        pthread_t      worker;
        pthread_attr_t workerAttrs;
        
        pthread_attr_init(&workerAttrs);
        pthread_attr_setdetachstate(&workerAttrs, PTHREAD_CREATE_DETACHED);
        pthread_attr_setschedpolicy(&workerAttrs, SCHED_FIFO);
        
        
        // create connextion context
        PCONNECTION cx = newConnectionOps(newCxSocket);
        
        // start worker
        pthread_create(&worker, &workerAttrs, handleRequest, newConnectionParams(cx, callback));
        
    }
    
    // free results
    freeaddrinfo(serverInfo);
}


PSERVER_OPS newServerOpts() {

    PSERVER_OPS ops     = malloc(sizeof(SERVER_OPS));
    ops->socket         = -1;
    ops->connect        = serverConnect;
    ops->isConnected    = serverIsConnected;
    ops->disconnect     = serverDisconnect;
    ops->listen         = serverListen;
    return ops;
}



//
// Connection ops
//
//

ssize_t cxReceive (PCONNECTION cx, PVOID buffer, size_t size) {
    
    return recv(cx->socket, buffer, size, 0);
}

ssize_t cxSend (PCONNECTION cx, PVOID buffer, size_t size) {

    return send(cx->socket, buffer, size, 0);
}

VOID cxClose (PCONNECTION cx) {

    close(cx->socket);
}

PCONNECTION newConnectionOps(INT socket){

    PCONNECTION ops = malloc(sizeof(CONNECTION));
    ops->socket     = socket;
    ops->receive    = cxReceive;
    ops->send       = cxSend;
    ops->close      = cxClose;
    
    return ops;
}

VOID freeConnectionOpts(PCONNECTION ops) {

    free(ops);
}

PCONNECTION_PARAMS newConnectionParams(PCONNECTION connection, ON_CONNECTION_CALLBACK callback) {

    PCONNECTION_PARAMS params = malloc(sizeof(CONNECTION_PARAMS));

    params->connection  = connection;
    params->callback    = callback;

    return params;
}

VOID freeConnectionParams(PCONNECTION_PARAMS params) {
    
    free(params);
}

