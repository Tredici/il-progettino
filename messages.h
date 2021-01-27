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
 * [4 byte che identificano il messaggio,
 * usabile o meno a seconda del tipo di
 * messaggio per il controllo dei duplicati
 * o per qualsiasi altra ragione - in
 * network order]
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

    /* messaggi che il server usa
     * per comandare lo spegnimento
     * dei peer */
    MESSAGES_SHUTDOWN_REQ,
    MESSAGES_SHUTDOWN_ACK,

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
    uint32_t pid; /* pseudo id */
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

/** Questa semplice struttura serve a
 * contenere tutte le informazioni che
 * servono a un peer per contattarne
 * un altro.
 *
 * Tutte le informazioni sono contenute
 * in network order.
 */
struct peer_data
{
    /* id assegnato dal server al peer */
    uint32_t ID;
    /* numero di porta UDP per la relazione d'ordine */
    uint16_t order;
    /* struttura con le informazioni per raggiungere
     * il socket TCP del peer */
    struct ns_host_addr ns_addr;
} __attribute__ ((packed));

/** Inizializza, eventualmente allocando in
 * memoria dinamica, una struttura peer_data.
 *
 * Restituisce il puntatore alla struttura
 * in caso di successo oppure NULL in caso
 * di errore.
 */
struct peer_data* peer_data_init(struct peer_data*, uint32_t, uint16_t, const struct ns_host_addr*);

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

/** Questa struttura definisce il
 * formato dei messaggi inviati
 * dal server ai peer al momento
 * dello shutdown.
 * MESSAGES_SHUTDOWN_REQ
 */
