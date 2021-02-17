#include "../commons.h"
#include "../unified_io.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#define THREADS 100
#define ACTION_FOR_THREAD 100

/* per gestione dei thread */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int alive = 0;

static void* test(void* arg)
{
    long id;
    char buffer[512];
    int i;
    char myName[10];
    char test[20];

    id = (long)arg;

    sprintf(myName, "%ld", id);
    if (unified_io_set_thread_name(myName) != 0)
        errExit("unified_io_set_thread_name");

    for (i = 0; i < ACTION_FOR_THREAD; i++)
    {
        sprintf(buffer, "thread [%ld] message (%d)", id, i);
        unified_io_push(UNIFIED_IO_NORMAL, buffer);
    }

    pthread_mutex_lock(&mutex);
    --alive;
    pthread_mutex_unlock(&mutex);

    return NULL;
}


int main(/*int argc, char* argv[]*/)
{
    int i, end;
    pthread_t tid;

    unified_io_init();

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
        if (unified_io_print(1) == -1)
        {
            if (errno != EWOULDBLOCK)
               errExit("*** unified_io_print() ***\n");
            
            pthread_mutex_lock(&mutex);
            end = alive == 0;
            pthread_mutex_unlock(&mutex);
            if (end)
                break;
        }
    }

    unified_io_close();

    printf("DONE!\n");

    return 0;
}
