#include "../main_loop.h"
#include "../unified_io.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define THREADN 1

/* print arguments */
static int pr_args(const char* str)
{
    printf("\tpr_args: strlen(\"%s\")=(%d)\n", str, (int)strlen(str));
    //puts(str);
    return 0;
}
static int cmd1(const char* str)
{
    printf("\tcmd1: %s\n", str);
    return 0;
}
static int cmd2(const char* str)
{
    printf("\tcmd2: %s\n", str);
    return 0;
}
static int cmd22(const char* str)
{
    printf("\tcmd22: %s\n", str);
    return 0;
}
static int cmd11(const char* str)
{
    printf("\tcmd11: %s\n", str);
    return 0;
}
static int somma(const char* str)
{
    int a,b;
    sscanf(str, "%d %d", &a, &b);
    printf("\tsomma [\"%s\"]: %d + %d = %d\n", str, a, b, a+b);
    return 0;
}

static int stop(const char* str)
{
    (void) str;
    printf(" *** FINE *** \n");
    return OK_TERMINATE;
}

static void* thread(void* args)
{
    long id = (long)args;
    char buffer[256];
    int i;

    i = 0;
    while (1)
    {
        sprintf(buffer, "thread [%ld] output (%d)", id, i++);
        unified_io_push(buffer, UNIFIED_IO_NORMAL);
        usleep(100000 + (rand()%2000000));
    }
    
    return NULL;
}

int main()
{
    //repl_sign = NULL;
    struct main_loop_command commands[] = {
        { "cmd1", &cmd1, "comando 1" },
        { "cmd11", &cmd11, NULL },
        { "cmd2", &cmd2, NULL },
        { "cmd22", &cmd22, NULL },
        { "som", &pr_args, NULL },
        { "somm", &pr_args, NULL },
        { "somma", &somma, "somma di due numeri" },
        { "case", &pr_args, NULL },
        { "stop", &stop, "termina il ciclo" }
    };

    int commandNumber = sizeof(commands)/sizeof(struct main_loop_command );

    srand(time(NULL));
    unified_io_init();

    /* avvia i singoli thread */
    for (long i = 0; i!=THREADN; ++i)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, &thread, (void*)i);
    }

    main_loop("INPUT", commands, commandNumber);

    return 0;
}
