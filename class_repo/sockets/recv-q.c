
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




int main(int argc, char **argv) {

    int socketFd;
    
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;


    
    socketFd = socket(AF_INET, SOCK_STREAM, 0);

    printf("socket listen: %d\n", socketFd);
   


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("2.23.178.8");
    server_addr.sin_port = htons(3000);
   

    // server
    int yes = 1;
    setsockopt(socketFd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

    bind(socketFd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    listen(socketFd, 2);



    socklen_t address_len = sizeof(client_addr);

    int socketClient = accept(socketFd, (struct sockaddr *) &client_addr, &address_len);

 


   // buffer TCP - size
    // get socket options
    unsigned int bufferSize = 0;
    unsigned int bufferSizeLenght = sizeof(bufferSize);

    getsockopt(socketClient, SOL_SOCKET, SO_RCVBUF, &bufferSize, &bufferSizeLenght);
    printf("recv buffer size: %u\n", bufferSize);
    
    getsockopt(socketClient, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferSizeLenght);
    printf("send buffer size: %u\n", bufferSize);



    // # watch -n0 ss -t4ane 
    // # netcat localhost 3000
    // comenzar a enviar data por netcat. 
    // observar Recv-Q en la conexión establecida como se llena

    int c = getc(stdin);

    printf("%d\n", c);


    // El proceso toma los dato del buffer TCP
    // Recv-Q queda en 0

    char respuesta[1024] = {'\0'};

    int bytesRecibidos = 0;

    bytesRecibidos = recv(socketClient, &respuesta, 1024, 0);

    printf("Mensaje cliente: (%d bytes) %s\n", bytesRecibidos, respuesta);


    c = getc(stdin);

    /*

    char *mensaje = "hola cliente!\n";

    send(socketClient, mensaje, 1, 0);


    char respuesta[1024] = {'\0'};

    int bytesRecibidos = 0;

    bytesRecibidos = recv(socketClient, &respuesta, 1024, 0);

    printf("Mensaje cliente: (%d bytes) %s\n", bytesRecibidos, respuesta);

    */
    
}

