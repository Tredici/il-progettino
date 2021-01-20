/** Questo file conterrà il main
 * del discovery server e, di
 * conseguenza, tutte le inclusioni
 * di cui esso ha bisogno.
 */
#include "ds-src/ds_esc.h"
#include "ds-src/ds_showpeers.h"
#include "ds-src/ds_udp.h"
#include "ds-src/ds_showneighbours.h"
#include "commons.h"
#include "repl.h"
#include "main_loop.h"
#include "unified_io.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARGNAME "porta"

void usageHelp(const char* p)
{
    printf("Usage:\n\t%s <" ARGNAME ">\n", p);

    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    char serverID[10];

    struct main_loop_command commands[] = {
        { "showneighbors", &showneighbours, "[peer] mostra i neighbor di un peer" },
        { "showneighbours", &showneighbours, "come showneighbors ma più british" },
        { "showpeers", &showpeers, "mostra un elenco dei peer connessi" },
        { "esc", &esc, "termina il server e tutti i peer" }
    };

    int commandNumber = sizeof(commands)/sizeof(struct main_loop_command );
    int port;

    if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usageHelp(argv[0]);

    /* parsing dell'argomento */
    port = argParseIntRange(argv[1], ARGNAME, 0, (1<<16)-1);

    if (unified_io_init() == -1)
        errExit("*** Errore attivazione sottosistema di I/O ***\n");

    printf("Attivato sottosistema di I/O.\n");

    port = UDPstart(port);
    if (port == -1)
        errExit("*** Errore attivazione sottosistema UDP ***\n");

    printf("Attivato sottosistema UDP con porta (%d)\n", port);

    sprintf(serverID, "[%d]", port);

    main_loop(serverID, commands, commandNumber);

    unified_io_set_mode(UNIFIED_IO_SYNC_MODE);
    if (UDPstop() == -1)
        errExit("*** Errore terminazione sottosistema UDP ***\n");
    unified_io_set_mode(UNIFIED_IO_ASYNC_MODE);

    if (unified_io_close() == -1)
        errExit("*** Errore terminazione sottosistema IO ***\n");

    printf("Server terminato correttamente.\n");

    return EXIT_SUCCESS;
}
