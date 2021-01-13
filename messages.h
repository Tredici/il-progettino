/** Questo file si occuperà di raccogliere
 * tutte le funzioni da utilizzare per la
 * creazione, l'invio e la ricezione di
 * messaggi.
 */

#ifndef MESSAGES
#define MESSAGES

#include "ns_host_addr.h"
#include <stdlib.h>

/* in questo progetto minimale */
#define MAX_NEIGHBOUR_NUMBER 2


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


/** Contiene i primi
 * due cambi di un messaggio
 * ben strutturato
 */
struct messages_head
{
    uint16_t sentinel;
    uint16_t type;
} __attribute__ ((packed));

/** Questa struttura definisce
 * il formato di una richiesta
 * di boot
 */
struct boot_req
{
    /* header */
    struct messages_head head;
    /* corpo: contenuto */
    struct ns_host_addr body;
} __attribute__ ((packed));

/** Questa struttura definisce il
 * formato di un messaggio di boot
 * inviato dal server in risposta
 * a un ack
 */
struct boot_ack
{
    /* header */
    struct messages_head head;
    /* corpo della domanda */
    struct boot_ack_body
    {
        /* id che permetterà di identificare
         * il peer nelle comunicazioni future,
         * in questo sistema semplificato sarà
         * dato dal numero di porta */
        uint32_t ID;
        /* numero di vicini, deve valere length<MAX_NEIGHBOUR_NUMBER */
        uint16_t length;
        struct ns_host_addr neighbours[MAX_NEIGHBOUR_NUMBER];
    } body __attribute__ ((packed));
} __attribute__ ((packed));


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
int messages_make_boot_req(struct boot_req**, size_t*, int);

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
 * specificato come quarto argomento.
 *
 * L'ultimo argomento, che deve essere il
 * puntatore a un puntatore a una struttura
 * (struct ns_host_addr), oppure NULL,
 * permette di ottenere una copia del
 * dato inviato nel messaggio di boot.
 *
 * Restituisce 0 in caso di successo
 */
int messages_send_boot_req(int, const struct sockaddr*, socklen_t, int, struct ns_host_addr**);

/** Dato un puntatore a una messaggio di boot
 * fornisce (una copia de) il contenuto della
 * richiesta accessibile mediante il puntatore
 * argomento.
 *
 * La memoria allocata può essere liberata in
 * modo sicuro con free.
 *
 * Restituisce 0 in caso di successo o -1 in
 * caso di errore.
 */
int messages_get_boot_req_body(struct ns_host_addr**, const struct boot_req*);

/** Genera un messaggio di risposta a quello di
 * boot fornito utilizzando le informazioni dei
 * peer dati.
 *
 * I primi due argomenti sono utilizzati per
 * ottenere il messaggio e la sua dimensione
 * per inviarli attraverso un socket.
 *
 * Il terzo argomento è il messaggio a cui si
 * vuole rispondere.
 *
 * Gli ultimi due parametri permettono di
 * accedere all'elenco di neighbour da inviare
 * al peer richiedente. Il penultimo parametro
 * può essere NULL se l'ultimo è 0.
 * Si tratta di un array di puntatori.
 *
 * La memoria allocata e il cui puntatore è
 * fornito mediante il primo argomento può
 * essere eliminata in maniera sicura con
 * una chiamata a free.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 *
 * È pensata per essere usata solo dal server.
 */
int messages_make_boot_ack(struct boot_ack**, size_t*, const struct boot_req*, uint32_t, const struct ns_host_addr**, size_t);

/** Verifica che il messaggio di tipo
 * MESSAGES_BOOT_ACK, di cui vengono
 * forniti il puntatore e la dimensione,
 * sia valido.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_check_boot_ack(void*, size_t);

#endif
