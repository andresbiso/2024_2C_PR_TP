/**
 * Original Author: https://github.com/mbrossard/threadpool
 * The file was updated and adapted to the requirements of this assignment
 */

/**
 * @file threadpool.c
 * @brief Threadpool implementation file
 */

// Standard library headers
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Project header
#include "threadpool.h"

/**
 * @function void *threadpool_thread(void *threadpool)
 * @brief the worker thread
 * @param threadpool the pool which own the thread
 */
// static limits the function's scope to the file it is declared in.
static void *threadpool_thread(void *threadpool);

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags)
{
    threadpool_t *pool;
    int i;
    (void)flags;

    if (thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)
    {
        return NULL;
    }

    if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL)
    {
        goto err;
    }

    /* Initialize */
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;

    /* Allocate thread and task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

    /* Initialize mutex and conditional variable first */
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0) ||
        (pool->threads == NULL) ||
        (pool->queue == NULL))
    {
        goto err;
    }

    /* Start worker threads */
    for (i = 0; i < thread_count; i++)
    {
        if (pthread_create(&(pool->threads[i]), NULL,
                           threadpool_thread, (void *)pool) != 0)
        {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;

err:
    if (pool)
    {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_add(threadpool_t *pool, void *(*function)(void *), void *argument, threadpool_task_t **out_task, int flags)
{
    int err = 0;
    int next;
    (void)flags;

    if (pool == NULL || function == NULL)
    {
        return threadpool_invalid;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return threadpool_lock_failure;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do
    {
        /* Are we full ? */
        if (pool->count == pool->queue_size)
        {
            err = threadpool_queue_full;
            break;
        }

        /* Are we shutting down ? */
        if (pool->shutdown)
        {
            err = threadpool_shutdown;
            break;
        }

        /* Add task to queue */
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->queue[pool->tail].result = NULL;
        pthread_mutex_init(&(pool->queue[pool->tail].result_lock), NULL);
        pthread_cond_init(&(pool->queue[pool->tail].result_notify), NULL);
        pool->queue[pool->tail].done = 0;

        if (out_task != NULL)
        {
            *out_task = &(pool->queue[pool->tail]);
        }

        pool->tail = next;
        pool->count += 1;

        /* pthread_cond_broadcast */
        if (pthread_cond_signal(&(pool->notify)) != 0)
        {
            err = threadpool_lock_failure;
            break;
        }
    } while (0);

    if (pthread_mutex_unlock(&pool->lock) != 0)
    {
        err = threadpool_lock_failure;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if (pool == NULL)
    {
        return threadpool_invalid;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return threadpool_lock_failure;
    }

    do
    {
        /* Already shutting down */
        if (pool->shutdown)
        {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ? graceful_shutdown : immediate_shutdown;

        /* Wake up all worker threads */
        if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
            (pthread_mutex_unlock(&(pool->lock)) != 0))
        {
            err = threadpool_lock_failure;
            break;
        }

        /* Join all worker threads */
        for (i = 0; i < pool->thread_count; i++)
        {
            if (pthread_join(pool->threads[i], NULL) != 0)
            {
                err = threadpool_thread_failure;
            }
        }
    } while (0);

    /* Only if everything went well do we deallocate the pool */
    if (!err)
    {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if (pool == NULL || pool->started > 0)
    {
        return -1;
    }

    /* Did we manage to allocate? */
    if (pool->threads)
    {
        free(pool->threads);
        free(pool->queue);

        /* Because we allocate pool->threads after initializing the
           mutex and condition variable, we're sure they're
           initialized. Let's lock the mutex just in case. */
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
    return 0;
}

static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    for (;;)
    {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while ((pool->count == 0) && (!pool->shutdown))
        {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if ((pool->shutdown == immediate_shutdown) ||
            ((pool->shutdown == graceful_shutdown) &&
             (pool->count == 0)))
        {
            break;
        }

        /* Grab our task */
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        task.result = pool->queue[pool->head].result;
        task.result_lock = pool->queue[pool->head].result_lock;
        task.result_notify = pool->queue[pool->head].result_notify;
        task.done = 0;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        /* Unlock */
        pthread_mutex_unlock(&(pool->lock));

        /* Execute the task and store the result */
        task.result = (*(task.function))(task.argument);

        /* Signal task completion */
        pthread_mutex_lock(&(task.result_lock));
        task.done = 1;
        pthread_cond_signal(&(task.result_notify));
        pthread_mutex_unlock(&(task.result_lock));
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return NULL;
}

void *threadpool_wait_for_task(threadpool_task_t *task)
{
    void *result;

    pthread_mutex_lock(&(task->result_lock));
    while (!task->done)
    {
        pthread_cond_wait(&(task->result_notify), &(task->result_lock));
    }
    result = task->result;
    pthread_mutex_unlock(&(task->result_lock));

    /* Clean up */
    pthread_mutex_destroy(&(task->result_lock));
    pthread_cond_destroy(&(task->result_notify));

    return result;
}
