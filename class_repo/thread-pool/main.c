//
//  main.c
//  thread-pool
//
// Github: https://github.com/mbrossard/threadpool
//
//
//  Created by Diego Marafetti on 23/10/2017.
//  Copyright © 2017 Diego Marafetti. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include "threadpool.h"

#define randnum(min, max) ((rand() % (int)(((max) + 1) - (min))) + (min))

sem_t t1;
sem_t t2;
sem_t t3;
sem_t t4;
sem_t t5;


int getSemValue(sem_t *sem) {

    int value;

    int ret = sem_getvalue(sem, &value);

    if(ret < 0) {

        printf("%s\n", strerror(errno));

        return 0;
    }

    return value;

} 




/**
 * Tarea a ejecutar en el pool
 *
 */
void runnable(void *param) {

    sem_t *sem = (sem_t *) param;

    sleep(2);

    printf("end task...\n");

    sem_post(sem);

};


/**
 * Entry point
 */
int main(int argc, const char * argv[]) {
        
    int ret;
    

    sem_init(&t1, 0, 0);
    sem_init(&t2, 0, 0);
    sem_init(&t3, 0, 0);
    sem_init(&t4, 0, 0);
    sem_init(&t5, 0, 0);


    
    // create pool
    threadpool_t *pool = threadpool_create(2, 2, 0);
    
    
    
    
    // add tasks

    

    ret = threadpool_add(pool, &runnable, &t1, 0);
    
    if(ret < 0) {
        
        printf("add thread 1 error: %d \n", ret);
        
    }
    


    
    ret = threadpool_add(pool, &runnable, &t2, 0);
    
    if(ret < 0) {
        
        printf("add thread 2 error: %d \n", ret);
       
    }


  
    
    ret = threadpool_add(pool, &runnable, &t3, 0);
    
    if(ret < 0) {
        
        printf("add thread 3 error: %d \n", ret);
        
    }



   
    ret = threadpool_add(pool, &runnable, &t4, 0);
    
    if(ret < 0) {
        
        printf("add thread 4 error: %d \n", ret);
        
    }


  
    
    ret = threadpool_add(pool, &runnable, &t5, 0);
    
    if(ret < 0) {
        
        printf("add thread 5 error: %d \n", ret);
        
    }
  

    

            
    printf("waiting for tasks\n");
    
    sem_wait(&t1);
    sem_wait(&t2);
    sem_wait(&t3);
    sem_wait(&t4);
    sem_wait(&t5);
   
    printf("main thread finished\n");


    // destroy semaphore
    sem_destroy(&t1);
    sem_destroy(&t2);
    sem_destroy(&t3);
    sem_destroy(&t4);
    sem_destroy(&t5);

    // destroy pool
    threadpool_destroy(pool, 0);
    
  
    return 0;
}
