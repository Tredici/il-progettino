/** File che conterrà il sorgente dei peer
 * Il codice sarà testato man mano PRIMA
 * di essere unito in un prodotto
 * definitivo
 */
#include "peer-src/peer_stop.h"
#include "peer-src/peer_add.h"
#include "peer-src/peer_get.h"
#include "peer-src/peer_udp.h"
#include "peer-src/peer_start.h"
#include "peer-src/peer_entries_manager.h"
#include "peer-src/peer_tcp.h"
#include "commons.h"
#include "repl.h"
#include "main_loop.h"
#include "unified_io.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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
        { "start", &start, "<DS_addr DS_port> termina il peer" },
        { "add", &add, "{" ADD_SWAB "|" ADD_NEW_CASE "} <quantity> crea e inserisce una nuova entry nel registo di oggi" },
        { "get", &get, "{totale|variazione} {" ADD_SWAB "|" ADD_NEW_CASE "} [period] calcola l'aggregazione sull'intervallo specificato" },
        { "stop", &stop, "termina il peer" }
    };

    int commandNumber = sizeof(commands)/sizeof(struct main_loop_command );
    int port;

    printf("PROCESS ID [%lu]\n", (unsigned long)getpid());

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

    if (TCPinit(port) != 0)
        errExit("*** Errore attivazione sottosistema TCP ***\n");

    printf("Attivato sottosistema TCP con porta (%d)\n", port);

    /* avvia il sottosistema a gestione dei peer */
    if (startEntriesSubsystem(port) != 0)
        errExit("*** Errore attivazione sottosistema ENTRIES ***\n");

    sprintf(peerID, "[%d]", port);

    if (main_loop(peerID, commands, commandNumber) == REPL_REPEAT_STOP)
        printf("Peer terminated by server.\n");

    if (UDPstop() == -1)
    {
        errExit("*** Errore terminazione sottosistema UDP ***\n");
    }

    if (TCPclose() == -1)
        errExit("*** Errore terminazione sottosistema TCP ***\n");

    if (closeEntriesSubsystem() == -1)
        errExit("*** Errore terminazione sottosistema ENTRIES ***\n");

    if (unified_io_close() == -1)
        errExit("*** Errore terminazione sottosistema IO ***\n");

    printf("Peer terminato correttamente.\n");

    return EXIT_SUCCESS;
}

