#include "cmd_shell.h"
#include "../repl.h"
#include "../unified_io.h"
#include "../commons.h"
#include <stdlib.h>
#include <stdio.h>

int shell(const char* args)
{
    int res;
    printf("Invoca la shell com cmd: \"%s\"\n", args);
    res = system(args);
    if (res != 0)
    {
        printError("Comando terminato con status: \"%d\"\n", res);
    }
    else
    {
        printSuccess("SUCCESS!\n");
    }
    return OK_CONTINUE;
}

