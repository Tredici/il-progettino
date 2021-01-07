/** Funzione che prepara e avvia il main loop
 * (quello che gestisce l'interazione con 
 * l'utente ne thread principale) delle
 * applicazioni client e server.
 */

#ifndef MAIN_LOOP
#define MAIN_LOOP

#include "repl.h"

/** Struttura che contiene nome, gestore
 * e descrizione di un comando riconosciuto
 * dal main loop.
 */
struct main_loop_command
{
    char command[REPL_CMD_MAX_L+1];
    int (*fun)(const char*);
    const char* description;
};


/** Prepara tutto quello che serve per il
 * main loop e lo avvia
 *
 * Il primo argomento è la stringa che sarà
 * mostrata in un ciclo REPL all'inizio di
 * ogni riga di input.
 *
 * Il secondo argomento è il puntatore a un
 * array contenente comandi, gestori e
 * descrizioni degli stessi.
 *
 * Il terzo argomento è la lunghezza
 * dell'array di comandi.
 */
int main_loop(const char*, const struct main_loop_command*, int);

#endif
