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


    // bind socket
    
    PADDR serverAddr = (PADDR) malloc(sizeof(ADDR));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr->sin_port = htons(3030); // ver


    // try to connect

    printf("connecting...\n");

    int ret = connect(fd, (struct sockaddr *) serverAddr, sizeof(ADDR));

    if(ret == -1) {

        printf("error lseek: %s\n", strerror(errno));
        exit(1);
    }

    printf("connected!\n");



    // send data

    char *buffer = "123456789";
    //unsigned short number = 1000;

    int sent = send(fd, buffer, strlen(buffer), 0);
    //int sent = send(fd, &number, sizeof(unsigned short), 0);

    printf("bytes sent: %d\n", sent);



    return 0;
}