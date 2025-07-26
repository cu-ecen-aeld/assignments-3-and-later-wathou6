#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    pthread_mutex_lock(thread_func_args->mutex);
    usleep(thread_func_args->wait_to_release_ms * 1000);
    pthread_mutex_unlock(thread_func_args->mutex);

    return thread_func_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data* thdata = (struct thread_data*) malloc(sizeof(struct thread_data));
    thdata->mutex = mutex;
    thdata->wait_to_obtain_ms = wait_to_obtain_ms;
    thdata->wait_to_release_ms = wait_to_release_ms;
    thdata->thread_complete_success = false;

    int testrc = pthread_create(thread, NULL, threadfunc, (void*) thdata);
    if (testrc !=0) {
        return false;
    }
    return true;
}

