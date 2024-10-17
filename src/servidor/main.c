//
//  main.c
//  Multithreading
//
//  Created by Diego Marafetti on 4/6/15.
//  Copyright (c) 2015 Diego Marafetti. All rights reserved.
//

#include "main.h"
#include "server.h"
#include "http.h"

#define RECV_BUFFER_SIZE 1024 * 1





//
// Handle connection. This function is executed in a separate
// thread.
//
//
VOID onClientConnected(PCONNECTION cx) {
 
    PCHAR buffer = malloc(RECV_BUFFER_SIZE);
    memset(buffer, 0, RECV_BUFFER_SIZE);
    
    ssize_t bytes = cx->receive(cx, buffer, RECV_BUFFER_SIZE);
    
    if(bytes < 0) {
        
        printf("recv error: %s \n", strerror(errno));
        goto end;
    }
    
    if(bytes == 0) {
        
        printf("Connection closed by peer.\n");
        goto end;
    }
    
    //PCHAR token = strtok(buffer, "\r\n");
    
    printf("%s\n", buffer);
    printf("Bytes received %lu - data: \n%s\n", bytes, buffer);
    free(buffer);
    
end:
    
    cx->close(cx);
    freeConnectionOpts(cx);
}



//
// Display help
//
//
VOID displayUsage() {
    
    fprintf(stdout, "Usage: server [port]\n");
}



//
// Entry point. Start Application
//
//
INT main(INT argc, const PCHAR *argv) {
    

    if(argc < 1) {
    
        displayUsage();
        
        return 1;
    }
    
    
    PSERVER_OPS server = newServerOpts();
    
    server->listen(server, argv[1], onClientConnected);

    return 0;
}

