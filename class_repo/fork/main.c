//
//  main.c
//  Fork
//
//  Created by Diego Marafetti on 3/14/16.
//  Copyright (c) 2016 diego. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, const char **argv) {
    
    
    printf("Executing before fork.\n");
    

    int fd = open("./test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        

    //
    // fork process 
    //
    pid_t pid = fork();
        

    
    //
    // check for errors
    //
    if(pid == -1) {
    
        perror("Error calling fork()");
        exit(EXIT_FAILURE);
    }
        
  
    
    
    
    if(pid == 0) {
        
        // test fork + execv
        
        /*
        
        char *arr[] = {"ls", "-l", "-R", "-a", NULL};
        
        int ret = execv("/bin/ls", arr);
        
        if(ret < 0) {
            
            perror("Error: ");
            return -1;
            
        }
        
        */
    
        printf("I am the child with pid %d from parent %d - fork pid %d\n", getpid(), getppid(), pid);
        
        
        
        
        write(fd, "123", 3);
        printf("File descriptor: %d - offset: %lld\n", fd, lseek(fd, 0, SEEK_CUR));
        close(fd);
        
        printf("child end.\n");
                
    
    } else {
    
        printf("I am the parent with pid %d - child pid %d\n", getpid(), pid);
        
        // The waitpid() system call suspends execution of the calling process
        // until a child specified by pid argument has changed state.
        // By default, waitpid() waits only for terminated children,
        // but this behavior is modifiable via the options argument, as described below.
        int exitStatus;
        
        printf("waiting for child to finish...\n");

        waitpid(pid, &exitStatus, 0);
            
        

        write(fd, "456", 3); //10.0.0.1/home/test.txt = 10ms / 500ms
        
        
        printf("File descriptor: %d - offset: %lld\n", fd, lseek(fd, 0, SEEK_CUR));
        close(fd);
        
        printf("Retrieved pid: %d, parent end.\n", pid);
        
    }
    
    return 0;
}

