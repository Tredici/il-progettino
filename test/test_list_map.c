#include "../list.h"
#include <stdlib.h>
#include <stdio.h>

#define _NUMBER 10

void sum(void* x, void* tot)
{
    *(int*)tot += (int)(long)x;
}

void* duplicate(void* x)
{
    return (void*)(2*(long)x);
}

int main()
{
    struct list* l,* l2;
    int i;
    /* per il test con last */
    void* val;
    //int somma, check;

    l = list_init(NULL);

    printf("Inizio:\n");
    list_debug(l);
    printf("Copia:\n");
    l2 = list_map(l, &duplicate, NULL);
    if (l2 == NULL)
    {
        fprintf(stderr, "*** FAIL list_map() ***");
        exit(EXIT_FAILURE);
    }
    list_debug(l2);
    list_destroy(l2);
    /* prova  */
    list_append(l, (void*)(long)42);
    list_debug(l);
    printf("list_map su un solo elemento:\n");
    l2 = list_map(l, &duplicate, NULL);
    if (l2 == NULL)
    {
        fprintf(stderr, "*** FAIL list_map() ***");
        exit(EXIT_FAILURE);
    }
    list_debug(l2);
    list_destroy(l2);

    /* ripulisce la lista */
    list_clear(l);
    printf("LISTA RESETTATATA!\n");
    list_debug(l);

    for (i = 0; i < _NUMBER; ++i)
    {
        //check += i;
        list_append(l, (void*)(long)i);
        list_last(l, &val);
        if ((int)(long)val != i)
        {
            fprintf(stderr, "*** FAIL list_last() ***");
            exit(EXIT_FAILURE);
        }
    }
    printf("list_append:\n");
    list_debug(l);

    l2 = list_map(l, &duplicate, NULL);
    if (l2 == NULL)
    {
        fprintf(stderr, "*** FAIL list_map() ***");
        exit(EXIT_FAILURE);
    }
    printf("list_map:\n");
    list_debug(l2);
    list_destroy(l2);

    list_destroy(l);
    return 0;
}
