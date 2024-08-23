//
//  main.c
//  pthreads
//
//  Created by Diego Marafetti on 3/28/16.
//  Copyright (c) 2016 diego. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


typedef void * (* PCALLBACK) (void *);


void *printNumbers(void *args) {

    pthread_t hThread = pthread_self();
    
    for(int i = 0; i < 100; i++) {
    
        printf("thread %ld: %d\n", (long)hThread, i);
    }

    return NULL;
}






int main(int argc, const char * argv[]) {
    
    pthread_t threads[5];
        
    PCALLBACK callback = printNumbers;

    pthread_create(&threads[0], NULL, callback, NULL);
    pthread_create(&threads[1], NULL, callback, NULL);
    pthread_create(&threads[2], NULL, callback, NULL);
    pthread_create(&threads[3], NULL, callback, NULL);
    pthread_create(&threads[4], NULL, callback, NULL);


    // wait for all threads

    for(int i = 0; i < 5; i++) {

        pthread_join(threads[i], NULL);

    }

    printf("main thread finished... \n");

    // continuar la exec
    
    return 0;
}

