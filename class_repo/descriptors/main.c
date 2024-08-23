
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


int main(int argc, const char **argv) {
     
        
    int fd = open("./sync_test.txt", O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0666);
        
    const char *wBuffer = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";


    // io nonblocking
    
    fcntl(fd, F_SETFL, O_NONBLOCK);


    // write file

    int bytesWritten = write(fd, wBuffer, strlen(wBuffer));

    printf("bytes written: %d\n", bytesWritten);



    
    int offset = lseek(fd, 0, SEEK_SET);

    if(offset == -1) {

        printf("error lseek: %s\n", strerror(errno));

        exit(1);

    }



    // read file

    char *rBuffer = (char *) malloc(strlen(wBuffer));

    int ret = read(fd, rBuffer, 10);

    if(ret == -1) {

        printf("error read: %s\n", strerror(errno));

    } else {

        printf("buffer readed: %s\n", rBuffer);

    }


    
    close(fd);

    return 0;
}

