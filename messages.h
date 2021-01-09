/** Questo file si occuperà di raccogliere
 * tutte le funzioni da utilizzare per la
 * creazione, l'invio e la ricezione di
 * messaggi.
 */

#ifndef MESSAGES
#define MESSAGES

#include "ns_host_addr.h"
#include <stdlib.h>

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
 * dell'autore, fornito dal server
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

/** Genera un messaggio di boot con i parametri
 * passati.
 *
 * Utilizza gli argomenti per restituire
 * puntatore e dimensione del buffer allocato.
 *
 * Il terzo parametro è il file descriptor del
 * socket su cui ci si aspetta di ricevere i
 * messaggi dai peer.
 *
 * In caso di successo la memoria occupata può
 * essere liberata tranquillamente tramite free.
 *
 * Restituisce -1 in caso di errore, 0 in caso
 * di successo.
 */
int messages_make_boot_req(void**, size_t*, int);

/** Verifica l'integrità del messaggo di
 * boot di cui sono forniti inizio e dimensione.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_check_boot_req(void*, size_t);

/** Invia un messaggio di boot usando il socket
 * specificato come primo argomento, la destinazione
 * indicata come secondo argomento e inviando le
 * informazioni per raggiungere il socket
 * specificato come terzo argomento.
 *
 * Restituisce 0 in caso di successo
 */
int messages_send_boot_req(int, const struct sockaddr*, socklen_t, int);

#endif
