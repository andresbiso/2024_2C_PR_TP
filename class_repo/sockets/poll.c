//
//  main.c
//  poll-example
//
//  Created by Diego on 09/04/2019.
//  Copyright © 2019 Diego. All rights reserved.
//

#include "main.h"

int main(int argc, const char *argv[])
{

    char buffer[256];
    struct pollfd fds[2];
    struct addrinfo hints;
    struct addrinfo *serverInfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "9000", &hints, &serverInfo);

    if (serverInfo == NULL)
    {

        printf("Local ip address coudn't be obtained.\n");
        exit(EXIT_FAILURE);
    }

    int socketListen = socket(serverInfo->ai_family,
                              serverInfo->ai_socktype,
                              serverInfo->ai_protocol);

    /* mark socket as non-blocking */
    int yes = 1;
    setsockopt(socketListen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    fcntl(socketListen, F_SETFL, O_NONBLOCK);

    /* bind to localhost and make it passive */
    bind(socketListen, serverInfo->ai_addr, serverInfo->ai_addrlen);
    listen(socketListen, 10);

    /* watch stdin for input */
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    /* watch socket for input  */
    fds[1].fd = socketListen;
    fds[1].events = POLLIN;

    while (TRUE)
    {

        int ret = poll(fds, 2, 5000);

        printf("Poll returned: %d\n", ret);

        if (ret == -1)
        {
            fprintf(stderr, "Error while polling: %s\n", strerror(errno));
            return 1;
        }

        if (!ret)
        {
            printf("Timeout - 5 seconds elapsed.\n");
        }

        /* handle STDIN */
        if (fds[0].revents & POLLIN)
        {

            printf("0x%04X\n", fds[0].revents);

            memset(&buffer, 0, 256);
            ssize_t readed = read(fds[0].fd, &buffer, 256);
            printf("console %ld bytes readed: %s\n", readed, buffer);
            continue;
        }

        /* handle new connection */
        if (fds[1].revents & POLLIN)
        {

            printf("0x%04X\n", fds[1].revents);
            struct sockaddr sockaddClient;
            socklen_t sockaddClientLength;
            int socketNewCx = accept(socketListen, &sockaddClient, &sockaddClientLength);

            // aca!
            

        }
    }

    return 0;
}
