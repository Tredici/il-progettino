#include "peer_start.h"
#include "peer_udp.h"
#include "../repl.h"
#include <stdio.h>


int start(const char* args)
{
    char hostname[32] = "";
    char portname[8] = "";

    if (sscanf(args, "%31s %7s", hostname, portname) == -1)
    {
        fprintf(stderr, "Wrong sintax: start <DS_addr DS_port>\n");
        return ERR_PARAMS;
    }
    printf("start: <%s> <%s>\n", hostname, portname);

    if (UDPconnect(hostname, portname) == -1)
    {
        printf("Impossibile raggiungere [%s:%s]\n", hostname, portname);
        return ERR_FAIL;
    }

    printf("Connessione al network riuscita!\n");

    return OK_CONTINUE;
}