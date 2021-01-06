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

enum unified_io_type
{
    UNIFIED_IO_NORMAL,
    UNIFIED_IO_ERROR,
    /* serve per il check,
     * non è un tipo di messaggio
     * valido */
    UNIFIED_IO_LIMIT
};

/** Inizializza tutto il sistema,
 * va chiamata all'inizio dal thread
 * principale prima di eseguire ogni
 * sorta di operazione.
 */
int unified_io_init();

/** Da chiamare alla fine dell'utilizzo.
 * Libera tutte le risorse utilizzate.
 *
 * Restituisce 0 in caso di successo
 * -1 in caso di errore.
 */
int unified_io_close();

/** Aggiunge un messaggio da stampare a video.
 * È possibile specificare se si tratta di un
 * messaggio "normale" o di un "errore"
 *
 * Restituisce 0 in caso di successo, -1 in
 * caso di errore.
 */
int unified_io_push(const char*, enum unified_io_type);

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
