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
 * +Il primo argomento è la stringa in cui cercare
 *  un comando,
 * +il secondo è un array di
 *  repl_cmd_hint eventualmente terminato da
 *  (struct repl_cmd_hint*) NULL che specifica i
 *  comandi da individuare,
 * +il terzo contiene la dimensione dell'arrray
 *  passato come secondo parametro, oppure -1 se
 *  si vuole lasciare questa non specificata e
 *  affidarsi al NULL terminatore
 *
 * Il controllo del comando è CASE-INSENSITIVE
 */
struct repl_cmd repl_recognise_cmd(const char*, const struct repl_cmd_hint[], int);

/** Specifica il nome di un comando e una
 * funzione da invocare su di esso fornendole
 * la stringa con gli argomenti che seguono
 * questo.
 *
 * La funzione deve restituire 0 in caso di
 * successo e un valore NEGATIVO in caso di
 * errore.
 * Eventuali valori positivi sarebbero
 * nascosti al chiamante di una delle funzioni
 * che usano questa struttura.
 */
struct repl_cmd_todo
{
    char command[REPL_CMD_MAX_L+1];
    int (*fun)(const char*);
};



#endif
