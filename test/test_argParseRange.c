#include "../commons.h"
#include <string.h>
#include <stdio.h>


int main(int argc, char* argv[])
{
    int num;

    if (argc != 2)
    {
        errExit("Usage:\n\t%s <num>\n", argv[0]);
    }

    num = argParseIntRange(argv[1], "num", 0, (1<<16)-1);
    printf("OK [%d]\n", num);

    return 0;
}
