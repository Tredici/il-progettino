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

int main(/* int argc, char* argv[] */)
{
    struct rb_tree* T;
    int i, err;
    ssize_t size;
    /* per testare precedente e successivo */
    long int key;
    void* value;

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

    /* rb_tree_min */
    printf("Test minimo:\n");
    if (rb_tree_min(T, &key, &value) == -1
        || key != (long)value || key != 0)
    {
        fprintf(stderr, "rb_tree_min()\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUPERATO!\n");

    /* rb_tree_max */
    printf("Test massimo:\n");
    if (rb_tree_max(T, &key, &value) == -1
        || key != (long)value || key != NUMBER-1)
    {
        fprintf(stderr, "rb_tree_max()\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUPERATO!\n");


    /* test next */
    printf("Test: rb_tree_next()\n");
    for (i=0; i<NUMBER; ++i)
    {
        err = rb_tree_next(T, (long int)i, &key, &value);
        if (key != (long)value)
        {
            fprintf(stderr, "ERRORE rb_tree_next(): inconsistenza [key,value]\n");
            exit(EXIT_FAILURE);
        }

        switch (err)
        {
        case 0:
            if (i == NUMBER-1)
            {
                fprintf(stderr, "ERRORE rb_tree_next(): successore ultimo elemento\n");
                exit(EXIT_FAILURE);
            }
            if (i+1 != key)
            {
                fprintf(stderr, "ERRORE rb_tree_next(): elementi non consecutivi\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        case -1:
            if (i != NUMBER-1)
            {
                fprintf(stderr, "ERRORE rb_tree_next(): successore mancante\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "ERRORE rb_tree_next(): NONSENSE\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    printf("TEST SUPERATO!\n");

    /* test PREV */
    printf("Test: rb_tree_prev()\n");
    for (i=0; i<NUMBER; ++i)
    {
        err = rb_tree_prev(T, (long int)i, &key, &value);
        if (key != (long)value)
        {
            fprintf(stderr, "ERRORE rb_tree_prev(): inconsistenza [key,value]\n");
            exit(EXIT_FAILURE);
        }

        switch (err)
        {
        case 0:
            if (i == 0)
            {
                fprintf(stderr, "ERRORE rb_tree_prev(): predecessore primo elemento\n");
                exit(EXIT_FAILURE);
            }
            if (i-1 != key)
            {
                fprintf(stderr, "ERRORE rb_tree_prev(): elementi non consecutivi\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        case -1:
            if (i != 0)
            {
                fprintf(stderr, "ERRORE rb_tree_prev(): predecessore mancante\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "ERRORE rb_tree_prev(): NONSENSE\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    printf("TEST SUPERATO!\n");

    rb_tree_destroy(T);
    printf("DONE!\n");

    return 0;
}

