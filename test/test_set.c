/** Questo test serve a capire se le funzioni che
 * ho scritto per trovare il precedente e il
 * successore di un nodo funzionano
 *
 * Riempie l'albero con numeri consecutivi e a quel
 * punto controlla il successore e il precedente di
 * ogni elemento
 */
#include "../set.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define NUMBER 50

void next(struct set* S)
{
    int err;
    int i;
    long int key;

    for (i=0; i<NUMBER; ++i)
    {
        err = set_next(S, (long int)i, &key);

        switch (err)
        {
        case 0:
            if (i == NUMBER-1)
            {
                fprintf(stderr, "ERRORE set_next(): successore ultimo elemento\n");
                exit(EXIT_FAILURE);
            }
            if (i+1 != key)
            {
                fprintf(stderr, "ERRORE set_next(): elementi non consecutivi\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        case -1:
            if (i != NUMBER-1)
            {
                fprintf(stderr, "ERRORE set_next(): successore mancante\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "ERRORE set_next(): NONSENSE\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
}

static void f(long int x)
{
    printf(" (%d) ", (int)x);
}

static void print(struct set* S)
{
    printf("[");
    set_foreach(S, &f);
    printf("]\n");
}

static void testUnion()
{
    struct set* S1;
    struct set* S2;
    struct set* U;

    int i;
    const int limit = 5;

    printf("Test set_union:\n");

    S1 = set_init(NULL); S2 = set_init(NULL);

    /* mette i dispari */
    for(i=0; i!= limit; ++i) set_add(S1, 2*i+1);
    /* mette i pari */
    for(i=0; i!= limit; ++i) set_add(S2, 2*i);
    printf("Test set_union:\n");

    printf("S1: "); print(S1);
    printf("S2: "); print(S2);

    U = set_union(S1, S2);
    printf("U:  "); print(U);

    set_destroy(U);
    set_destroy(S1); set_destroy(S2);
    printf("OK set_union!\n");
}

int main(/* int argc, char* argv[] */)
{
    struct set* S;
    struct set* S2; /* copia */
    int i, err;
    ssize_t size;
    /* per testare precedente e successivo */
    long int key;

    S = set_init(NULL);
    if (S == NULL)
    {
        fprintf(stderr, "set_init\n");
        exit(EXIT_FAILURE);
    }

    printf("Riempimento set.\n");
    for (i=0; i<NUMBER; ++i)
    {
        set_add(S, (long int)i);
    }
    size = set_size(S);
    if (size != NUMBER)
    {
        fprintf(stderr, "*** Disastro riempimento ***\n");
        exit(EXIT_FAILURE);
    }
    printf("Set riempito.\n");

    /* set_min */
    printf("Test minimo:\n");
    if (set_min(S, &key) == -1 || key != 0)
    {
        fprintf(stderr, "set_min()\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUPERATO!\n");

    /* rb_tree_max */
    printf("Test massimo:\n");
    if (set_max(S, &key) == -1 || key != NUMBER-1)
    {
        fprintf(stderr, "set_max()\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUPERATO!\n");

    /* test next */
    printf("Test: set_next()\n");
    for (i=0; i<NUMBER; ++i)
    {
        err = set_next(S, (long int)i, &key);

        switch (err)
        {
        case 0:
            if (i == NUMBER-1)
            {
                fprintf(stderr, "ERRORE set_next(): successore ultimo elemento\n");
                exit(EXIT_FAILURE);
            }
            if (i+1 != key)
            {
                fprintf(stderr, "ERRORE set_next(): elementi non consecutivi\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        case -1:
            if (i != NUMBER-1)
            {
                fprintf(stderr, "ERRORE set_next(): successore mancante\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "ERRORE set_next(): NONSENSE\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    printf("TEST SUPERATO!\n");

    /* test PREV */
    printf("Test: set_prev()\n");
    for (i=0; i<NUMBER; ++i)
    {
        err = set_prev(S, (long int)i, &key);

        switch (err)
        {
        case 0:
            if (i == 0)
            {
                fprintf(stderr, "ERRORE set_prev(): predecessore primo elemento\n");
                exit(EXIT_FAILURE);
            }
            if (i-1 != key)
            {
                fprintf(stderr, "ERRORE set_prev(): elementi non consecutivi\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        case -1:
            if (i != 0)
            {
                fprintf(stderr, "ERRORE set_prev(): predecessore mancante\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "ERRORE set_prev(): NONSENSE\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    printf("TEST SUPERATO!\n");

    printf("TEST set_clone\n");
    S2 = set_clone(S);
    if (S2 == NULL)
    {
        fprintf(stderr, "ERRORE set_clone().\n");
        exit(EXIT_FAILURE);
    }
    set_destroy(S);

    next(S2);

    set_destroy(S2);

    testUnion();

    printf("DONE!\n");

    return 0;
}

