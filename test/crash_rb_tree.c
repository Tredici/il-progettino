#include "../rb_tree.h"

#include <stdlib.h>
//#include <time.h>
#include <stdio.h>

void test(void* val)
{
    printf("Destroying (%d)\n", (int)val);
}

int main(int argc, char* argv[])
{
    struct rb_tree* T;
    long int key;
    int i;

    int keys[] = {767232, 18072, 560905, 738211,
                725933, 657435, 640532, 319026};

    //srand(time(NULL));
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
    for (i=0; i<8; ++i)
    {
        //key = rand()%1000000;
        key = keys[i];
        rb_tree_set(T, key, (void*)(long int)i);
        printf("T, inserted: %10ld, size (%zd)\n", key, rb_tree_size(T));
        rb_tree_debug(T);
        puts("");
    }
    printf("\n");
    printf("T succesfully filled, size: (%zd)\n", rb_tree_size(T));
    rb_tree_debug(T);
    rb_tree_clear(T);

    rb_tree_debug(T);
    rb_tree_destroy(T);

    return 0;
}
