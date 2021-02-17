#include <stdio.h>
#include "../commons.h"

#define REC_LIMIT 5

void rec(int x)
{
    printf("rec(x = %d)\n", x);
    if (x > REC_LIMIT)
        fatal("***rec***");
    rec(x+1);
}

int main()
{
    printf("Test datal:\n");
    rec(0);

    return 0;
}

