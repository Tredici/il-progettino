#include "ds_showpeers.h"
#include "ds_peers.h"
#include "../repl.h"

int showpeers(const char* args)
{
    (void)args;
    if (peers_showpeers() == -1)
        return ERR_FAIL;

    return OK_CONTINUE;
}
