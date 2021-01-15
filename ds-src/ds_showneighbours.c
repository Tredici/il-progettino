#include "ds_showneighbours.h"
#include "../repl.h"
#include <stdio.h>

int showneighbours(const char* args)
{
    long int node;

    if (sscanf(args, "%ld", &node) == 1)
    {
        /* lavora su un solo argomento */
        if (peers_showneighbour(node) == -1)
        {
            fprintf(stderr, "Errore: il peer %ld non esiste\n", node);
            return ERR_PARAMS;
        }
    }
    else
    {
        /* mostra i vicini di ogni nodo */
        if (peers_showneighbour(NULL) == -1)
            return ERR_FAIL;
    }

    return OK_CONTINUE;
}

