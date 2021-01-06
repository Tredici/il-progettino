#include "main_loop.h"
#include "unified_io.h"
#include "commons.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* variabili utili per l'help */
static int commN;
static const struct main_loop_command* commV;

/** Funzione corrispondente
 * al comando di help.
 *
 * Come è ragionevole aspettarsi stampa
 * una semplice descrizione di tutti i
 * comandi forniti.
 */
static int help(const char* args)
{
    int i;
    (void)args;

    /* descrizione di help stesso */
    printf("\t%s:\n\t\t%s\n", "help", "stampa nome e descrizione dei comandi disponibili");
    /* ciclo per la descrizione degli
     * altri comandi */
    for (i = 0; i != commN; ++i)
        printf("\t%s:\n\t\t%s\n", commV[i].command, commV[i].description);

    return OK_CONTINUE;
}

