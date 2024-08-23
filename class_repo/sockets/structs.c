
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



typedef struct alignmentTest {

    uint8_t flag; // 1
    uint32_t value; // 4
    uint8_t status; // 1

} ALIGNEMENT_STR; // __attribute__((packed, aligned(4)));

// aligned(4)

// x(xxx),xxxx-xxxx,x(xxx)


int main(int argc, char **argv) {

    // create tcp socket
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);


    // inet addr 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(3000);

    ALIGNEMENT_STR str;

    printf("sizeof uint8: %lu\n", sizeof(uint8_t));
    printf("sizeof uint32: %lu\n", sizeof(uint32_t));
    printf("sizeof double: %lu\n", sizeof(double));
    printf("sizeof: %lu\n", sizeof(str));

    return 0;



    printf("Connecting...\n");

    int ret = connect(socketFd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if(ret == -1) {

        printf("Connect error. Cause: %s\n", strerror(errno));

        exit(1);

    }





    

    
}

