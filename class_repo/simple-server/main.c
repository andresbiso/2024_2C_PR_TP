//
//  main.c
//  simple-server
//
//  Created by Diego on 02/09/2020.
//  Copyright © 2020 Diego. All rights reserved.
//

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
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>


int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}




void *atenderCliente(void *params) {
    
    /* As soon as server gets a request from client, it prepares the date and time and
     * writes on the client socket through the descriptor returned by accept()
     */
    int clientSocket = *((int *) params);
    char buffer[2048];
    char response[1024];
    struct stat fileStats;
    
    // buscar imagen en filesystem
    // get image props
    int imagefd = open("./yosemite.jpg", O_RDONLY);
    fstat(imagefd, &fileStats);
    printf("File size : %ld\n", fileStats.st_size);
    
    
    // build response
    char *responseHeaders = "HTTP/1.1 200\r\nContent-type:image/jpeg\r\nContent-lenght: %ld\r\nConnection: Close\r\n\r\n";
    memset(response, 0, 1024);
    sprintf(response, responseHeaders, fileStats.st_size);
        

    // receive request
    printf("Connection established on socket %d\n", clientSocket);
    memset(buffer, 0, 2048);
    recv(clientSocket, &buffer, 2048, 0);

    
    // send response
    // headers first
    int sent = send(clientSocket, response, strlen(response), 0);
    
    
    // payload next
    off_t sbytes = 0;
    int ret = sendfile(clientSocket, imagefd, NULL, fileStats.st_size);
    
    if(ret < 0) {
        
        fprintf(stderr, strerror(errno));
        exit(1);
        
    }
    
    printf("sending file... %lld\n", ret);   
    close(clientSocket);
    close(imagefd);
    
    return 0;
    
}



void *atenderCliente2(void *params) {


    /* As soon as server gets a request from client, it prepares the date and time and
     * writes on the client socket through the descriptor returned by accept()
     */
    int clientSocket = *((int *) params);
    char buffer[2048];
    char response[1024];
    struct stat fileStats;
    
    // buscar imagen en filesystem
    // get image props
    int imagefd = open("./yosemite.jpg", O_RDONLY);
    fstat(imagefd, &fileStats);
    printf("File size : %ld\n", fileStats.st_size);
    
    
    // build response
    char *responseHeaders = "HTTP/1.1 200 OK\r\nContent-type:image/jpeg\r\nContent-lenght: %ld\r\nConnection: Close\r\n\r\n";
    memset(response, 0, 1024);
    sprintf(response, responseHeaders, fileStats.st_size);
        

    // receive request
    printf("Connection established on socket %d\n", clientSocket);
    memset(buffer, 0, 2048);
    recv(clientSocket, &buffer, 2048, 0);
    printf("request: %s\n", buffer);

    
    // send response
    // headers first
    int sent = send(clientSocket, response, strlen(response), 0);
    
    
    // payload next
    off_t total = fileStats.st_size;
    char sendBuffer[1024] = {'\0'};
    size_t blockSize = 1024;

    while(total > 0) {

        if(total < 1024) {

            blockSize = total;
        } 

        memset(&sendBuffer, 0, 1024);
        read(imagefd, &sendBuffer, blockSize);

        ssize_t sentBytes = send(clientSocket, &sendBuffer, blockSize, 0);
        total = total - sentBytes;
        printf("%ld\n", total);

        msleep(60);
    }
    
    close(clientSocket);
    close(imagefd);
    
    return 0;


}





int main(int argc, char *argv[]) {


    int listenfd = 0, clientSocket = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;

    char sendBuff[1025];
    

    /* creates an UN-named socket inside the kernel and returns
     * an integer known as socket descriptor
     * This function takes domain/family as its first argument.
     * For Internet family of IPv4 addresses we use AF_INET
     */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    

    /* Do not wait to listener socket to be released
     */
    int yes=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));


    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));
    
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //sudo apt install net-tools
    serv_addr.sin_port = htons(3030);
    

    /* The call to the function "bind()" assigns the details specified
     * in the structure serv_addr' to the socket created in the step above
     */
    bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    /* The call to the function "listen()" with second argument as 10 specifies
     * maximum number of client connections that server will queue for this listening
     * socket.
     */
    listen(listenfd, 10);
    
    printf("listening port 3030...\n");
    
    /* In the call to accept(), the server is put to sleep and when for an incoming
     * client request, the three way TCP handshake* is complete, the function accept()
     * wakes up and returns the socket descriptor representing the client socket.
     */
    socklen_t size = sizeof(client_addr);
    
    
    clientSocket =  accept(listenfd, (struct sockaddr*) &client_addr, &size);

    if(clientSocket < 0) {
        
        // controlo error
        
    }
    
    
    // send & recv cliente (clientSocket)
    pthread_t clienteThread;
    pthread_attr_t threadAttrs;

    pthread_attr_init(&threadAttrs);
    pthread_attr_setdetachstate(&threadAttrs, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&threadAttrs, SCHED_FIFO);  

    //pthread_create(&clienteThread, &threadAttrs, atenderCliente, &clientSocket);

    atenderCliente2(&clientSocket);
}



