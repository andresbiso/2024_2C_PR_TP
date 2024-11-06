
/**
 * Original Author: https://github.com/mbrossard/threadpool
 * The file was updated and adapted to the requirements of this assignment
 */

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

// Standard library headers
#include <pthread.h>

/**
 * @file threadpool.h
 * @brief Threadpool Header File
 */

/**
 * Increase this constants at your own risk
 * Large values might slow down your system
 */
#define MAX_THREADS 64
#define MAX_QUEUE 65536

typedef enum
{
    threadpool_invalid = -1,
    threadpool_lock_failure = -2,
    threadpool_queue_full = -3,
    threadpool_shutdown = -4,
    threadpool_thread_failure = -5
} threadpool_error_t;

typedef enum
{
    threadpool_graceful = 1
} threadpool_destroy_flags_t;

/**
 * @struct threadpool_task
 * @brief the work struct
 *
 * @var function Pointer to the function that will perform the task.
 * @var argument Argument to be passed to the function.
 * @var result   Pointer to store the result of the task.
 * @var result_lock Mutex to lock the result access.
 * @var result_notify Condition variable to notify the result availability.
 * @var done     Flag indicating if the task is done.
 */
typedef struct
{
    void *(*function)(void *);
    void *argument;
    void *result;
    pthread_mutex_t result_lock;
    pthread_cond_t result_notify;
    int done;
} threadpool_task_t;

typedef enum
{
    immediate_shutdown = 1,
    graceful_shutdown = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads.
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks.
 *  @var shutdown     Flag indicating if the pool is shutting down.
 *  @var started      Number of started threads.
 */
typedef struct threadpool_t
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
} threadpool_t;

/**
 * @function threadpool_create
 * @brief Creates a threadpool_t object.
 * @param thread_count Number of worker threads.
 * @param queue_size   Size of the queue.
 * @param flags        Unused parameter.
 * @return a newly created thread pool or NULL
 */
threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);

/**
 * @function threadpool_add
 * @brief add a new task in the queue of a thread pool
 * @param pool     Thread pool to which add the task.
 * @param function Pointer to the function that will perform the task.
 * @param argument Argument to be passed to the function.
 * @param out_task Pointer to store the added task.
 * @param flags    Unused parameter.
 * @return 0 if all goes well, negative values in case of error (@see
 * threadpool_error_t for codes).
 */
int threadpool_add(threadpool_t *pool, void *(*function)(void *),
                   void *argument, threadpool_task_t **out_task, int flags);

/**
 * @function threadpool_destroy
 * @brief Stops and destroys a thread pool.
 * @param pool  Thread pool to destroy.
 * @param flags Flags for shutdown
 *
 * Known values for flags are 0 (default) and threadpool_graceful in
 * which case the thread pool doesn't accept any new tasks but
 * processes all pending tasks before shutdown.
 */
int threadpool_destroy(threadpool_t *pool, int flags);

/**
 * @function threadpool_wait_for_task
 * @brief Waits for a task to complete and returns the result.
 * @param task  The task to wait for.
 * @return The result of the task.
 */
void *threadpool_wait_for_task(threadpool_task_t *task);

int threadpool_free(threadpool_t *pool);

#endif /* _THREADPOOL_H_ */
