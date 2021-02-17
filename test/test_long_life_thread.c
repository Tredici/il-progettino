/** Prova a creare un thread dalla lunga
 * vita e vede se va tutto bene
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../thread_semaphore.h"

#define STATUS 1254
const char* str = "PANINOOOO";

volatile pthread_t son;

/* Long life thread */
void *ttest(void* args)
{
    struct thread_semaphore* ts;
    pthread_t* data;

    son = pthread_self();
    printf("\tttest(): thread (%ld) started\n", (long)son);
    if (args == NULL)
    {
        fprintf(stderr, "ttest(): args is NULL\n");
        exit(EXIT_FAILURE);
    }
    ts = thread_semaphore_form_args(args);
    data = (pthread_t*)thread_semaphore_get_args(args);
    *data = son;

    printf("\tttest(): pause(3)\n");
    sleep(3);
    printf("\tttest(): segnala il semaforo\n");

    thread_semaphore_signal(ts, 0, (void*)(long)pthread_self());

    while (1)
    {
        printf("\tttest(): running...\n");
        sleep(1);
    }

    /* in teoria non ci arriverà mai */
    return NULL;
}

int main()
{
    pthread_t tid;
    pthread_t data;

    printf("main(): [%ld]!\n", (long)pthread_self());
    if (start_long_life_thread(&tid, &ttest, (void*)&data, NULL) != 0)
    {
        fprintf(stderr, "main(): start_long_life_thread()\n");
        exit(EXIT_FAILURE);
    }

    if (son != data || tid != son)
    {
        fprintf(stderr, "main(): ERRORE creazione thread!\n");
        exit(EXIT_FAILURE);
    }

    printf("main(): Si aspetta un input per continuare!\n");
    getchar();

    return 0;
}
