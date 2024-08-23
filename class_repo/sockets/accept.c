
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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //listen on every available network interface
    server_addr.sin_port = htons(3001);
   

    // server

    /* Do not wait to listener socket to be released
     */
    int yes = 1;
    setsockopt(socketFd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

    bind(socketFd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    listen(socketFd, 1);




    while(1) {

        printf("Press any key to accept connection\n");

        int c = getc(stdin);


        socklen_t address_len = sizeof(client_addr);

        int socketClient = accept(socketFd, (struct sockaddr *) &client_addr, &address_len);

        


        // # watch -n0 ss -t4ane 
        // # netcat localhost 3000
        // comenzar a enviar data por netcat. 
        // observar Recv-Q en la conexión establecida como se llena





        // El proceso toma los dato del buffer TCP
        // Recv-Q queda en 0

        char respuesta[1024] = {'\0'};

        int bytesRecibidos = 0;

        bytesRecibidos = recv(socketClient, &respuesta, 1024, 0);

        printf("Mensaje cliente: (%d bytes) %s\n", bytesRecibidos, respuesta);

        close(socketClient);

    }


}

