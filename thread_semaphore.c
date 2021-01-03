#include "thread_semaphore.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

enum thread_semaphore_flag
{
    TSF_WAIT, /* bloccati in attesa */
    TSF_GO  /* status ora è valido */
};

struct thread_semaphore{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    enum thread_semaphore_flag flag;
    int status;
    void* data;
};

