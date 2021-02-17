#include "../commons.h"
#include "../unified_io.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

#define THREADS 100
#define ACTION_FOR_THREAD 100

/* per gestione dei thread */
pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int alive = 0;

static void* test(void* arg)
{
    long id;
    char buffer[512];
    int i;

    id = (long)arg;

    for (i = 0; i < ACTION_FOR_THREAD; i++)
    {
        sprintf(buffer, "thread [%ld] message (%d)", id, i);
        unified_io_push(UNIFIED_IO_NORMAL, buffer);
    }

    pthread_mutex_lock(&Mutex);
    --alive;
    pthread_mutex_unlock(&Mutex);

    return NULL;
}


int main(/*int argc, char* argv[]*/)
{
    int i, end;
    pthread_t tid;

    unified_io_init();

    if (unified_io_set_mode(UNIFIED_IO_SYNC_MODE) != 0)
        errExit("*** unified_io_set_mode ***\n");

    alive = THREADS;
    for (i = 0; i < THREADS; i++)
    {
        if (pthread_create(&tid, NULL, &test, (void*)(long)i) != 0)
        {
            errExit("pthread_create\n");
        }
        printf("main(): Created thread (%d)\n", i);
    }

    end = 0;
    while (1)
    {
        errno = 0;
        pthread_mutex_lock(&Mutex);
        end = alive == 0;
        pthread_mutex_unlock(&Mutex);
        if (end)
            break;
        sched_yield();
    }
    /*printf("sleep()\n");
    sleep(1);
    unified_io_set_mode(UNIFIED_IO_SYNC_MODE);*/

    unified_io_close();

    printf("DONE!\n");

    return 0;
}
