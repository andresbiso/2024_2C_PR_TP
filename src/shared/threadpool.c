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

// Static since it is only used inside this file
static void *threadpool_thread(void *threadpool);

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags)
{
    threadpool_t *pool;
    int i;

    if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL)
    {
        return NULL;
    }

    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0) ||
        (pool->threads == NULL) ||
        (pool->task_queue == NULL))
    {
        if (pool)
        {
            threadpool_free(pool);
        }
        return NULL;
    }

    for (i = 0; i < thread_count; i++)
    {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool) != 0)
        {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;
}

int threadpool_add(threadpool_t *pool, void *(*function)(void *), void *argument, threadpool_task_t **task_out, int flags)
{
    int next, err = 0;

    if (pool == NULL || function == NULL)
    {
        return THREADPOOL_INVALID;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return THREADPOOL_LOCK_FAILURE;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do
    {
        if (pool->count == pool->queue_size)
        {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }

        if (pool->shutdown)
        {
            err = THREADPOOL_SHUTDOWN;
            break;
        }

        pool->task_queue[pool->tail].function = function;
        pool->task_queue[pool->tail].argument = argument;
        pool->task_queue[pool->tail].result = NULL;
        pool->task_queue[pool->tail].thread = &pool->threads[pool->tail];
        pool->task_queue[pool->tail].done = 0;
        pthread_mutex_init(&pool->task_queue[pool->tail].task_mutex, NULL);
        pthread_cond_init(&pool->task_queue[pool->tail].task_complete, NULL);

        if (task_out)
        {
            *task_out = &(pool->task_queue[pool->tail]);
        }

        pool->tail = next;
        pool->count += 1;

        if (pthread_cond_signal(&(pool->notify)) != 0)
        {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while (0);

    if (pthread_mutex_unlock(&pool->lock) != 0)
    {
        err = THREADPOOL_LOCK_FAILURE;
    }

    return err;
}

void *threadpool_wait(threadpool_task_t *task)
{
    if (task == NULL || task->thread == NULL)
    {
        return NULL;
    }

    pthread_mutex_lock(&task->task_mutex);
    while (!task->done)
    {
        pthread_cond_wait(&task->task_complete, &task->task_mutex);
    }
    pthread_mutex_unlock(&task->task_mutex);

    return task->result; // Return the stored result
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if (pool == NULL)
    {
        return THREADPOOL_INVALID;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return THREADPOOL_LOCK_FAILURE;
    }

    do
    {
        if (pool->shutdown)
        {
            err = THREADPOOL_SHUTDOWN;
            break;
        }

        pool->shutdown = (flags & THREADPOOL_GRACEFUL) ? THREADPOOL_GRACEFUL : 1;

        if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
            (pthread_mutex_unlock(&(pool->lock)) != 0))
        {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        for (i = 0; i < pool->thread_count; i++)
        {
            if (pthread_join(pool->threads[i], NULL) != 0)
            {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while (0);

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
        return THREADPOOL_INVALID;
    }

    if (pool->threads)
    {
        free(pool->threads);
        free(pool->task_queue);

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
    threadpool_task_t *task;
    void *result;

    for (;;)
    {
        pthread_mutex_lock(&(pool->lock));

        while ((pool->count == 0) && (!pool->shutdown))
        {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if ((pool->shutdown == 1) ||
            ((pool->shutdown == THREADPOOL_GRACEFUL) && (pool->count == 0)))
        {
            pthread_mutex_unlock(&(pool->lock));
            break;
        }

        task = &(pool->task_queue[pool->head]);
        task->done = 0;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        // Execute the function and store the result
        result = (*(task->function))(task->argument);
        task->result = result;

        pthread_mutex_lock(&task->task_mutex);
        task->done = 1;
        pthread_cond_signal(&task->task_complete);
        pthread_mutex_unlock(&task->task_mutex);
    }

    pool->started--;
    return NULL;
}
