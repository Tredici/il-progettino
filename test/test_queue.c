#include "../queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

#define THREADS 100
#define ACTION_FOR_THREAD 100

#define MAXN 1000

/* per gestione dei thread */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int alive = 0;

/* per la gestione dell'output */
pthread_mutex_t output = PTHREAD_MUTEX_INITIALIZER;

struct queue* q;

static void terminate()
{
    pthread_mutex_lock(&mutex);
    --alive;
    pthread_mutex_lock(&output);
    printf("Thread (%lu) terminated.\n", pthread_self());
    pthread_mutex_unlock(&output);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

static void start()
{
    pthread_mutex_lock(&output);
    printf("Thread (%lu) started.\n", pthread_self());
    pthread_mutex_unlock(&output);
}

static void* pusher(void* arg)
{
    int i;
    int val;
    int n;

    start();
    for (i = 0; i < ACTION_FOR_THREAD; i++)
    {
        val = rand() % MAXN;
        pthread_mutex_lock(&output);
        pthread_mutex_unlock(&output);
        n = queue_push(q, (void*)(long)val);
        printf("Thread (%lu) write n.(%d): [%d]\n", pthread_self(), n, val);
    }
    terminate();
    return NULL;
}

static void* reader(void* arg)
{
    int i;
    void* val;
    int err;
    int n;

    start();
    for (i = 0; i < ACTION_FOR_THREAD; i++)
    {
        n = queue_pop(q, &val, 0);
        pthread_mutex_lock(&output);
        pthread_mutex_unlock(&output);
        printf("Thread (%lu) read n.(%d): [%d]\n", pthread_self(), n, (int)(long)val);
    }
    terminate();
    return NULL;
}


int main(int argc, char* argv[])
{
    int i;
    pthread_t tid;
    int err;

    srand(time(NULL));
    q = queue_init(NULL, Q_CONCURRENT);


    for (i = 0; i < THREADS; i++)
    {
        pthread_mutex_lock(&mutex);
        err = pthread_create(&tid, NULL, pusher, NULL);
        if (err == 0)
        {
            ++alive;
            pthread_mutex_lock(&output);
            printf("Created writer thread (%d)\n", alive);
            pthread_mutex_unlock(&output);
        }
        else
        {
            pthread_mutex_lock(&output);
            fprintf(stderr, "FAILED CREATING WRITER THREAD!\n");
            pthread_mutex_unlock(&output);
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&mutex);
        err = pthread_create(&tid, NULL, reader, NULL);
        if (err == 0)
        {
            ++alive;
            pthread_mutex_lock(&output);
            printf("Created reader thread (%d)\n", alive);
            pthread_mutex_unlock(&output);
        }
        else
        {
            pthread_mutex_lock(&output);
            fprintf(stderr, "FAILED CREATING READING THREAD!\n");
            pthread_mutex_unlock(&output);
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&mutex);

    }

    pthread_mutex_lock(&mutex);
    while (alive > 0)
        pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    queue_destroy(q);

    q = NULL;
    return 0;
}
