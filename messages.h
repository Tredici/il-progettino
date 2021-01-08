/** Questo file si occuperà di raccogliere
 * tutte le funzioni da utilizzare per la
 * creazione, l'invio e la ricezione di
 * messaggi.
 */

#ifndef MESSAGES
#define MESSAGES


/** La struttura dei messaggi
 * scambiati sarà la seguente:
 * (da leggersi dall'alto verso
 *  il basso, da destra a sinistra)
 * [2 byte a 0 - inizi diversi sono
 * per altri sistemi e.g. debugging]
 * [2 byte contengono un intero
 * rappresentante il tipo del
 * messaggio - in network order]
 * 
 * ++++ la parte successiva è variabile ++++
 * 
 * [ 4 byte contengono un id univoco
 * dell'autore, il numero di porta
 * in questo sistema semplificato 
 * - in network order]
 * [ 4 byte contengono l'id della
 * richiesta, per evitare problemi
 * di duplicazione e confusione - in
 * network order ]
 * [ 4 byte contengono la lunghezza
 * del resto del corpo del messaggio ]
 * 
 */

/** Enumerazione per identificare
 * tutti i tipi di messaggi che
 * client e server si possono
 * scambiare.
 */
enum messages_types
{
    /* per identificare i messaggi
     * che non rispettano il formato
     * atteso di seguito definito */
    MESSAGES_MSG_UNKWN,

    /* messaggi per l'interazione
     * client server */
    MESSAGES_BOOT_REQ,
    MESSAGES_BOOT_ACK,

    /* tipi di messaggio
     * per interagire con
     * gli altri peer */
    MESSAGES_REQ_DATA,
    MESSAGES_REPLY_DATA,
    MESSAGES_FLOOD_FOR_ENTRIES,
    MESSAGES_REQ_ENTRIES
};

/** Dato il puntatore al buffer che ospita il
 * messaggio fornisce il tipo di questo. 
 *
 * Restituisce -1 in caso di errore (e.g.
 * l'argomento è null), altrimenti un valore
 * in enum messages_types.
 */
int recognise_messages_type(void*);

#endif
