/** Questo file si occuperà di raccogliere
 * tutte le funzioni da utilizzare per la
 * creazione, l'invio e la ricezione di
 * messaggi.
 */

#ifndef MESSAGES
#define MESSAGES

/** Enumerazione per identificare
 * tutti i tipi di messaggi che
 * client e server si possono
 * scambiare.
 */
enum messages_types
{
    /* messaggi per l'interazione
     * client server */
    BOOT_REQ,
    BOOT_ACK,

    /* tipi di messaggio
     * per interagire con
     * gli altri peer */
    REQ_DATA,
    REPLY_DATA,
    FLOOD_FOR_ENTRIES,
    REQ_ENTRIES
};

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


#endif
