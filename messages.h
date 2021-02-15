/** Questo file si occuperà di raccogliere
 * tutte le funzioni da utilizzare per la
 * creazione, l'invio e la ricezione di
 * messaggi.
 */

#ifndef MESSAGES
#define MESSAGES

#include "ns_host_addr.h"
#include <stdlib.h>
#include "time_utils.h"
#include "register.h"
#include "peer-src/peer_query.h"

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

    /** messaggi per chiedere al server
     * se ci sia un processo peer su
     * una data porta, chi sia e quali
     * siano le informazioni sui suoi
     * vicini */
    MESSAGES_CHECK_REQ,
    MESSAGES_CHECK_ACK,

    /* tipi di messaggio
     * per interagire con
     * gli altri peer */
    /* per la connessione */
    MESSAGES_PEER_HELLO_REQ,
    MESSAGES_PEER_HELLO_ACK, /* può essere usato anche */
    /* messaggio per annunciare il distacco di un peer
     * ai suoi vicini */
    MESSAGES_DETATCH,
    /* per chiedere a un vicino se ha già calcolato una query */
    MESSAGES_REQ_DATA,
    MESSAGES_REPLY_DATA,
    /* per iniziare un rastrellamento di entry */
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

/** Estrae selttivamente in campi richiesti
 * da un oggetto struct peer_data.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 */
int peer_data_extract(const struct peer_data*, uint32_t*, uint16_t*, struct ns_host_addr*);

/** Funzione che permette di estrarre con
 * facilità il solo campo ID di una
 * struttura struct peer_data, restituendolo
 * in host order.
 *
 * Restituisce l'ID in caso di successo
 * e 0 in caso di errore
 */
uint32_t peer_data_extract_ID(const struct peer_data*);

/** Genera una stringa che rappresenta il
 * contenuto di un oggetto struct peer_data.
 * Se non viene fornito un buffer lo spazio
 * richiesto è allocato in memoria dinamica.
 * Se il buffer fornito risulta troppo
 * piccolo fallisce. Il terzo parametro,
 * ignorato se il secondo è NULL, indica la
 * dimensione del biffer preallocato.
 *
 * Restituisce il puntatore al buffer in caso
 * di successo e NULL in caso di errore.
 */
char* peer_data_as_string(const struct peer_data*, char*, size_t);

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
        struct peer_data neighbours[MAX_NEIGHBOUR_NUMBER];
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

/** Formato dei messaggi
 * MESSAGES_BOOT_REQ
 */
