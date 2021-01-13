/** Funzioni per fare in modo di trasferire
 * l'IO dai thread secondari a quello
 * principale
 *
 * In pratica quello che si fa è usare una
 * coda di messaggi per passare i messaggi
 * da stampare dai thread secondari a quello
 * dedicato all'IO
 */

#ifndef UNIFIED_IO
#define UNIFIED_IO

#define UNIFIED_IO_MAX_MSGLEN 512

enum unified_io_type
{
    UNIFIED_IO_NORMAL,
    UNIFIED_IO_ERROR,
    /* serve per il check,
     * non è un tipo di messaggio
     * valido */
    UNIFIED_IO_LIMIT
};

/** Specifica il comportamento di
 * unified_io_push
 */
enum unified_io_mode
{
    /* accoda il messaggio - default */
    UNIFIED_IO_ASYNC_MODE,
    /* come scrivi output */
    UNIFIED_IO_SYNC_MODE
};

/** Inizializza tutto il sistema,
 * va chiamata all'inizio dal thread
 * principale prima di eseguire ogni
 * sorta di operazione.
 */
int unified_io_init();

/** Imposta il comportamento del
 * sottosistema di I/O, ovvero se
 * la scrittura dell'output deve essere
 * sincrona oppure no.
 *
 * Restituisce 0 in caso di successo e
 * -1 in caso di errore
 */
int unified_io_set_mode(enum unified_io_mode);

/** Ottiene la modalità in cui ora si
 * trova il sottosistema di io.
 */
enum unified_io_mode unified_io_get_mode(void);

/** Da chiamare alla fine dell'utilizzo.
 * Libera tutte le risorse utilizzate.
 *
 * Va usato SOLO quando si è sicuri
 * che nessun thread proverà a scrivere.
 *
 * Restituisce 0 in caso di successo
 * -1 in caso di errore.
 */
int unified_io_close();

/** Aggiunge un messaggio da stampare a video.
 * È possibile specificare se si tratta di un
 * messaggio "normale" o di un "errore".
 * Utilizza la stessa convenzione seguita dalla
 * famiglia delle funzioni printf.
 *
 * Se il messaggio dovesse superare per
 * lunghezza il valore UNIFIED_IO_MAX_MSGLEN
 * (512) la parte in eccesso verrebbe troncata
 * senza alcuna avvertenza.
 *
 * Restituisce 0 in caso di successo, -1 in
 * caso di errore.
 */
int unified_io_push(enum unified_io_type, const char*, ...);

/** Prova a stampare un messaggio in coda.
 *
 * Di default (argomento a 0) si blocca fino
 * a che non diviene disponibile un messaggio
 * da stampare.
 *
 * Restituisce 0 in caso di successo, -1
 * in caso di errore.
 *
 * Qualora non ci sia nessun messaggio da stampare
 * ed è stato richiesto un comportamento
 * NONBLOCKING se non v'è alcun messaggio da
 * stampare restituisce -1 e imposta errno a
 * EWOULDBLOCK
 *
 * Va utilizzato SOLAMENTE nel thread principale.
 */
int unified_io_print(int);

#endif
