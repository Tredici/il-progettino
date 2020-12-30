/** Raccolta di funzioni utili ad eseguire
 * il parsing e il riconoscimento di comandi
 * in un ciclo REPL
 * 
 */

#ifndef REPL
#define REPL

/** Massimo numero di caratteri
 * ('\0' terminale escluso) per
 * identificare un comando
 */
#define REPL_CMD_MAX_L 20

/** Permette di riconoscere un comando
 * e specificare un flag per identificare
 * il comando riconosciuto
 */
struct repl_cmd_hint
{
    char command[REPL_CMD_MAX_L+1];
    int flag;
};

/** Raccoglie le informazioni su un comando
 * riconosciuto:
 *  +puntatore alla stringa analizzata
 *  +puntatore al primo argomento che segue
 *      il comando, o a '\0' se non ve sono
 *  +flag che permette di riconoscere il 
 *      comando facilmente, o -1 se questo
 *      non è stato riconosciuto
 */
struct repl_cmd
{
    const char* command;
    const char* args;
    int flag;
};

/** Riconosce il comando se tra quelli indicati
 * e restituisce una struttura che permette di
 * identificare il comando in questione
 *
 * Il primo argomento è la stringa in cui cercare
 * un comando, il secondo è un array di
 * repl_cmd_hint terminato da
 * (struct repl_cmd_hint*) NULL che specifica i
 * comandi da individuare
 *
 * Il controllo del comando è CASE-INSENSITIVE
 */
struct repl_cmd repl_recognise_cmd(const char*, const struct repl_cmd_hint[]);



#endif
