#include "peer_start.h"
#include "peer_udp.h"
#include "../repl.h"
#include "../unified_io.h"
#include <stdio.h>


int start(const char* args)
{
    static int once; /* questo comando può essere invocato solo una volta con successo */
    char hostname[32] = "";
    char portname[8] = "";

    if (once)
    {
        printf("ATTENZIONE: il peer è già stato connesso al network!\n");
        return OK_CONTINUE;
    }

    if (sscanf(args, "%31s %7s", hostname, portname) == -1)
    {
        fprintf(stderr, "Wrong sintax: start <DS_addr DS_port>\n");
        return ERR_PARAMS;
    }
    printf("start: <%s> <%s>\n", hostname, portname);

    unified_io_set_mode(UNIFIED_IO_SYNC_MODE);
    if (UDPconnect(hostname, portname) == -1)
    {
        unified_io_set_mode(UNIFIED_IO_ASYNC_MODE);
        printf("Impossibile raggiungere [%s:%s]\n", hostname, portname);
        return ERR_FAIL;
    }
    /* connesso con successo */
    once = 1;

    unified_io_set_mode(UNIFIED_IO_ASYNC_MODE);
    printf("Connessione al network riuscita!\n");

    return OK_CONTINUE;
}
