#include "../list.h"
#include <stdlib.h>
#include <stdio.h>

#define _NUMBER 10

void sum(void* x, void* tot)
{
    *(int*)tot += (int)(long)x;
}

int main()
{
    struct list* l;
    int i;
    int somma, check;

    l = list_init(NULL);

    printf("Inizio:\n");
    list_debug(l);

    /* usando list_append */
    check = 0;
    for (i = 0; i < _NUMBER; ++i)
    {
        check += i;
        list_append(l, (void*)(long)i);
    }
    printf("list_append:\n");
    list_debug(l);
    somma = 0;
    list_accumulate(l, sum, &somma);
    if (somma != check)
    {
        fprintf(stderr, "*** FAIL list_accumulate() ***");
        exit(EXIT_FAILURE);
    }
    printf("OK: list_append\n");

    list_clear(l);
    if (list_size(l) != 0)
    {
        fprintf(stderr, "*** FAIL list_size() ***");
        exit(EXIT_FAILURE);
    }

    /* usando list_prepend */
    check = 0;
    for (i = 0; i < _NUMBER; ++i)
    {
        check += i;
        list_prepend(l, (void*)(long)i);
    }
    printf("list_prepend:\n");
    list_debug(l);
    somma = 0;
    list_accumulate(l, sum, &somma);
    if (somma != check)
    {
        fprintf(stderr, "*** FAIL list_accumulate() ***");
        exit(EXIT_FAILURE);
    }
    printf("OK: list_prepend\n");

    printf("SUCCESSO! Somma (0)..(%d) = (%d)\n", (int)_NUMBER, somma);

    list_destroy(l);
    return 0;
}