struct shutdown_req
{
    /* header */
    struct messages_head head;
    /* corpo */
    struct shutdown_req_body
    {
        /* id che permette di discernere
         * se il messaggio era destinato
         * a questo peer o a uno risalente
         * a una istanza precedente sempre
         * in ascolto sulla stessa porta */
        uint32_t ID;
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Struttura utilizzata per definire
 * il formato dei messaggi di tipo
 * MESSAGES_SHUTDOWN_ACK
 * da utilizzare in risposta ai messaggi
 * di tipo MESSAGES_SHUTDOWN_REQ.
 *
 * Il formato è molto semplice e
 * copia quello dei messaggi di REQ.
 */
struct shutdown_ack
{
    /* header */
    struct messages_head head;
    /* corpo */
    struct shutdown_ack_body
    {
        /* per controllo da parte del
         * server */
        uint32_t ID;
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Dato il puntatore al buffer che ospita il
 * messaggio fornisce il tipo di questo.
 *
 * Restituisce -1 in caso di errore (e.g.
 * l'argomento è null), altrimenti un valore
 * in enum messages_types.
 */
int recognise_messages_type(const void*);

/** Ottiene una copia del messaggio fornito
 * ottenendo dinamicamente la memoria
 * necessaria.
 *
 * Non effettua alcun controllo sull'integrità
 * del messaggio che deve essere già stato
 * eseguito dal chiamante.
 *
 * Restituisce NULL in caso di errore.
 */
void* messages_clone(const void*);

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
 * Il quarto argomento permette di specificare
 * il pseudo id che va associato alla richiesta
 * di boot.
 *
 * In caso di successo la memoria occupata può
 * essere liberata tranquillamente tramite free.
 *
 * Restituisce -1 in caso di errore, 0 in caso
 * di successo.
 */
int messages_make_boot_req(struct boot_req**, size_t*, int, uint32_t);

/** Verifica l'integrità del messaggo di
 * boot di cui sono forniti inizio e dimensione.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_check_boot_req(const void*, size_t);

/** Invia un messaggio di boot usando il socket
 * specificato come primo argomento, la destinazione
 * indicata come secondo argomento e inviando le
 * informazioni per raggiungere il socket
 * specificato come quarto argomento.
 *
 * Il quinto argomento permette di specificare
 * l'id della richiesta da inviare.
 *
 * L'ultimo argomento, che deve essere il
 * puntatore a un puntatore a una struttura
 * (struct ns_host_addr), oppure NULL,
 * permette di ottenere una copia del
 * dato inviato nel messaggio di boot.
 *
 * Restituisce 0 in caso di successo e
 * -1 in caso di errore.
 */
int messages_send_boot_req(int, const struct sockaddr*, socklen_t, int, uint32_t, struct ns_host_addr**);

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
int messages_check_boot_ack(const void*, size_t);

/** Permette di estrarre in maniera sicura
 * il contenuto di un messaggio di tipo
 * MESSAGES_BOOT_ACK.
 *
 * Il primo argomento indica il messaggio.
 * Il secondo è un puntatore alla variabile
 * dove viene immagazzinato l'ID fornito
 * dal server.
 * Il terzo argomento punta a un array, la
 * cui lunghezza è fornita tramite il
 * quarto argomento, di puntatori a oggetti
 * struct ns_host_addr ricavati dal
 * messaggio.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int messages_get_boot_ack_body(const struct boot_ack*, uint32_t*, struct ns_host_addr**, size_t*);

/** Verifica che il pid del messaggio di
 * tipo MESSAGES_BOOT_ACK sia pare a
 * quello fornito.
 *
 * Restituisce 1 se il pid del messaggio
 * è uguale a quello fornito, 0 se sono
 * diversi e -1 in caso di errore.
 */
int messages_cmp_boot_ack_pid(const struct boot_ack*, uint32_t);

/** Ricava il pseudo id del messaggio
 * fornito.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_get_pid(const void*, uint32_t*);

/** Genera e invia un messaggio di
 * richiesta di shutdown.
 *
 * I primi due argomenti forniscono
 * le informazioni necessarie a inviare
 * la richiesta di shutdown.
 * Il terzo parametro è l'ID del peer
 * che deve spegnersi.
 */
int messages_make_shutdown_req(struct shutdown_req**, size_t*, uint32_t);

/** Verifica l'integrità di un
 * messaggio del tipo
 * MESSAGES_SHUTDOWN_REQ
 */
int messages_check_shutdown_req(const void*, size_t);

/** Genera e invia un messaggio
 * di tipo
 * MESSAGES_SHUTDOWN_REQ
 * all'indirozzo fornito tramite
 * il socket fornito.
 */
int messages_send_shutdown_req(int, const struct sockaddr*, socklen_t, uint32_t);

/** Come messages_send_shutdown_req
 * ma permette di specificare il
 * destinatario tramite l'impiego
 * di un oggetto di tipo
 * struct ns_host_addr* anziché
 * "alla vecchia maniera"
 */
int messages_send_shutdown_req_ns(int, const struct ns_host_addr*, uint32_t);

/** Ottiene l'ID inserito nel corpo
 * di una richiest a di un messaggio
 * di tipo MESSAGES_SHUTDOWN_REQ.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_get_shutdown_req_body(const struct shutdown_req*, uint32_t*);

/** Genera un messaggio di tipo
 * MESSAGES_BOOT_ACK a partire
 * da un messaggio di tipo
 * MESSAGES_BOOT_REQ.
 *
 * La memoria allocata può essere
 * liberata in maniera sicura con
 * una chiamata a free.
 *
 * Attenzione: assume che l'oggeto
 * di tipo struct shutdown_req sia
 * valido (in grado di superare un
 * controllo di integrità) e non si
 * preoccupa di eseguirci controlli
 * sopra.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_make_shutdown_ack(struct shutdown_ack**, size_t*, const struct shutdown_req*);

/** Verifica l'integrità di un
 * messaggio del tipo
 * MESSAGES_SHUTDOWN_ACK
 */
int messages_check_shutdown_ack(const void*, size_t);

/** Ottiene l'ID inserito nel corpo
 * di una richiest a di un messaggio
 * di tipo MESSAGES_SHUTDOWN_ACK.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_get_shutdown_ack_body(const struct shutdown_ack*, uint32_t*);

/** Genera un messaggio di risposta
 * a uno di tipo MESSAGES_BOOT_REQ
 * e lo invia all'indirizzo fornito
 * (che ragionevolmente dovrebbe
 * essere quello da cui è stato
 * ricevuto il messaggio iniziale).
 *
 * Il primo argomento è il descrittore
 * di file che identifica il socket,
 * il secondo e il terzo argomento
 * permettono di identificare il
 * destinatario della risposta/
 * mittente della richiesta e il
 * quarto argomento è il messaggio
 * di richiesta.
 *
 * La funzione non garantisce alcun
 * controllo sull'integrità del
 * messaggio di richiesta e si assume
 * sia invocata sempre e solo dopo
 * gli opportuni controlli.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_shutdown_response(int, const struct sockaddr*, socklen_t, const struct shutdown_req*);

#endif
