/** File che conterrà il sorgente dei peer
 * Il codice sarà testato man mano PRIMA
 * di essere unito in un prodotto
 * definitivo
 */
#include "peer-src/peer_stop.h"
#include "peer-src/peer_udp.h"
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
    char peerID[10];

    struct main_loop_command commands[] = {
        { "stop", &stop, "termina il peer" }
    };

    int commandNumber = sizeof(commands)/sizeof(struct main_loop_command );
    int port;

    if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usageHelp(argv[0]);

    /* parsing dellìargomento */
    port = argParseIntRange(argv[1], ARGNAME, 0, (1<<16)-1);


    unified_io_init();
    port = UDPstart(port);
    if (port == -1)
        errExit("*** Errore attivazione sottosistema UDP ***\n");

    printf("Attivato sottosistema UDP con porta (%d)\n", port);

    sprintf(peerID, "[%d]", port);

    main_loop(peerID, commands, commandNumber);

    if (UDPstop() == -1)
    {
        errExit("*** Errore terminazione sottosistema UDP ***\n");
    }

    printf("Peer terminato correttamente.\n");

    return EXIT_SUCCESS;
}

