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
        printf("\t%s:\n\t\t%s\n", commV[i].command, commV[i].description != NULL ? commV[i].description : "N.D.");

    return OK_CONTINUE;
}

/** Genera l'array di struct repl_cmd_todo
 * a partire da quello di
 *  struct main_loop_command*
 */
static struct repl_cmd_todo*
makeRCTODOV(const struct main_loop_command* commands,
            int len)
{
    struct repl_cmd_todo* ans;
    int i;

    if (commands == NULL || len < 0)
        return NULL;

    /* il +1 è per lasciare spazio al comando di help */
    ans = calloc(sizeof(struct repl_cmd_todo), len+1);
    if (ans == NULL)
        return NULL;

    for (i = 0; i < len; ++i)
    {
        ans[i].fun = commands[i].fun;
        strncpy(ans[i].command, commands[i].command, REPL_CMD_MAX_L+1);
    }
    /* inserisce il comandi di help */
    ans[i].fun = &help;
    strncpy(ans[i].command, "help", REPL_CMD_MAX_L+1);

    return ans;
}

