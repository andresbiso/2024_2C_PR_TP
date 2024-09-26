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


typedef struct sockaddr_in ADDR, *PADDR;



int main(int argc, char **argv) {

    
    // create file descriptor

    int fd = socket(AF_INET, SOCK_STREAM, 0);


    // io nonblocking
    
    // fcntl(fd, F_SETFL, O_NONBLOCK);


    // socket opts
    int yes=1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
      

    // bind socket sockadd_in
    PADDR serverAddr = (PADDR) malloc(sizeof(ADDR));
   
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons(3030); // ver
    serverAddr->sin_addr.s_addr = htonl(INADDR_ANY); //sudo apt install net-tools
   


    bind(fd, (struct sockaddr *) serverAddr, sizeof(ADDR));


    // set backlog
    listen(fd, 10);


    printf("listening...\n");



    PADDR clientAddr = (PADDR) malloc(sizeof(ADDR));
    socklen_t size = sizeof(ADDR);

    int clientSocket = accept(fd, (struct sockaddr *) clientAddr, &size);

    // The inet_ntoa() function converts the Internet host address in, given in network byte order, 
    // to a string in IPv4 dotted-decimal notation. 
        
    unsigned short usport = ntohs(clientAddr->sin_port);
    unsigned short port = clientAddr->sin_port;
    
    // ejemplo:
    // usport == 33982 to hex (84BE)
    // port == 48772 to hex (BE84)



    printf("accepted connection - socket: %d - ip: %s - port ho: %d - port no: %d\n", 
                    clientSocket, 
                    inet_ntoa(clientAddr->sin_addr),
                    ntohs(clientAddr->sin_port),
                    port);


    char *clientBuffer = (char *) malloc(1024);
    memset(clientBuffer, 0, 1024);
    
    int received = recv(clientSocket, clientBuffer, 10, 0);

    if(received > 0) {

        //unsigned short p = 0;

        //memcpy(&p, clientBuffer, 2);

        printf("data received: %s\n", clientBuffer);

    }



    return 0;
}