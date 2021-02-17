/** Questo test serve a capire se le funzioni che
 * ho scritto per trovare il precedente e il
 * successore di un nodo funzionano
 *
 * Riempie l'albero con numeri consecutivi e a quel
 * punto controlla il successore e il precedente di
 * ogni elemento
 */
#include "../rb_tree.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define NUMBER 50

int counter;

static void test(long int key, void* value)
{
    if (key != (long)value)
    {
        fprintf(stderr, "ERRORE: test - inconsistenza [key,value]\n");
        exit(EXIT_FAILURE);
    }
    if (key != counter)
    {
        fprintf(stderr, "ERRORE: test - fuori ordine\n");
        exit(EXIT_FAILURE);
    }
    printf("OK - [%ld,%ld]\n", key, (long)value);
    ++counter;
}

int main(/* int argc, char* argv[] */)
{
    struct rb_tree* T;
    int i;
    ssize_t size;

    T = rb_tree_init(NULL);
    if (T == NULL)
    {
        fprintf(stderr, "rb_tree_init\n");
        exit(EXIT_FAILURE);
    }

    printf("Riempimento albero.\n");
    for (i=0; i<NUMBER; ++i)
    {
        rb_tree_set(T, (long int)i, (void*)(long int)i);
    }
    size = rb_tree_size(T);
    if (size != NUMBER)
    {
        fprintf(stderr, "*** Disastro riempimento ***\n");
        exit(EXIT_FAILURE);
    }
    printf("Albero riempito.\n");

    printf("Test rb_tree_foreach:\n");
    if (rb_tree_foreach(T, &test) != 0 || NUMBER != counter)
    {
        fprintf(stderr, "*** Disastro foreach ***\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUPERATO!\n");

    rb_tree_destroy(T);
    printf("DONE!\n");

    return 0;
}

