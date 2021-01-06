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

/** Funzione ausiliaria da invocare
 * quando l'utente inserisce un
 * comando non riconosciuto.
 */
static void cmd404(const char* cmd)
{
    fprintf(stderr, "ERRORE: comando \"%s\" non riconosciuto\n", cmd);
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

int main_loop(const char* msg, const struct main_loop_command* commands, int len)
{
    struct repl_cmd_todo* cmds;
    int cmdsLen;
    int ans;
    /* per specificare la funzione da
     * invocare quando l'utente invoca
     * un comando inesistente */
    extern void (*repl_not_found_f)(const char*);
    /* specifica la funzione da invocare
     * prima di ogni iterazione del ciclo
     * REPL */
    extern void (*repl_repeat)(void);

    /* controlla la validità dei parametri */
    if (commands == NULL || len <= 0)
        return -1;

    /* modo artificioso di passare i parametri
     * alla funzione per gestire l'help */
    commV = commands;
    commN = len;

    /* prova a trasformare i dati nel formato
     * necessario a gestire la meglio il ciclo
     * REPL */
    cmds = makeRCTODOV(commands, len);
    if (cmds == NULL)
    {
        /* è andata male */
        return -1;
    }
    cmdsLen = 1 + len;

    /* specifica la funzione per
     * "COMMAND NOT FOUND" */
    repl_not_found_f = &cmd404;

    /* Specifica che funzione eseguire
     * prima di ogni iterazione del ciclo,
     * ovvero quella che si occuperà
     * di stampare l'output dei thread
     * secondari fino a che l'utente non
     * premerà un tasto qualsiasi */
    repl_repeat = &repeat;

    /* avvia il ciclo REPL */
    ans = repl_start(msg, cmds, cmdsLen);

    /* libera la memoria riservata
     * per tenere i dati sui comandi
     * riconosciuti */
    free(cmds);

    /* restituisce quanto fornito dalla funzione
     * sopra, lo scopo di questa funzione infatti
     * è solo gestire tutte le informazioni di
     * contorno per gestire al meglio l'avvio
     * e l'help in un normale ciclo REPL */
    return ans;
}
