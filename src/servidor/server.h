//
//  server.h
//  Multithreading
//
//  Created by Diego Marafetti on 4/6/15.
//  Copyright (c) 2015 Diego Marafetti. All rights reserved.
//

#ifndef Multithreading_server_h
#define Multithreading_server_h

#include "main.h"


// foward declaration. Fix circular referece
struct __cx;
struct __server_opts;

typedef struct __server_opts    SERVER_OPS, *PSERVER_OPS;
typedef struct __cx             CONNECTION, *PCONNECTION;



/**
 * Server
 *
 */
typedef VOID        (* ON_CONNECTION_CALLBACK)  (PCONNECTION cx);
typedef VOID        (* SERVER_OPS_CONNECT)      (PSERVER_OPS ctx, PCHAR ip, USHORT port);
typedef BOOLEAN     (* SERVER_OPS_ISCONNECTED)  (PSERVER_OPS ctx);
typedef VOID        (* SERVER_OPS_DISCONNECT)   (PSERVER_OPS ctx);
typedef VOID        (* SERVER_OPS_LISTEN)       (PSERVER_OPS ctx, PCHAR port, ON_CONNECTION_CALLBACK callback);


struct __server_opts {
    
    // state
    INT socket;
    
    // methods
    SERVER_OPS_CONNECT     connect;
    SERVER_OPS_ISCONNECTED isConnected;
    SERVER_OPS_DISCONNECT  disconnect;
    SERVER_OPS_LISTEN      listen;
};


/**
 * Connection
 *
 */

typedef ssize_t     (* CX_OPS_RECEIVE)  (PCONNECTION cx, PVOID buffer, size_t size);
typedef ssize_t     (* CX_OPS_SEND)     (PCONNECTION cx, PVOID buffer, size_t size);
typedef VOID        (* CX_OPS_CLOSE)    (PCONNECTION cx);

struct __cx {
    
    // state
    INT socket;
    
    // methods
    CX_OPS_RECEIVE receive;
    CX_OPS_SEND    send;
    CX_OPS_CLOSE   close;

};


typedef struct __CONNECTION_PARAMS {
    
    PCONNECTION            connection;
    ON_CONNECTION_CALLBACK callback;
    
    
} CONNECTION_PARAMS, *PCONNECTION_PARAMS;






/**
 * Initialization
 *
 */

PCONNECTION_PARAMS newConnectionParams(PCONNECTION connection, ON_CONNECTION_CALLBACK callback);

PSERVER_OPS newServerOpts();

PCONNECTION newConnectionOps(INT socket);

VOID freeConnectionOpts(PCONNECTION ops);

VOID freeConnectionParams(PCONNECTION_PARAMS params);

PCONNECTION newConnection(INT socket);



#endif
