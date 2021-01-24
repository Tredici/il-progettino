#define _POSIX_C_SOURCE 199506L /* per sigjmp_buf */
#define _XOPEN_SOURCE 500 /* per pthread_sigmask */

#include "main_loop.h"
#include "unified_io.h"
#include "commons.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>

/** Strutture dati ausiliarie per
 * gestire il segnale e la terminazione
 * del main thread tramite un segnale
 */
static sigjmp_buf sigSetJmp; /* per posizionare un checkpoint */
static sigset_t toBlock; /* per bloccare il segnale */
static struct sigaction toTerminate; /* per gestire il restart */

static void handle_TERMINATION_SIGNAL(int sigNum)
{
    (void)sigNum;
    siglongjmp(sigSetJmp, 1); /* riparte dal checkpoint */
}

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
    printf(" %s:\n\t%s\n", "help", "stampa nome e descrizione dei comandi disponibili");
    /* ciclo per la descrizione degli
     * altri comandi */
    for (i = 0; i != commN; ++i)
        printf(" %s:\n\t%s\n", commV[i].command, commV[i].description != NULL ? commV[i].description : "N.D.");

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

/** Funzione ausiliaria che sarà
 * invocata prima di ogni iterazione
 * del ciclo REPL, essa si occupa
 * di stampare l'output prodotto
 * dai thread secondari fino
 * a che l'utente non preme un tasto
 *
 * ATTENZIONE: se questa funzione
 * incontra un errore termina il
 * processo palesando uno stato di
 * errore!
 */
static void repeat(void)
{
    struct termios tp, save;
    int err;

    /* disabilità l'echo per evitare
     * brutti effetti estetici */
    if (tcgetattr(STDIN_FILENO, &tp) == -1)
    {
        errExit("***  void repeat(void) ***\n");
    }

    save = tp;
    tp.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tp) == -1)
    {
        errExit("***  void repeat(void) ***\n");
    }

    /* stampa tutti i messaggi accumulati
     * e pone il sottosistema di IO in uno
     * stato di funzionamento sincrono */
    unified_io_set_mode(UNIFIED_IO_SYNC_MODE);

    /* CHECKPOINT */
    if (sigsetjmp(sigSetJmp, 1) == 0)
    {
        /* sblocca il segnale */
        if (pthread_sigmask(SIG_UNBLOCK, &toBlock, NULL) != 0)
            errExit("*** main_loop:pthread_sigmask ***\n");

        if (waitForInput(0) == -1)
        {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &save);
            errExit("***  void repeat(void) ***\n");
        }
        /* blocca il segnale di nuovo */
        if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
            errExit("*** main_loop:pthread_sigmask ***\n");
    }
    else
    {
        errExit("IL MAIN LOOP FUNZIONA!\n");
    }

    /* ripristina lo stato di funzionamento
     * del sottosistema di IO */
    unified_io_set_mode(UNIFIED_IO_ASYNC_MODE);

    /* ripristina l'echo */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &save) == -1)
    {
        errExit("***  void repeat(void) ***\n");
    }
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

    /* prepara quanto serve per la gestione del segnale */
    /* blocca il segnale */
    if (sigemptyset(&toBlock) != 0) /* leva tutti messaggi */
        errExit("*** main_loop:sigemptyset ***\n");
    if (sigaddset(&toBlock, MAIN_LOOP_TERMINATION_SIGNAL) != 0)
        errExit("*** main_loop:sigaddset ***\n");
    if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
        errExit("*** main_loop:pthread_sigmask ***\n");
    /* imposta l'handler */
    toTerminate.sa_handler = &handle_TERMINATION_SIGNAL;
    /* blocca tutto durante la gestione del segnale */
    if (sigfillset(&toTerminate.sa_mask) != 0)
        errExit("*** main_loop:sigfillset ***\n");
    /* imposta gloablmente l'handler */
    if (sigaction(MAIN_LOOP_TERMINATION_SIGNAL, &toTerminate, NULL) != 0)
        errExit("*** main_loop:sigaction ***\n");

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
