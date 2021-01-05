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



#endif
