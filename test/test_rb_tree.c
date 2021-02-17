#include "../rb_tree.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

long int counter = 0;

void test(void* val)
{
    ++counter;
    //printf("Destroying (%d)\n", (int)val);
}

#define NUMBER 100000
#define TO_DELETE (NUMBER)


long int news[NUMBER];

int main(int argc, char* argv[])
{
    struct rb_tree* T;
    long int key;
    int i;
    ssize_t size;

    srand(time(NULL));
    //srand(0);
    T = rb_tree_init(NULL);
    rb_tree_set_cleanup_f(T, &test);

    if (&test != rb_tree_get_cleanup_f(T))
    {
        fprintf(stderr, "rb_tree_?et_cleanup_f\n");
        exit(EXIT_FAILURE);
    }

    if (T == NULL)
    {
        fprintf(stderr, "rb_tree_init\n");
        exit(EXIT_FAILURE);
    }
    printf("T created, size: (%zd)\n", rb_tree_size(T));
    for (i=0; i<NUMBER; ++i)
    {
        key = rand();//%1000000;
        news[i] = key;
        rb_tree_set(T, key, (void*)(long int)i);
        //printf("T, inserted: %10ld, size (%zd)\n", key, rb_tree_size(T));
        //rb_tree_debug(T);
    }
    printf("\n");
    size = rb_tree_size(T);
    printf("T succesfully filled, size: (%zd)\n", size);
    //rb_tree_debug(T)
    printf("\nInizio eliminazione (%d) elementi:\n", TO_DELETE);
    counter = 0;
    for (i=0; i<TO_DELETE; ++i)
    {
        printf("Tentativo (%d/%d) eliminazione [%ld]\n", i+1, TO_DELETE, news[i]);
        rb_tree_remove(T, news[i], NULL);
        //printf("Stato dell'albero:\n");
        //rb_tree_debug(T);
        if (rb_tree_check_integrity(T) < 0)
        {
            fprintf(stderr, "NOOOOOOOO\n");
            exit(EXIT_FAILURE);
        }
        /*else
        {
            printf("Albero ben formato!\n");
        }
        printf("\n");*/
    }
    printf("Rimossi (%ld) nodi, rimasti: (%zd)\n",
        counter, rb_tree_size(T));

    if (size != counter + rb_tree_size(T))
    {
        fprintf(stderr, "Andata male.\n");
        exit(EXIT_FAILURE);
    }
    printf("DONE!\n");

    //rb_tree_clear(T);
    counter = 0;
    rb_tree_destroy(T);
    printf("Distrutti (%ld) nodi.\n", counter);

    //rb_tree_debug(T);

    return 0;
}
