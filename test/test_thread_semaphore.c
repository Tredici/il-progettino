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

/* Long life thread */
static void *ttest(void* args)
{
    struct thread_semaphore* ts;

    printf("\tttest(): thread started\n");
    if (args == NULL)
    {
        fprintf(stderr, "ttest(): args is NULL\n");
        exit(EXIT_FAILURE);
    }
    ts = (struct thread_semaphore*)args;

    printf("\tttest(): pause(3)\n");
    sleep(3);
    printf("\tttest(): segnala il semaforo\n");

    thread_semaphore_signal(ts, STATUS, (void*)str);

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
    struct thread_semaphore* ts;
    int status;
    char* s_check;
    pthread_t tid;

    ts = thread_semaphore_init();
    if (ts == NULL)
    {
        fprintf(stderr, "main(): thread_semaphore_init()\n");
        exit(EXIT_FAILURE);
    }

    printf("main(): creato ts.\n");
    if (pthread_create(&tid, NULL, &ttest, (void*)ts) != 0)
    {
        fprintf(stderr, "main(): thread_semaphore_init()\n");
        exit(EXIT_FAILURE);
    }
    printf("main(): creato il thread (%ld).\n", (long int)tid);

    printf("main(): aspettando il semaforo.\n");
    if (thread_semaphore_wait(ts) != 0)
    {
        fprintf(stderr, "main(): thread_semaphore_init()\n");
        exit(EXIT_FAILURE);
    }

    printf("main(): È ora!\n");
    if (thread_semaphore_get_status(ts, &status, (void**) &s_check) != 0)
    {
        fprintf(stderr, "main(): thread_semaphore_init()\n");
        exit(EXIT_FAILURE);
    }

    if (status != STATUS || s_check != str)
    {
        fprintf(stderr, "main(): ERRORE parametri!\n");
        exit(EXIT_FAILURE);
    }


    thread_semaphore_destroy(ts);
    ts = NULL;

    printf("main(): Si aspetta un input per continuare!\n");
    getchar();

    return 0;
}