struct check_req
{
    /* header */
    struct messages_head head;
    /* body */
    struct check_req_body
    {
        /* query */
        uint16_t port;
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Formato dei messaggi di tipo
 * MESSAGES_BOOT_ACK
 */
struct check_ack
{
    /* header */
    struct messages_head head;
    /* body */
    struct check_ack_body
    {
        /* query */
        uint16_t port;
        /* errore se non 0 */
        uint8_t status;
        /* numero di vicini */
        uint8_t length;
        /* informazioni sul peer legato alla porta */
        struct peer_data peer;
        /* informazioni sui suou vicini */
        struct peer_data neighbours[MAX_NEIGHBOUR_NUMBER];
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Struttura che rappresenta il formato
 * di un messaggio di tipo
 * MESSAGES_FLOOD_FOR_ENTRIES
 *
 * Il corpo del messaggio contiene
 * la data di interesse e l'elenco
 * delle firme delle entry già
 * possedute dal peer che invia
 * il messaggio.
 */
struct flood_req
{
    /* header */
    struct messages_head head;
    /* body */
    struct flood_req_body
    {
        /* id che identifica l'autore
         * per evitare duplicati */
        uint32_t authorID;
        /* id che identifica la richiesta
         * dato l'autore, sempre per
         * evitare duplicati */
        uint32_t reqID;
        /* data di interesse */
        struct ns_tm date;
        /* numero di elementi in coda */
        uint32_t length;
        /* informazioni sui suou vicini */
        uint32_t signatures[0];
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Struttura che rappresenta il formato
 * di un messaggio di tipo
 * MESSAGES_REQ_ENTRIES.
 *
 * Il corpo di un messaggio ha lunghezza
 * variabile e permette di di associarlo
 * al messaggio di tipo
 * MESSAGES_FLOOD_FOR_ENTRIES
 */
struct flood_ack
{
    /* header */
    struct messages_head head;
    /* body */
    struct flood_ack_body
    {
        /* informazioni per identificare
         * univocamente la richiesta */
        uint32_t authorID;
        uint32_t reqID;
        /* per controllo */
        struct ns_tm date;
        /* numero di elementi in coda */
        uint32_t length;
        /* informazioni sui suou vicini */
        struct ns_entry entry[0];
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Struttura che rappresenta il formato dei
 * messaggi ti tipo MESSAGES_REQ_DATA.
 */
struct req_data
{
    /* header */
    struct messages_head head;
    /* body */
    struct req_data_body
    {
        /** Autore iniziale della query
         */
        uint32_t autorID;
        /** Query effettiva
         */
        struct ns_query query;
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Enumerazione per definire
 * delle costanti che indicano
 * lo stato di un messaggio
 * di tipo MESSAGES_REPLY_DATA.
 */
enum messages_reply_data_status
{
    /* Tutto a posto, la risposta è presente */
    MESSAGES_REPLY_DATA_OK,
    /* Il vicino non ha la risposta */
    MESSAGES_REPLY_DATA_NOT_FOUND,
    /* Un errore generico è avvenuto */
    MESSAGES_REPLY_DATA_ERROR
};

/** Struttura che rappresenta il formato dei
 * messaggi ti tipo MESSAGES_REPLY_DATA.
 */
struct reply_data
{
    /* header */
    struct messages_head head;
    /* body */
    struct reply_data_body
    {
        /** flag che indica se la
         * parte successiva è presente
         * o meno e la eventuale causa
         * di errore
         */
        uint32_t status;
        /** Risposta alla query
         */
        struct ns_answer answer;
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Formato dei messaggi di tipo
 * MESSAGES_PEER_HELLO_REQ.
 *
 * Questo tipo di messaggio viene inviato
 * da ogni peer che prova ad aprire una
 * connessione con i suoi vicini "per
 * informarli" della sua comparsa
 */
struct hello_req
{
    /* header */
    struct messages_head head;
    /* body */
    struct hello_req_body
    {
        uint32_t authID; /* ID dell'autore */
        uint32_t destID; /* ID di chi ci si aspetta sia il destinatario */
    } body __attribute__ ((packed));
} __attribute__ ((packed));

enum messages_hello_status
{
    /* tutto regolare - si può proseguire con la fase di attività */
    MESSAGES_HELLO_OK,
    /* non si ha ricevuto una richiesta d */
    MESSAGES_HELLO_TIMEOUT,
    /* segnala all'altro peer che non può accettare altre connessi */
    MESSAGES_HELLO_OVERFLOW,
    /* segnala che il destinatario è sbagliato - cambio di topologia */
    MESSAGES_HELLO_NOT_ME,
    /* seganala che è avvenuto un errore nell'esecuzione del protocollo */
    MESSAGES_HELLO_PROTOCOL_ERROR,
    /* segnala che è avvenuto un errore irrecuperabile */
    MESSAGES_HELLO_ERROR
};

/** Possibili stati inseribili
 * in un messaggio di tipo
 * MESSAGES_DETATCH.
 */
enum detatch_status
{
    /* status do default, non ha alcun
     * significato particolare */
    MESSAGES_DETATCH_OK
};

/** Struttura dei messaggi di tipo
 * MESSAGES_DETATCH.
 *
 * I messsaggi di questo tipo sono
 * pensati per essere gli ultimi che
 * un peer riceve dai suoi vicini.
 * Dopo che un messaggio di questo
 * tipo viene ricevuto il socket
 * associato va considerato come
 * chiuso.
 */
struct message_detatch
{
    /* header */
    struct messages_head head;
    /* body */
    struct message_detatch_body
    {
        uint32_t status;
    } body __attribute__ ((packed));
} __attribute__ ((packed));

/** Formato di un messaggio di tipo
 * MESSAGES_PEER_HELLO_ACK.
 *
 * Il corpo del messaggio contiene
 * un unico campo il cui valore
 * indica se il peer che invia
 * questo messaggio è quello "giusto",
 * ovvero se il mittente è effettivamente
 * un suo vicino nella attuale topologia
 * della rete.
 *
 * Status:
 *  +MESSAGES_HELLO_OK: OK
 *  !MESSAGES_HELLO_ERROR: errore
 */
struct hello_ack
{
    /* header */
    struct messages_head head;
    /* body */
    struct hello_ack_body
    {
        uint32_t status;
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
int messages_make_boot_ack(struct boot_ack**, size_t*, const struct boot_req*, uint32_t, const struct peer_data**, size_t);

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
int messages_get_boot_ack_body(const struct boot_ack*, uint32_t*, struct peer_data**, size_t*);

/** Simile a messages_get_boot_ack_body ma
 * non alloca nulla in memoria dinamica ed
 * evita di creare problemi favorendo la
 * nascita di memory leak.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int messages_get_boot_ack_body_cp(const struct boot_ack*, uint32_t*, struct peer_data*, size_t*);

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

/** Verifica l'integrità di un
 * messaggio del tipo
 * MESSAGES_CHECK_REQ
 */
int messages_check_check_req(const void*, size_t);

/** Costruisce un messaggio di tipo
 * MESSAGES_CHECK_REQ
 */
int messages_make_check_req(struct check_req**, size_t*, uint16_t);

/** Estrae il contenuto di un
 * messaggio di tipo
 * MESSAGES_CHECK_REQ
 *
 * Non effettua alcun controllo
 * di integrità che deve essere
 * già stato svolto precedentemente.
 */
int messages_get_check_req_body(const struct check_req*, uint16_t*);

/** Crea e invia un messaggio di tipo
 * MESSAGES_CHECK_REQ
 */
int messages_send_check_req(int, const struct sockaddr*, socklen_t, uint16_t);

/** Genera un messaggio di tipo
 * MESSAGES_SHUTDOWN_ACK.
 *
 * Se lo status è diverso da 0 i
 * parametri peer,neighbours,length
 * vengono ignorati.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_make_check_ack(
            struct check_ack** buffer,
            size_t* bufLen,
            uint16_t port, /* porta su cui si cerca il peer */
            uint8_t status, /* 0 se tutto ok, 1 se non esiste il peer */
            const struct peer_data* peer, /* informazioni sul peer principale */
            const struct peer_data** neighbours, /* informazioni sui vicini */
            size_t length);

/** Genera e invia all'indirizzo fornito
 * un messaggio di tipo MESSAGES_SHUTDOWN_ACK.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_check_ack(
            int sockfd,
            const struct sockaddr* dest,
            socklen_t destLen,
            uint16_t port,
            uint8_t status,
            const struct peer_data* peer,
            const struct peer_data** neighbours,
            size_t length);

/** Verifica l'integrità di un
 * messaggio del tipo
 * MESSAGES_CHECK_ACK
 */
int messages_check_check_ack(const void*, size_t);

/** Estrae e fornisce lo status
 * presente nel corpo del messaggio.
 *
 * Restituisce lo status del messaggio
 * oppure -1 in caso di errore.
 */
int messages_get_check_ack_status(const struct check_ack*);

/** Permette di ottenere le varie parti
 * del corpo di un messaggio di tipo
 * struct check_ack - le parti non
 * di interesse si possono ignorare
 * fornendo dei puntatori NULL.
 *
 * Se lo status non è 0 i parametri
 * peer,neighbours,length sono
 * ignorati.
 *
 * I puntatori neighbours e length
 * debbono essere coerenti (non può
 * essere solo uno dei due NULL).
 *
 * Non effettua alcun controllo
 * preventivo sull'integrità della
 * struttura.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_get_check_ack_body(
            const struct check_ack* ack,
            uint16_t* port,
            uint8_t* status,
            struct peer_data* peer,
            struct peer_data* neighbours,
            size_t* length);

/** Alloca e fornisce un oggetto di tipo
 * MESSAGES_FLOOD_FOR_ENTRIES
 * inizializzato con i dati forniti.
 * La memoria allocata può essere
 * liberata in maniera sicura con free.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_make_flood_req(
            struct flood_req** req,
            size_t* reqLen,
            uint32_t authID,
            uint32_t reqID,
            const struct tm* date,
            uint32_t length,
            uint32_t* signatures);

/** Crea e invia un messaggio di tipo
 * MESSAGES_FLOOD_FOR_ENTRIES usando
 * un socket TCP (non necessita quindi
 * di informazioni per raggiungere la
 * destinazione).
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_flood_req(
            int sockFd,
            uint32_t authID,
            uint32_t reqID,
            const struct tm* date,
            uint32_t length,
            uint32_t* signatures);

/** Controlla l'integrità di un messaggio
 * di tipo MESSAGES_FLOOD_FOR_ENTRIES.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_check_flood_req(
            const void* buffer,
            size_t bufferLen);

/** Estrae le informazioni richieste
 * da un messaggio di tipo
 * MESSAGES_FLOOD_FOR_ENTRIES.
 * Se l'argomento length è NULL allora
 * deve essere anche l'argomento
 * signatures.
 *
 * Se il messaggio ha length 0 e
 * signatures non è null la variabile
 * puntata viene messa a NULL.
 *
 * ATTENZIONE: non garantisce alcun
 * controllo dell'integrità del
 * messaggio che deve essere già
 * stata verificata in precedenza.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_get_flood_req_body(
            const struct flood_req* req,
            uint32_t* authID,
            uint32_t* reqID,
            struct tm* date,
            uint32_t* length,
            uint32_t** signatures);

/** Poiché alla fine è saltato
 * fuori che i messaggi di tipo
 * MESSAGES_FLOOD_FOR_ENTRIES vanno
 * scambiati tramite socket tcp ho
 * dovuto implementare questa funzione
 * per leggere il corpo di uno di
 * questi messaggi da un socket.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_read_flood_req_body(
            int sockfd,
            uint32_t* authID,
            uint32_t* reqID,
            struct tm* date,
            uint32_t* length,
            uint32_t** signatures);

/** Genera e invia un messaggio di tipo
 * MESSAGES_REQ_ENTRIES in risposta al
 * messaggio di tipo MESSAGES_FLOOD_FOR_ENTRIES
 * fornito.
 *
 * Il messaggio è eventualmente riempito
 * con i dati presenti nel registro fornito
 * o è vuoto se è fornito invece NULL.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_flood_ack(int sockFd,
            const struct flood_req*,
            const struct e_register*,
            const struct set*);

/** Invia un messaggio di tipo MESSAGES_REQ_ENTRIES
 * vuoto.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_empty_flood_ack(
            int sockFd,
            uint32_t authID,
            uint32_t reqID,
            const struct tm* date);

/** Dato che in questa versione del progetto
 * si usano socket tcp per lo scambio di
 * dati trai i peer rirulta necessaria una
 * funzione per leggere il corpo di un
 * messaggio di tipo MESSAGES_REQ_ENTRIES.
 *
 * Per semplicità, se il messaggio ha
 * lunghezza 0 non spreca risorse per
 * generare un registro e lasciarlo vuoto
 * ma associa NULL al puntatore al registro
 * per comunicare al chiamante che il
 * messaggio era vuoto.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_read_flood_ack_body(
            int sockFd,
            uint32_t* authorID,
            uint32_t* reqID,
            struct tm* date, /* può essere NULL */
            const struct e_register** R
            );

/** Inizializza e fornisce un messaggio
 * di tipo MESSAGES_REQ_DATA con l'ID
 * dell'autore e la query indicata.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_make_req_data(
            struct req_data** req,
            size_t* reqLen,
            uint32_t authID,
            const struct query* query);

/** Invia un messaggio di tipo
 * MESSAGES_REQ_DATA tramite il
 * socket fornito.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_req_data(
            int sockfd,
            uint32_t authID,
            const struct query* query);

/** Legge da un socket tcp il corpo di
 * un messaggio di tipo MESSAGES_REQ_DATA
 * e ne fornisce il contenuto.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_read_req_data_body(
            int sockfd,
            uint32_t* authID,
            struct query* query);

/** Invia un messaggio di tipo
 * MESSAGES_PEER_HELLO_REQ
 * tramite il socket fornito.
 *
 * Il messaggio include l'ID, assegnato
 * dal peer, dell'autore e del destinatario.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_hello_req(
            int sockfd,
            uint32_t senderID,
            uint32_t receiverID);

/** Cerca di leggere da un socket TCP
 * il corpo di un messaggio di tipo
 * MESSAGES_PEER_HELLO_REQ e di
 * restituirne il contenuto.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_read_hello_req_body(
            int sockfd,
            uint32_t* senderID,
            uint32_t* receiverID);

/** Genera e invia un messaggio di
 * tipo MESSAGES_PEER_HELLO_ACK.
 *
 * Il parametro status permette di
 * segnalare l'accettazione della
 * connessione o meno.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_hello_ack(
            int sockfd,
            enum messages_hello_status status
            );

/** Cerca di leggere da un socket TCP
 * il corpo di un messaggio di tipo
 * MESSAGES_PEER_HELLO_ACK.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_read_hello_ack_body(
            int sockfd,
            enum messages_hello_status* status
            );

/** Invia un messaggio di tipo
 * MESSAGES_DETATCH attraverso il
 * socket fornito.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_detatch_message(
            int sockfd,
            enum detatch_status status
            );

/** Permette di inviare un messaggio di tipo
 * MESSAGES_REPLY_DATA vuoto, ovvero privo
 * di contenuto oltre lo stato, il quale
 * indicherà necessariamente uno stato di
 * errore.
 *
 * Se il messaggio di errore è del tipo
 * MESSAGES_REPLY_DATA_NOT_FOUND allora
 * il parametro query non può essere NULL
 * e la query viene inserita nel messaggio
 * di risposta.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_empty_reply_data(
            int sockfd,
            enum messages_reply_data_status status,
            struct query* query
            );

/** Invia un messaggio di tipo
 * MESSAGES_REPLY_DATA contenente la
 * risposta alla query fornita.
 *
 * Lo stato del messaggio di risposta
 * è automaticamente posto a
 * MESSAGES_REPLY_DATA_OK.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int messages_send_reply_data_answer(
            int sockfd,
            const struct answer* answer
            );

/** Funzione che si occupa di leggere da un socket
 * il corpo di un messaggio di tipo
 * MESSAGES_REPLY_DATA.
 *
 * Fornisce lo stato presente nel messaggio e, se
 * questo non indica un errore del tipo
 * MESSAGES_REPLY_DATA_ERROR lavora anche sugli
 * argomenti query e answer:
 *  +MESSAGES_REPLY_DATA_NOT_FOUND:
 *      *query != NULL:
 *          dovrà essere poi fornita
 *          a free; rappresenta la
 *          query cui si era richiesta
 *          la risposta
 *      *answer == NULL:
 *          non è stata ricevuta alcuna
 *              risposta
 *
 *  +MESSAGES_REPLY_DATA_OK:
 *      *query == *answer != NULL:
 *          una risposta è stata
 *          estratta dal messaggio
 *          ricevuto, è stato allocato
 *          lo spazio richiesto in
 *          memoria dinamica e si è
 *          pronti a caricarla nella
 *          cache.
 *
 * Restituisce -1 in caso di errore e
 * lo stato della risposta in caso di
 * successo.
 */
int messages_read_reply_data_body(
            int sockfd,
            enum messages_reply_data_status* status,
            struct query** query,
            struct answer** answer
            );

#endif
