
/**
 * Original Author: https://github.com/mbrossard/threadpool
 * The file was updated and adapted to the requirements of this assignment
 */

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

// Standard library headers
#include <pthread.h>

#define THREADPOOL_INVALID -1
#define THREADPOOL_LOCK_FAILURE -2
#define THREADPOOL_QUEUE_FULL -3
#define THREADPOOL_SHUTDOWN -4
#define THREADPOOL_THREAD_FAILURE -5
#define THREADPOOL_GRACEFUL 1

typedef struct
{
    void *(*function)(void *);
    void *argument;
    void *result; // To store the result of the thread function
    pthread_t *thread;
    pthread_mutex_t task_mutex;
    pthread_cond_t task_complete;
    int done;
} threadpool_task_t;

typedef struct
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *task_queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
} threadpool_t;

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);
int threadpool_add(threadpool_t *pool, void *(*function)(void *), void *argument, threadpool_task_t **task_out, int flags);
void *threadpool_wait(threadpool_task_t *task);
int threadpool_destroy(threadpool_t *pool, int flags);
int threadpool_free(threadpool_t *pool);

#endif /* _THREADPOOL_H_ */
