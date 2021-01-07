/** File che conterrà il sorgente dei peer
 * Il codice sarà testato man mano PRIMA
 * di essere unito in un prodotto
 * definitivo
 */
#include "peer-src/peer_stop.h"
#include "peer-src/peer_udp.h"
#include "repl.h"
#include "main_loop.h"
#include "unified_io.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void usageHelp(const char* p)
{
    printf("Usage:\n\t%s <porta>\n", p);

    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    struct main_loop_command commands[] = {
        { "stop", &stop, "termina il peer" }
    };

    int commandNumber = sizeof(commands)/sizeof(struct main_loop_command );

    if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usageHelp(argv[0]);


    unified_io_init();
    main_loop("INPUT", commands, commandNumber);


    printf("Peer terminato correttamente.\n");

    return EXIT_SUCCESS;
}

