
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>


#define THREADS_COUNT 10

typedef struct connection_params {

    int iteration;

} CONNECTION_PARAMS, *PCONNECTION_PARAMS;





void *establishConnection(void *params) {

    struct sockaddr_in server_addr;
    int iteration = (int) params;
    
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(3001);


    int ret = connect(socketFd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if(ret == -1) {

        printf("Connect error %d. Cause: %s\n", iteration, strerror(errno));

    } else {

        printf("Connect success %d\n", iteration);

        send(socketFd, "hola", 4, 0);

    }

    return 0;

}



int main(int argc, char **argv) {

    pthread_t threads[THREADS_COUNT];

    for(int i = 0; i < THREADS_COUNT; i++) {
        

        pthread_create(&threads[i], NULL, &establishConnection, (void *) i);

    }

    // wait for all threads

    for(int i = 0; i < THREADS_COUNT; i++) {

        pthread_join(threads[i], NULL);

    }

    printf("main thread finished... \n");

    return 0;

}

