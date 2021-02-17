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

int counter;

void test(long int val)
{
    if (val != counter)
    {
        fprintf(stderr, "*** Inconsistenza test ***\n");
        exit(EXIT_FAILURE);
    }
    printf("test [%ld]\n", val);
    ++counter;
}

int main(/* int argc, char* argv[] */)
{
    struct set* S;
    int i;
    ssize_t size;

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

    /* test set_foreach. */
    printf("Test: set_foreach.()\n");
    counter = 0;
    set_foreach(S, &test);
    if (counter != NUMBER)
    {
        fprintf(stderr, "*** Disastro set_foreach ***\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUPERATO!\n");

    set_destroy(S);
    printf("DONE!\n");

    return 0;
}

