#include "../list.h"
#include "../commons.h"
#include <stdio.h>

#define NUM 10


int parity(void* val, void* mod)
{
    return (long)val % 2 == (long)mod;
}


int main(void)
{
    struct list* l,* tmp;
    int i;

    l = list_init(NULL);
    if (l == NULL)
        errExit("*** list_init ***\n");

    for (i = 0; i != NUM; ++i)
    {
        if (list_append(l, (void*)(long)i) == -1)
            errExit("*** list_init ***\n");
    }
    list_debug(l);

    printf("TEST parità\n");
    printf("Pari:\n");
    tmp = list_select(l, &parity, (void*)(long)0);
    if (tmp == NULL)
        errExit("*** list_select ***\n");
    list_debug(tmp);
    list_destroy(tmp);

    printf("Dispari:\n");
    tmp = list_select(l, &parity, (void*)(long)1);
    if (tmp == NULL)
        errExit("*** list_select ***\n");
    list_debug(tmp);
    list_destroy(tmp);
    printf("OK!\n");

    list_destroy(l);

    return 0;
}
