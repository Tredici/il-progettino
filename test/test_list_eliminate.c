#include "../list.h"
#include <stdlib.h>
#include <stdio.h>

#define _NUMBER 10

int isOdd(void* val)
{
    return (((long)val)%2);
}

int main()
{
    struct list* l;
    int i;
    int check;

    l = list_init(NULL);

    printf("Inizio:\n");
    list_debug(l);

    /* usando list_append */
    check = 0;
    for (i = 0; i < _NUMBER; ++i)
    {
        check += isOdd((void*)(long)(i/2));
        list_append(l, (void*)(long)(i/2));
    }
    if (list_size(l) != _NUMBER)

    printf("After list fill:\n");
    list_debug(l);

    /* prova e eliminare i dispari */
    if (check != list_eliminate(l, &isOdd))
    {
        fprintf(stderr, "*** FAIL list_eliminate() ***");
        exit(EXIT_FAILURE);
    }

    printf("list_eliminate:\n");
    list_debug(l);

    if (_NUMBER - check != list_size(l))
    {
        fprintf(stderr, "*** FAIL list_size() ***");
        exit(EXIT_FAILURE);
    }

    printf("SUCCESSO!\n");
    printf("sz iniziale (%d) - adesso (%d)\n", _NUMBER, (int)list_size(l));

    list_destroy(l);
    return 0;
}