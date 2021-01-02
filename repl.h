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
 *  repl_cmd_hint eventualmente terminato da un
 *  elemento il cui campo command inizia con '\0'
 *  ("comando nullo") che specifica i
 *  comandi da individuare,
 * +il terzo contiene la dimensione dell'arrray
 *  passato come secondo parametro, oppure -1 se
 *  si vuole lasciare questa non specificata e
 *  affidarsi al "comando nullo" terminatore
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

/** Funzione ausiliaria per ottenere
 *      struct repl_cmd_hint
 * a partire da
 *      struct repl_cmd_todo
 *
 * Se il secondo argomento non è -1
 * si aspetta esattamente len elementi
 * nel primo array e spazio a
 * sufficenza nel terzo elemento qualora
 * questo non sia NULL.
 *
 * Se questo è NULL len elementi sono
 * allocati in memoria dinamica e
 * vanno poi liberati con una chiamata
 * a free.
 *
 * Il flag negli elementi ritornati è pari
 * alla posizione nell'array originario
 * dell'elemento corrispondente.
 *
 * Ritorna NULL in caso di errore,
 * altrimenti fornisce un puntatore
 * all'array con i risultati, eventualmente
 * coincidente con il terzo parametro fornito.
 *
 * Gli ultimi due parametri non possono essere
 * contemporaneamente -1 e NULL.
 */
struct repl_cmd_hint* repl_make_hint_from_todo(const struct repl_cmd_todo*, int, struct repl_cmd_hint*);

/** Ricava la lunghezza di un array di
 * (struct repl_cmd_hint) terminato da
 * un "elemento nullo" (il cui campo
 * command inizia con '\0'), in caso di
 * errore ritorna -1 (ovvero quando il
 * parametro è esso stesso NULL)
 */
int repl_hints_lenght(struct repl_cmd_hint*);

/** Come repl_hints_lenght ma
 * per gli array di (struct repl_cmd_todo)
 */
int repl_todos_lenght(struct repl_cmd_todo*);

/** Cerca un comando nella stringa fornita e prova
 * ad invocare su esso la funzione associata.
 */
int repl_apply_cmd(const char*, struct repl_cmd_todo*, int);

/** Insieme di costanti utilizzabili per
 * controllare un ciclo repl.
 *
 * Queste costanti sono i valori di riferimento
 * che le funzioni dei comandi dovrebbero ritornare
 * per controllare un ciclo.
 * Corrispondono tutte a valori non negativi poiché
 * l'eventuale restituzione di un valore negativo
 * indica al sistema l'avvento di un errore grave e
 * irrecuperabile che causa l'immediata terminazione
 * del ciclo.
 */
enum repl_code
{
    /* TUTTO OK: prosegui con la prossima iterazione*/
    OK_CONTINUE,
    /* TUTTO OK: il ciclo ha fine */
    OK_TERMINATE,
    /* ATTENZIONE: qualcosa è andato storto ma non è grave */
    WRN_CONTINUE,
    /* ERRORE: parametri invalidi, proseguire */
    ERR_PARAMS,
    /* ERRORE: un errore di libreria ha fatto fallire il comando */
    ERR_FAIL,
    /* ATTENZIONE: comando inesistente,
        da non usare direttamente */
    WRN_CMDNF
};

/** Massimo numero di caratteri che si garantisce
 * di poter gestire in un ciclo repl per riga,
 * eventuali caratteri in eccesso saranno scartati
 */
#define REPL_MAX_LINE 256

/** Implementa un ciclo REPL
 * READ EVALUATE PRINT LOOP
 *
 * In cui continua a chiedere un input
 * da tastiera, cerca un comando in esso
 * e prova a richiamare la funzione
 * associata a questo passandoci la
 * stringa con gli argomenti forniti.
 *
 * La logica seguita per gestire l'array
 * e il parametro sulla lunghezza è la
 * stessa seguita con:
 *      repl_recognise_cmd
 *
 * Il primo parametro, che può essere
 * NULL indica una stringa da stampare
 * ogni volta che si desidera chiedere
 * l'input all'utente.
 */
int repl_start(const char*, struct repl_cmd_todo*, int);


#endif
