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

struct thread_semaphore* thread_semaphore_init()
{
    struct thread_semaphore* ans;

    ans = malloc(sizeof(struct thread_semaphore));
    if (ans == NULL)
        return NULL;

    memset(ans, 0, sizeof(struct thread_semaphore));

    if (pthread_mutex_init(&ans->mutex, NULL) != 0)
    {
        free(ans);
        return NULL;
    }
    if (pthread_cond_init(&ans->cond, NULL) != 0)
    {
        pthread_mutex_destroy(&ans->mutex);
        free(ans);
        return NULL;
    }

    return ans;
}

int thread_semaphore_wait(struct thread_semaphore* ts)
{
    if (pthread_mutex_lock(&ts->mutex) != 0)
        return -1;

    while (ts->flag == TSF_WAIT)
        if (pthread_cond_wait(&ts->cond, &ts->mutex) != 0)
            return -1;

    if (pthread_mutex_unlock(&ts->mutex) != 0)
        return -1;

    return 0;
}

int thread_semaphore_get_status(struct thread_semaphore* ts, int* status, void** data)
{
    if (ts == NULL)
        return -1;

    if (status != NULL)
        *status = ts->status;

    if (data != NULL)
        *data = ts->data;

    return 0;
}

int thread_semaphore_destroy(struct thread_semaphore* ts)
{
    if (ts == NULL)
        return -1;

    if (pthread_cond_destroy(&ts->cond) != 0)
        return -1;

    if (pthread_mutex_destroy(&ts->mutex) != 0)
        return -1;

    return 0;
}
