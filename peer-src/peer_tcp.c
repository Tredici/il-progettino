#define _POSIX_C_SOURCE 200112L /* per pselect */
#define _GNU_SOURCE

#include "peer_tcp.h"
#include "peer_entries_manager.h"
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include <pthread.h>
#include "peer_udp.h" /* per UDPcheck */
#include "../messages.h"
#include <sys/types.h>
#include <sys/socket.h>
#include "../unified_io.h"
#include "../commons.h"
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/time.h> /* per gettimeofday */

/** CONVENZIONE SUL COLLEGAMENTO TRA PEER:
 * dovrà essere sempre il peer con ID più
 * altro a connettersi ai vicini con ID
 * più basso. "Chi prima arriva meno fatica."
 */

/** Segnale che sarà utilizzato per
 * richiedere al sottosistema di terminare.
 * È volutamente lo stesso impiegato per
 * il sottosistema ENTRIES.
 */
#define TERM_SUBSYS_SIGNAL SIGQUIT

/** Parametro da usare per la listen
 * sul socket TCP
 */
#define MAX_TCP_CONNECTION 20

/** Numero di secondi dopo i quali un socket con il
 * quale è stato iniziato il processo di apertura
 * di una connessione va scartato se questo non è
 * stato completato
 */
#define STARVATION_TIMEOUT 5

/** Numero di byte che vengono
 * scritti in nella pipe per
 * trasferire un comando
 */
#define CMD_SIZE sizeof(uint8_t)

/** Tempo massimo che si aspetta
 * prima di considerare fallita
 * una richiesta di tipo REQ_DATA
 * inviata ai propri vicini.
 */
#define QUERY_TIMEOUT 5

/** Flag che permette di riconoscere se
 * il thread TCP è già stato avviato.
 */
static int activated;

/** Thread id del thread TCP
 */
static pthread_t TCP_tid;

/** Descrittore di file
 * del socket tcp
 */
static int tcpFd;

/** Enumerazione che elenca tutti i
 * possibili comandi riconosciuti dal
 * thread TCP
 */
enum tcp_commands
{
    /** Il thread deve controllare se
     * i vicini sono cambiati - ovvero
     * se deve cercare di connettersi
     * a qualche altro peer
     */
    TCP_COMMAND_CHECK_PEER,
    /** Il thread deve stampare
     * tutte le informazioni sui
     * socket attivi che controlla
     */
    TCP_COMMAND_INFO,
    /** Bisogna inviare ai vicini un
     * messaggio di tipo MESSAGES_REQ_DATA
     * e mettersi in attesa di una risposta.
     * Un messaggio di questo tipo è sempre
     * seguito a un oggetto di tipo query
     * nella pipe che il thread dovrà
     * prelevare e gestire.
     */
    TCP_COMMAND_QUERY,
    /** Comanda al thread TCP di avviare
     * la procedura di terminazione
     */
    TCP_COMMAND_EXIT
};

/** Pipe per passare in modo semplice dei
 * comandi al thread UDP
 */
static int tcPipe[2];
/* alias di comodo per evitare di fare confusione */
static int tcPipe_readEnd;
static int tcPipe_writeEnd;

/** Flag che indica se il thread tcp
 * era stato avviato con successo.
 */
static volatile int running;

/** Raccolta di strutture dati per gestire
 * l'esecuzione del protocollo REQ_DATA.
 *
 *Descrizione del protocollo:
 * Thread coinvolti:
 *  +thread principale  - master
 *  +thread tcp         - slave
 * Precondizioni (all'inizio e dopo ogni esecuzione):
 *  +REQ_DATAsocket:    vuoto
 *  +REQ_DATAflag:      0
 *  +REQ_DATAquery:     Inaffidabile
 * Terminologia:
 *  +mutex      ->  REQ_DATAmutex
 *  +cond       ->  REQ_DATAcond
 * Funzionamento:
 *  INIZIAZIONE:    (thread principale)
 *      1) il thread principale esegue tutti i controlli del caso
 *          ed abortisce il tutto in caso di errore
 *      2) acquisisce il mutex
 *      3) inizalizza così le strtutture globali:
 *          +REQ_DATAflag:      1
 *          +REQ_DATAquery:     query fornita
 *      4) invia al thread tcp il comando con la query da gestire
 *      5) si mette in attesa sulla variabile di condizione
 *          con timeout QUERY_TIMEOUT
 *
 *  SVOLGIMENTO:    (thread tcp)
 *      1) riceve il comando di esecuzione del protocollo
 *      2) acquisisce il mutex
 *      3) verifica le condizioni - altrimenti rilascia il mutex
 *          e smette di eseguire il protocollo:
 *          +REQ_DATAflag:      1
 *          +REQ_DATAquery:     query ricevuta insieme al comando
 *      4) per ogni socket di connessione tcp nello stato
 *          PCS_READY:
 *          +invia tramite esso un messaggio di tipo
 *              MESSAGES_REQ_DATA
 *          +verifica che l'invio sia andato a buon fine:
 *              a) successo:
 *                  +aggiunge il descrittore di file all'insieme
 *                      REQ_DATAsocket
 *              b) fallimento:
 *                  +va alla prossima iterazione
 *          -se non trova nessun socket segnala la variabile di condizione
 *      5) rilascia il mutex e termina questa fase del protocollo
 *
 *  RICEZIONE: (MESSAGES_REQ_DATA)      (thread tcp | altro processo)
 *      1) Riconosce il mesaggio
 *      2) Cerca una risposta nella cache
 *          a)trovata:      la restituisce la risposta
 *          b)non trovata:  fornisce una risposta vuota
 *
 *  RICEZIONE: (MESSAGES_REPLY_DATA)    (thread tcp)
 *      1) verifica l'integrità del messaggio, altrimenti
 *          abbandona questa fase del protocollo
 *      2) aquisisce il mutex
 *      3) verifica - altrimenti rilascia il mutex e termina
 *          questa fase del protocollo - le seguenti condizioni:
 *          +REQ_DATAflag:      1
 *          +REQ_DATAsocket:    contiene il descrittore sorgente
 *          +REQ_DATAquery:     coincida con quella trovata nel
 *                                  messaggio.
 *      4) Controlla che la risposta abbia un corpo:
 *          a)sì:
 *              +carica la risposta ottenuta nella cache
 *              +imposta:
 *                  -REQ_DATAflag:      0
 *              +segnala la variabile di condizione
 *          b)no:
 *              +rimuove il descrittore del socket dall'insieme
 *                  REQ_DATAsocket
 *              +controlla che REQ_DATAsocket sia non vuoto
 *                  -se vuoto: segnala la variabile di condizione
 *      5) rilascia il mutex e termina questa fase del protocollo
 *
 *  CHIUSURA SOCKET:    (thread tcp)
 *      1) controlla che lo stato del socket sia PCS_READY,
 *          altrimenti termina
 *      2) aquisisce il mutex
 *      3) rimuove, se vi si trova, il descrittore di file
 *          dall'insieme REQ_DATAsocket
 *      4) rilascia il mutex e termina questa fase del protocollo
 *
 *  CONCLUSIONE:    (thread principale)
 *      1) acquisisce il mutex - era bloccato sulla variabile di
 *          condizione
 *      2) verifica il valore di REQ_DATAflag:
 *          +1:
 *              timeout: nessuna risposta è giunta dai vicini.
 *          +0:
 *              successo: un vicino ci ha fornito la risposta!
 *      3) rilascia il mutex e termina il protocollo
 */
static pthread_mutex_t REQ_DATAmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t REQ_DATAcond = PTHREAD_COND_INITIALIZER;
/* insieme dei socket socket coinvolti:
 *  quando si avvia il protocollo si contattano tutti
 *  i vicini (se non ce ne sono termina subito) e si
 *  invia loro */
static struct set* REQ_DATAsocket;
/* se a 1 si sta aspettando */
static volatile sig_atomic_t REQ_DATAflag;
/* query richiesta */
struct query REQ_DATAquery;

/** Funzione ausiliaria che controlla che la
 * query fornita sia al stessa che il thread
 * principale ha inviato.
 * Restituisce 0 in caso contrario, altrimenti
 * un valore non nullo in caso di corrispondenza.
 */
static int REQ_DATAisThisQuery(const struct query* query)
{
    return hashQuery(&REQ_DATAquery) == hashQuery(query);
}

/** Versione sperimentale della
 * funzione, fallisce sempre.
 * Serve a verificare che i vicini
 * ricevano effettivamente il
 * messaggio con la query.
 */
int TCPreqData(const struct query* query)
{
    struct iovec iov[2];
    uint8_t tmpCmd = TCP_COMMAND_QUERY;
    /* man pthread_cond_timedwait per capirne l'utilizzo */
    struct timeval now;
    struct timespec timeout;
    int retcode;
    /* per restituire il valore della funzione */
    int ans = -1; /* di default si aspetta di fallire */

    /* se non è attivo ha senso che si blocchi */
    if (!running)
        fatal("TCPreqData while TCP thread is not running!");

    if (query == NULL)
        return -1;

    /* "header" del comando */
    iov[0].iov_base = (void*)&tmpCmd;
    iov[0].iov_len = CMD_SIZE;
    /* "corpo" del comando */
    iov[1].iov_base = (void*)query;
    iov[1].iov_len = sizeof(*query);

    /* INIZIAZIONE protocollo REQ_DATA */
    if (pthread_mutex_lock(&REQ_DATAmutex) != 0)
        fatal("pthread_mutex_lock");
    /* 3) variabili globali */
    REQ_DATAflag = 1;
    REQ_DATAquery = *query;

    /* 4) scrive tutto nella pipe apposita */
    if (writev(tcPipe_writeEnd, iov, 2) != (ssize_t)iov[0].iov_len + (ssize_t)iov[1].iov_len)
        fatal("writing to command pipe");

    /* 5) attesa */
    if (gettimeofday(&now, NULL) != 0)
        fatal("gettimeofday");
    timeout.tv_sec = now.tv_sec + QUERY_TIMEOUT;
    timeout.tv_nsec = now.tv_usec * 1000;
    retcode = 0;
    while (REQ_DATAflag && retcode != ETIMEDOUT)
    {
         retcode = pthread_cond_timedwait(&REQ_DATAcond,
                                    &REQ_DATAmutex, &timeout);
    }
    /* CONCLUSIONE protocollo REQ_DATA */
    /* 2) controllo dell'esito */
    set_clear(REQ_DATAsocket); /* svuota il set */
    if (REQ_DATAflag)
    {
        /* nessuna risposta - timeout */
        if (retcode != ETIMEDOUT) /* controlla anomalie */
            fatal("pthread_cond_timedwait");
        ans = -1; /* non necassario ma lasciato per leggibilità */
    }
    else /* la risposta è giunta */
    {
        ans = 0;
    }
    /* 3) rilascio e terminazione */
    if (pthread_mutex_unlock(&REQ_DATAmutex) != 0)
        fatal("pthread_mutex_unlock");

    return ans;
}

/** Funzione ausiliaria che chiude il socket e svolge
 * tutto il necessario cleanup e pulizia delle strutture
 * dati globali - va invocata al posto di tutte le
 * close(sockfd) presenti nel codice
 */
static void closeConnection(struct peer_tcp* conn)
{
#pragma GCC warning "TODO: agire sulle strtture dati presenti"
    ;
}

/** Funzione ausiliaria che invia al
 * thread TCP il comando di spegnimento
 * usando la pipe a disposizione.
 */
static void sendShutdownRequest(void)
{
    uint8_t tmpCmd = TCP_COMMAND_EXIT;

    /* controlla che non abbia fatto disastri */
    assert(sizeof(tmpCmd) == CMD_SIZE);

    if (write(tcPipe_writeEnd, &tmpCmd, CMD_SIZE) != CMD_SIZE)
        fatal("writing to command pipe");
}

/** Funzione ausiliaria che invia al
 * thread TCP il comando di verificare
 * il cambio della topologia della rete
 * contattando il server e di procedere
 * di conseguenza.
 */
static void sendCheckRequest(void)
{
    uint8_t tmpCmd = TCP_COMMAND_CHECK_PEER;

    /* controlla che non abbia fatto disastri */
    assert(sizeof(tmpCmd) == CMD_SIZE);

    if (write(tcPipe_writeEnd, &tmpCmd, CMD_SIZE) != CMD_SIZE)
        fatal("writing to command pipe");
}

int TCPinit(int port)
{
    int sk;
    int flag;

    /* controlla che il sistema non sia già stato avviato */
    if (activated)
        return -1;

    /* prepara la struttura per gestire il protocollo
     * REQ_DATA */
    REQ_DATAsocket = set_init(NULL);
    if (REQ_DATAsocket == NULL)
        fatal("REQ_DATAsocket = set_init(NULL)");

    if (pipe2(tcPipe, O_NONBLOCK) != 0)
        errExit("*** pipe! ***\n");

    tcPipe_readEnd = tcPipe[0];
    tcPipe_writeEnd = tcPipe[1];

    sk = initTCPSocket(port, MAX_TCP_CONNECTION);
    if (sk == -1)
        return -1;

    /* lo rende non bloccante */
    flag = fcntl(sk, F_GETFL);
    flag |= O_NONBLOCK;
    if (fcntl(sk, F_SETFL, flag) != 0)
    {
        close(sk);
        return -1;
    }

    tcpFd = sk;
    activated = 1;

    return 0;
}

enum peer_conn_status
{
    PCS_EMPTY = 0,  /* questa struttura struct peer_tcp è inutilizzata */
    PCS_NEW,    /* il socket è nuovo - non si sa se funziona,
                 * è stato creato dal peer corrente,
                 * si aspetta un messaggio di hello_ack! */
    PCS_WAITING,/* il socket è nuovo, è stato ottenuto mediante
                 * una accept(2), si sta aspettando un messaggio
                 * di tipo hello_req */
    PCS_READY,  /* il socket funziona - possiamo inviare messaggi */
    PCS_CLOSED, /* il socket è stato chiuso regolarmente */
    PCS_ERROR   /* qualcosa è fallito in una operazione sul socket
                 * lo stato è da considerarsi inconsistente */
};

/** Funzione ausiliaria che fornisce una stringa
 * rappresentante lo stato della connessione.
 */
__attribute__ ((unused))
const char* statusAsString(enum peer_conn_status status)
{
    switch (status)
    {
    case PCS_EMPTY:
        return "PCS_EMPTY";
    case PCS_NEW:
        return "PCS_NEW";
    case PCS_WAITING:
        return "PCS_WAITING";
    case PCS_READY:
        return "PCS_READY";
    case PCS_CLOSED:
        return "PCS_CLOSED";
    case PCS_ERROR:
        return "PCS_ERROR";
    default:
        return NULL;
    }
}

/** Struttura dati atta a contenere
 * le informazioni e il socket
 * per raggiungere un peer
 */
struct peer_tcp
{
    /* informazioni sul vicino */
    struct peer_data data;
    /* socket per raggiungerlo */
    int sockfd;
    /* stato della connessione */
    enum peer_conn_status status;
    /* istante di creazione - per evitare starvation */
    time_t creation_time;
};

/** Cerca di instaurare una connessione
 * TCP con il peer le cui informazioni
 * sono fornite nell'oggetto peer_data.
 *
 * ATTENZIONE: non effettua alcun controllo
 * sulla relazione d'ordine
 */
static int connectToPeer(const struct peer_data* data)
{
    int sk; /* descrittore del socket */

    if (data == NULL)
        return -1;
    sk = connect_to_ns_host_addr(&data->ns_addr);

    return sk;
}

/** Funzione ausiliaria che si occupa di gestire
 * la disconnessione da un peer.
 * Gestisce tutti i casi possibili.
 */
static int handlePeerDetach(struct peer_tcp* peer)
{
    int sockfd = peer->sockfd;

    unified_io_push(UNIFIED_IO_NORMAL, "Closing connection using socket (%d)", sockfd);
    unified_io_push(UNIFIED_IO_NORMAL, "Connection status: [%s]", statusAsString(peer->status));
    switch (peer->status)
    {
    case PCS_READY:
#pragma GCC warning "Pulire la coda dei messaggi"
        /* messaggio di detatch */
        unified_io_push(UNIFIED_IO_NORMAL, "Sending [MESSAGES_DETATCH] via socket (%d)", sockfd);
        if (messages_send_detatch_message(sockfd, MESSAGES_DETATCH_OK) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Unexpected error while sending [MESSAGES_DETATCH] via socket (%d)", sockfd);
        break;

    case PCS_NEW:
    case PCS_WAITING:
        unified_io_push(UNIFIED_IO_NORMAL, "Sending [MESSAGES_HELLO_NOT_ME] via socket (%d)", sockfd);
        if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_NOT_ME) == -1)
            unified_io_push(UNIFIED_IO_ERROR, "Error occurred while sending [MESSAGES_HELLO_NOT_ME] message!");
        break;

    default:
        fatal("unexpected status: %s", statusAsString(peer->status));
    }
    /* chiusura del socket */
    unified_io_push(UNIFIED_IO_NORMAL, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket!");
    /* azzera lo slot */
    memset(peer, 0, sizeof(*peer));

    return 0;
}

/** Funzione ausiliaria che prova a leggere
 * l'header di un messaggio "regolare".
 * Restituisce -1 in caso di errore
 * (quanto letto, se si è riusciti a
 * leggere) e il tipo del messaggio
 * in caso di successo.
 */
static int readMessageHeader(int sockFd, void* buffer)
{
    if (recv(sockFd, buffer, sizeof(struct messages_head), MSG_DONTWAIT) != (ssize_t)sizeof(struct messages_head))
        return -1;

    /* restituisce il tipo del messaggio */
    return recognise_messages_type(buffer);
}

/** Variabili che permetto al thread di
 * ottenere le informazioni sui suoi
 * vicini */
static uint32_t peerID; /* ID nel netword */
static struct peer_data neighbours[MAX_NEIGHBOUR_NUMBER]; /* dati sui vicini */
static size_t neighboursNumber; /* numero di vicini. */

/** Funzione ausiliaria e per il testing che stampa
 * le informazioni su un elenco di peer.
 */
__attribute__ ((unused))
static void print_neighbours(const struct peer_data neighbours[], size_t length)
{
    size_t i;
    char buffer[64];

    unified_io_push(UNIFIED_IO_NORMAL, "Total neighbours: (%ld)", (long)length);
    for (i = 0; i != length; ++i)
    {
        peer_data_as_string(&neighbours[i], buffer, sizeof(buffer));
        unified_io_push(UNIFIED_IO_NORMAL, "\t%d)-> %s", i, buffer);
    }
}

/** Funzione ausiliaria che ha il compito di ricavare
 * uno slot da utilizzare dall'array di slot totali.
 *
 * Restituisce il puntatore al nuovo slot in caso di
 * successo, crasha in caso di errore.
 */
static struct peer_tcp* findPeerSlot(
            struct peer_tcp peers[],
            size_t* reachedNumber,
            time_t currentTime
            )
{
    struct peer_tcp* ans = NULL;
    struct peer_tcp* tmp = NULL;
    /* numero di slot nell'array di peer occupati */
    size_t i, usedSlots;
    time_t bestTime;

    usedSlots = *reachedNumber;

    /* controlla se ci sia spazio per accettare la connessione */
    for (i = 0; i != usedSlots; ++i)
    {
        switch (peers[i].status)
        {
        case PCS_EMPTY:
        case PCS_CLOSED:
        case PCS_ERROR:
            /* si è trovato lo slot da utilizzare */
            /* basta azzerarlo e siamo a posto */
            memset(&peers[i], 0, sizeof(peers[i]));
            goto endFor; /* deve terminare anche il for */

        case PCS_NEW: /* si stava aspettando il messaggio di hello ack */
        case PCS_WAITING: /* si stava aspettando un messaggio di hello req */
            /* è scaduto un timeout, perciò possiamo recuperare uno slot? */
            if (peers[i].creation_time + STARVATION_TIMEOUT >= currentTime)
                continue; /* il socket non è ancora andato in starvation - va tutto bene */

            /* invia un messaggio di timeout */
            if (messages_send_hello_ack(peers[i].sockfd, MESSAGES_HELLO_TIMEOUT) == -1)
                unified_io_push(UNIFIED_IO_ERROR, "Error occurred while sending timeout message!");
            /* chiude il descrittore di file */
            if (close(peers[i].sockfd) != 0)
                unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket!");
            /* azzera lo slot */
            memset(&peers[i], 0, sizeof(peers[i]));
            goto endFor;
        default:
            /* suppress warnings */
            break;
        }
    }
endFor:
    if (i == MAX_TCP_CONNECTION)
    {
        /* orario più recente */
        bestTime = time(NULL);
        /* cerca uno slot da riciclare - nello stato PCS_WAITING perché
         * sono gli unici che potrebbero essere farlocchi dato che quelli
         * nello stato PCS_NEW sono creati dal peer corrente e ha senso
         * siano affidabili - sceglierà lo slot più nuovo */
        for (i = 0; i != MAX_TCP_CONNECTION; ++i)
        {
            if (peers[i].status == PCS_WAITING)
            {
                if (peers[i].creation_time < bestTime)
                {
                    /* trovato uno più vecchio */
                    bestTime = peers[i].creation_time;
                    tmp = &peers[i];
                }
            }
        }
        ans = tmp;
    }
    else
    {
        if (i == usedSlots) /* richiede un nuovo slot */
        {
            unified_io_push(UNIFIED_IO_NORMAL, "New peer_tcp slot required!");
            ++(*reachedNumber); /* Segna che il numero di slot utilizzati è cresciuto */
        }
        else /* può riciclare uno slot */
        {
            unified_io_push(UNIFIED_IO_NORMAL, "Reuse peer_tcp slot!");
        }
        ans = &peers[i];
    }
    /* inconsistenza per come funziona il sottosistema */
    if (ans == NULL)
        fatal("findPeerSlot: ans == NULL");

    /* azzera lo slot */
    memset(ans, 0, sizeof(*ans));
    return ans;
}

/** Funzione ausiliaria che si occupa di gestire la ricezione
 * e l'eventuale accettazione di una richiesta di connessione
 * da parte di un vicino.
 */
static void acceptPeer(int listenFd,
            struct peer_tcp peers[],
            size_t* reachedNumber)
{
    struct sockaddr_storage ss;
    socklen_t ssLen = sizeof(struct sockaddr_storage);
    int newFd; /* nuovo socket */
    char senderStr[32] = ""; /* indirizzo del mittente in formato stringa */
    time_t currentTime; /* orario di ricezione della nuova richesta di connessione */
    char timeAsString[32] = ""; /* per mettere l'orario in formato stringa */
    struct peer_tcp* slotToUse;

    /* accetterà un socket per volta */
    newFd = accept(listenFd, (struct sockaddr*)&ss, &ssLen);
    if (newFd == -1) /* problema nella listen */
    {
        unified_io_push(UNIFIED_IO_ERROR, "Unexpected error in accepting TCP connection");
        return;
    }
    currentTime = time(NULL); /* segna l'orario di ricezione */
    (void)ctime_r(&currentTime, timeAsString); /* trasforma l'orario in stringa */
    timeAsString[strlen(timeAsString)-1] = '\0'; /* leva il newline terminale */
    /* sembra andare bene */
    if (sockaddr_as_string(senderStr, sizeof(senderStr), (struct sockaddr*)&ss, ssLen) == -1)
        errExit("*** TCP:sockaddr_as_string ***\n");
    unified_io_push(UNIFIED_IO_NORMAL, "Accepted tcp connection at [%s] from %s", timeAsString, senderStr);
    /* in questa versione iniziale non cerca di vedere se c'è un messaggio in arrivo */
    slotToUse = findPeerSlot(peers, reachedNumber, currentTime);
    /* questa nuova versione non fallisce mai, trova sempre uno slot */
    if (slotToUse != NULL) /* dovrebbe essere sempre scelto */
    {
        slotToUse->sockfd = newFd; /* salva il fd del socket per dopo */
        slotToUse->status = PCS_WAITING; /* segnala che aspetta una richiesta di hello */
        slotToUse->creation_time = currentTime; /* segnala l'orario di ricezione */
    }
    else /* non c'è più spazio - bisogna rifiutare */
    {
        unified_io_push(UNIFIED_IO_ERROR, "Reached limit of tcp connections - must refuse!");
        /* invia un messaggio di overflow e chiude */
        unified_io_push(UNIFIED_IO_ERROR, "Sending message with status MESSAGES_HELLO_OVERFLOW");
        if (messages_send_hello_ack(newFd, MESSAGES_HELLO_OVERFLOW) == -1)
        {
            unified_io_push(UNIFIED_IO_ERROR, "Unexpectet error while sending error message");
        }
        unified_io_push(UNIFIED_IO_ERROR, "Connection closed!");
        if (close(newFd) != 0)
            errExit("*** TCP:close ***\n");
    }
}

/** Funzione ausiliaria per gestire i messaggi
 * di tipo
 * MESSAGES_PEER_HELLO_REQ.
 */
static void handle_MESSAGES_PEER_HELLO_REQ(struct peer_tcp* neighbour)
{
    int sockfd = neighbour->sockfd;
    uint32_t senderID; /* ID di chi ha inviato il messaggio */
    uint32_t maybeME; /* ID che dovrebbe essere del peer corrente */
    struct peer_data currentNeighbours[MAX_NEIGHBOUR_NUMBER];
    size_t i, numCurrentNeighbours;

    /* se il socket era in uno stato diverso da
     * "PCS_WAITING" c'è un errore nel protocollo */
    if (neighbour->status != PCS_WAITING)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Error in protocol - wrong state of socket (%d)", sockfd);
        if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_PROTOCOL_ERROR) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Error sending [MESSAGES_PEER_HELLO_ACK] via socket (%d)", sockfd);
        goto onError;
    }

    if (messages_read_hello_req_body(sockfd, &senderID, &maybeME) == -1)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Error reading from socket (%d)", sockfd);
        /* va al gestore dell'error */
        goto onError;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "In HELLO REQ: [senderID:%lu] [maybeME:%lu]", (unsigned long)senderID, (unsigned long)maybeME);
    /* controlla che il suo ID sia corretto */
    if (peerID != maybeME)
    {
        unified_io_push(UNIFIED_IO_ERROR, "I am NOT [maybeME:%lu], I am [PeerID:%ul]", (unsigned long)maybeME, (unsigned long)peerID);
        if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_NOT_ME) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Error sending [MESSAGES_PEER_HELLO_ACK] via socket (%d)", sockfd);
        goto onError;
    }
    /* siamo stati contattati da un più "giovane"? */
    if (peerID > senderID)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Protocolo error: [senderID:%lu] < [PeerID:%lu]", (unsigned long)senderID, (unsigned long)peerID);
        if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_PROTOCOL_ERROR) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Error sending [MESSAGES_PEER_HELLO_ACK] via socket (%d)", sockfd);
        goto onError;
    }
    /* checking current peers */
    unified_io_push(UNIFIED_IO_NORMAL, "Contacting server for check...");
    if (UDPcheck(currentNeighbours, &numCurrentNeighbours) == -1)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Server has NOT responded!");
        /* informa il mittente dell'errore */
        if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_ERROR) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Error sending [MESSAGES_PEER_HELLO_ACK] via socket (%d)", sockfd);
        goto onError;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "Server has responded!");
    /* controlla se nella risposta c'è l'ID del mittente */
    for (i = 0; i != numCurrentNeighbours; ++i)
    {
        /* controlla tutti i vicini */
        if (senderID == peer_data_extract_ID(&currentNeighbours[i]))
        {
            neighbour->data = currentNeighbours[i];
            neighbour->status = PCS_READY;
            goto onSuccess;
        }
    }
    /* non ha trovato il vicino - lo informa dell'errore */
    if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_ERROR) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error sending [MESSAGES_PEER_HELLO_ACK] via socket (%d)", sockfd);

    goto onError;

    /* tutto sembra essere regolare */
onSuccess:
    unified_io_push(UNIFIED_IO_NORMAL, "Connection accepted!");
    if (messages_send_hello_ack(sockfd, MESSAGES_HELLO_OK) != 0)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Error sending [MESSAGES_PEER_HELLO_ACK] via socket (%d)", sockfd);
        /* TODO : se fallisce bisogna ricontrollare i vicini */
#pragma GCC warning "TODO: ricontrollare i vicini in caso di fallimento dell'accettazione di un vicino"
        goto onError;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "Ack sent!");
    /* se arriva qui è andato tutto bene */
    return;
onError:
    neighbour->status = PCS_ERROR;
    unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d)", sockfd);
}

/** Funzione ausiliaria per gestire la ricezione
 * di messaggi di tipo MESSAGES_DETATCH.
 */
static void handle_MESSAGES_DETATCH(struct peer_tcp* neighbour)
{
    int sockfd = neighbour->sockfd;
    /* ATTENZIONE! Potrebbe dover gestire la fase di riconnessione */

    neighbour->status = PCS_CLOSED;
    unified_io_push(UNIFIED_IO_NORMAL, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d)", sockfd);
#pragma GCC warning "Pulire la coda dei messaggi"
    /* ora bisogna controllare se ci sono nuovi vicini */
    sendCheckRequest(); /* "self invoking" command */
}

/** Funzione ausiliaria per la gestione
 * di messaggi di tipo MESSAGES_REQ_DATA.
 */
static void handle_MESSAGES_REQ_DATA(struct peer_tcp* neighbour)
{
    uint32_t authID; /* in realtà inutile */
    struct query query; /* realmente usato */
    int sockfd = neighbour->sockfd;
    char queryStr[48] = "";
    const struct answer* answer;

    /* parsing del corpo della richiesta */
    if (messages_read_req_data_body(sockfd, &authID, &query) != 0)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred reading body of [MESSAGES_REQ_DATA] via socket (%d)", sockfd);
        goto onError;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "Received query from peer [%u] via socket (%d)", authID, sockfd);
    /* controlla la validità della query */
    if (checkQuery(&query) != 0)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Malformed query received!");
        /* invia una risposta con l'errore - il peer corrente non può rispondere */
        if (messages_send_empty_reply_data(sockfd, MESSAGES_REPLY_DATA_ERROR, NULL) != 0)
            goto onSend;

        return;
    }
    /* la trasforma in formato stringa */
    unified_io_push(UNIFIED_IO_NORMAL, "Received query: %s", stringifyQuery(&query, queryStr, sizeof(queryStr)));
    /* ricerca */
    answer = findCachedAnswer(&query);
    /* risultato */
    if (answer == NULL)
    {
        unified_io_push(UNIFIED_IO_NORMAL, "Answer NOT found!");
        if (messages_send_empty_reply_data(sockfd, MESSAGES_REPLY_DATA_NOT_FOUND, &query) != 0)
            goto onSend;
    }
    else
    {
        unified_io_push(UNIFIED_IO_NORMAL, "Answer found!");
        if (messages_send_reply_data_answer(sockfd, answer) == -1)
            goto onSend;
    }

    return;

onSend:
    unified_io_push(UNIFIED_IO_ERROR, "Error occurred while sending [MESSAGES_REPLY_DATA] via socket (%d)", sockfd);
onError:
#pragma GCC warning "Usare una nuova funzione per chiudere i socket"
    /* cambia lo stato */
    neighbour->status = PCS_ERROR;
    /* chiude il socket */
    unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d)", sockfd);
}

/** Gestisce la parte del protocollo REQ_DATA
 * che riguarda handle_MESSAGES_REPLY_DATA
 */
static void
handle_MESSAGES_REPLY_DATA_protocol(
            struct peer_tcp* neighbour,
            struct query* query,
            struct answer* answer)
{
    /* 2) prende il mutex */
    if (pthread_mutex_lock(&REQ_DATAmutex) != 0)
        fatal("pthread_mutex_lock");

    /* 3) controllo delle condizioni */
    if (REQ_DATAflag && set_has(REQ_DATAsocket, neighbour->sockfd) && REQ_DATAisThisQuery(query))
    {
        /* questa risposta era attesa */
        /* rimuove il descrittore dall'insieme */
        set_remove(REQ_DATAsocket, neighbour->sockfd);
        if (answer != NULL)
        {
            /* abbiamo una risposta */
            REQ_DATAflag = 0;
            /* aggiunge la risposta alla cache */
            addAnswerToCache(query, answer);
            /* segnala la variabile di condizione */
            pthread_cond_signal(&REQ_DATAcond);
        }
        else
        {
            if (set_size(REQ_DATAsocket) == 0)
            {
                /* non ci sono altri vicini che potrebbero rispondere */
                pthread_cond_signal(&REQ_DATAcond);
            }
        }
    }
    else
    {
        /* rilascia le risorse */
        free(query);
        /* non serve controllare anche answer dato
         * che se non è NULL è uguale ad answer */
    }

    /* 5) rilascia il mutex */
    if (pthread_mutex_unlock(&REQ_DATAmutex) != 0)
        fatal("pthread_mutex_unlock");
}

/** Funzione ausiliaria per gestire la
 * ricezione di messaggi di tipo
 * MESSAGES_REPLY_DATA,
 */
static void handle_MESSAGES_REPLY_DATA(struct peer_tcp* neighbour)
{
    enum messages_reply_data_status status;
    struct query* query;
    struct answer* answer;
    char buffer[32]; /* per stampare la query ricevuta */

    /* protocollo REQ_DATA - fase RICEZIONE: (MESSAGES_REPLY_DATA) */
    unified_io_push(UNIFIED_IO_NORMAL, "Reading body of [MESSAGES_REPLY_DATA] from socket (%d)...", neighbour->sockfd);

    /* sfrutta il fatto che la funzione restituisca lo
     * stato del messaggio ricevuto oppure -1 in caso
     * di errore */
    switch (messages_read_reply_data_body(
                neighbour->sockfd, &status, &query, &answer)
            )
    {
    case MESSAGES_REPLY_DATA_ERROR:
        /* il mittente ha avuto un problema ma la connessione riman stabile */
        unified_io_push(UNIFIED_IO_ERROR, "Message status: MESSAGES_REPLY_DATA_ERROR");
        break;

    case MESSAGES_REPLY_DATA_NOT_FOUND:
        /* il vicino non ha potuto rispondere */
        unified_io_push(UNIFIED_IO_ERROR, "Message status: MESSAGES_REPLY_DATA_NOT_FOUND");
        if (stringifyQuery(query, buffer, sizeof(buffer)) != NULL)
            fatal("stringifyQuery");

        unified_io_push(UNIFIED_IO_ERROR, "Peer [%u] did not send answer for query: %s",
            peer_data_extract_ID(&neighbour->data), buffer);

        /* gestione dei risultati */
        handle_MESSAGES_REPLY_DATA_protocol(neighbour, query, answer);
        break;

    case MESSAGES_REPLY_DATA_OK:
        /* il vicino ha fornito la risposta */
        unified_io_push(UNIFIED_IO_NORMAL, "Message status: MESSAGES_REPLY_DATA_OK");
        if (stringifyQuery(query, buffer, sizeof(buffer)) != NULL)
            fatal("stringifyQuery");
        unified_io_push(UNIFIED_IO_NORMAL, "Peer [%u] sent answer for query: %s",
            peer_data_extract_ID(&neighbour->data), buffer);

        /* gestione dei risultati */
        handle_MESSAGES_REPLY_DATA_protocol(neighbour, query, answer);
        break;

    case -1:
        /* errore che compromette la connessione */
#pragma GCC warning "Usare una nuova funzione per chiudere i socket"
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while reading MESSAGES_REPLY_DATA body");
        return;

    default:
        fatal("Unknown status: messages_read_reply_data_body");
        break;
    }

}

/** Gestisce la ricezione di un messaggio
 * e l'eventuale aggiornamento dello stato
 * da parte di un socket
 */
static void handleNeighbour(struct peer_tcp* neighbour)
{
    /* buffer per contenere "messaggi brevi" */
    char buffer[128];
    /* evita */
    int sockfd = neighbour->sockfd;
    enum messages_hello_status hello_ack_status;

    unified_io_push(UNIFIED_IO_NORMAL, "Event affected socket (%d)", sockfd);
    switch (readMessageHeader(sockfd, (void*)buffer))
    {
    case -1:
        /* si segna l'errore - il socket è ora invalidato */
#pragma GCC warning "Pulire la coda dei messaggi"
        neighbour->status = PCS_ERROR;
        unified_io_push(UNIFIED_IO_ERROR, "Failed read data from socket (%d), maybe EOF reached?", sockfd);
        unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d).", sockfd);
        if (close(sockfd) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d).", sockfd);
        sendCheckRequest(); /* "self invoking" command */
        break;
    case MESSAGES_PEER_HELLO_REQ:
        /* messaggio di hello da parte di un peer più giovane */
        unified_io_push(UNIFIED_IO_NORMAL, "Received [MESSAGES_PEER_HELLO_REQ] from (%d)", sockfd);
        /* gestisce tutto */
        handle_MESSAGES_PEER_HELLO_REQ(neighbour);
        break;
    case MESSAGES_PEER_HELLO_ACK:
        unified_io_push(UNIFIED_IO_NORMAL, "Received [MESSAGES_PEER_HELLO_ACK] from (%d)", sockfd);
        /* cerca di recuperare lo status */
        if (messages_read_hello_ack_body(sockfd, &hello_ack_status) == -1)
        {
            /* il socket è andato, ciao! */
            unified_io_push(UNIFIED_IO_ERROR, "Error while reading data from socket (%d)", sockfd);
            unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d)", sockfd);
            neighbour->status = PCS_ERROR;
            if (close(sockfd) != 0)
                unified_io_push(UNIFIED_IO_ERROR, "Error closing socket (%d)", sockfd);
        }
        else
        {
            neighbour->status = PCS_READY;
            unified_io_push(UNIFIED_IO_NORMAL, "Successfully connected to socket (%d)", sockfd);
        }
        break;
    case MESSAGES_DETATCH:
        unified_io_push(UNIFIED_IO_NORMAL, "Received [MESSAGES_DETATCH] from (%d)", sockfd);
        handle_MESSAGES_DETATCH(neighbour);
        break;
    case MESSAGES_REQ_DATA:
        unified_io_push(UNIFIED_IO_NORMAL, "Received [MESSAGES_REQ_DATA] from (%d)", sockfd);
        handle_MESSAGES_REQ_DATA(neighbour);
        break;
    case MESSAGES_REPLY_DATA:
        unified_io_push(UNIFIED_IO_NORMAL, "Received [MESSAGES_REPLY_DATA] from (%d)", sockfd);
        handle_MESSAGES_REPLY_DATA(neighbour);
        break;
    }
}

/** Funzione ausiliaria che interroga il server
 * per individuare la presenza di eventuali nuovi
 * vicini ed eventualmente avviare la procedura
 * di connessione nei loro confronti.
 */
static void tryToReachNeighbours(
            struct peer_tcp reachedPeers[],
            size_t* reachedNumber
            )
{
    char buffer[32];
    struct peer_data currentNeighbours[MAX_NEIGHBOUR_NUMBER];
    size_t i, j, numCurrentNeighbours, connNumber;
    u_int32_t otherID;
    struct peer_tcp* newSlot;
    int newSock; /* socket per provare a connettersi */
    time_t currentTime;

    /* numero delle connessioni già presenti */
    connNumber = *reachedNumber;

    unified_io_push(UNIFIED_IO_NORMAL, "Requesting discovery server about neighbours...");
    if (UDPcheck(currentNeighbours, &numCurrentNeighbours) == -1)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Cannot reach discovery server!");
        sendCheckRequest(); /* bisognerebbe mettere un limite ai fallimenti */
        return;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "Discovery server has answered!");
    /* stampa il risultato */
    print_neighbours(currentNeighbours, numCurrentNeighbours);

    /* sono stati ottenuti dei risultati */
    for (i = 0; i != numCurrentNeighbours; ++i)
    {
        peer_data_as_string(&currentNeighbours[i], buffer, sizeof(buffer));
        unified_io_push(UNIFIED_IO_NORMAL, "%d)handling-> %s", i, buffer);
        otherID = peer_data_extract_ID(&currentNeighbours[i]);
        /* ha ID più alto: sarà l'altro a dover iniziare la connessione */
        if (otherID > peerID)
        {
            unified_io_push(UNIFIED_IO_NORMAL, "Skipped because of ID order");
            continue;
        }
        /* controlla di non essere già connesso */
        for (j = 0; j != connNumber; ++j)
        {
            /* questi sono gli unici casi in cui uno slot
             * ha il campo ID valido */
            if (reachedPeers[j].status != PCS_NEW
                && reachedPeers[j].status != PCS_READY)
                continue;
            /* già trovato? */
            if (otherID == peer_data_extract_ID(&reachedPeers[j].data))
                break;
        }
        /* avevamo già gestito questo peer quindi non serve fare altro? */
        if (j < connNumber) /* È stato trovato! Già gestito! */
        {
            unified_io_push(UNIFIED_IO_NORMAL, "Skipped because already handled");
            continue;
        }
        /* questo vicino non è mai stato gestito */
        /* prova a raggiungerlo */
        unified_io_push(UNIFIED_IO_NORMAL, "Try connecting...");
        newSock = connectToPeer(&currentNeighbours[i]);
        /* controlla l'esito */
        if (newSock == -1)
        {
            unified_io_push(UNIFIED_IO_ERROR, "Connection attempt failed!");
            /* accoda un altro tentativo */
            sendCheckRequest();
            continue;
        }
        unified_io_push(UNIFIED_IO_NORMAL, "Successfully connected to %s", buffer);
        /* cerca uno slot */
        currentTime = time(NULL);
        newSlot = findPeerSlot(reachedPeers, reachedNumber, currentTime);
        /* aggiorna con la dimensione */
        connNumber = *reachedNumber;
        /* prova a contattare il candidato vicino */
        newSlot->sockfd = newSock;
        newSlot->status = PCS_NEW;
        newSlot->creation_time = currentTime;
        newSlot->data = currentNeighbours[i];
    }
    /* adesso cerca i precedenti vicini con i quali interrompere la
     * connessione */
    unified_io_push(UNIFIED_IO_NORMAL, "Searching peer to detatch...");
    for (i = 0; i != connNumber; ++i)
    {
        /* il campo ID è valido? */
        if (reachedPeers[i].status != PCS_NEW && reachedPeers[i].status != PCS_READY)
            continue; /* ignora */
        /* controlla l'ID */
        otherID = peer_data_extract_ID(&reachedPeers[i].data);
        /* lo cerca tra i vicini */
        for (j = 0; j != numCurrentNeighbours; ++j)
        {
            if (otherID == peer_data_extract_ID(&currentNeighbours[j]))
                continue; /* trovato - può ignorare */
        }
        /* la connessione va chiusa - gestisce la terminazione */
        /* invia il messaggio di terminazione */
        handlePeerDetach(&reachedPeers[i]);
    }
}

/** Funzione ausiliaria che si occupa di gestire
 * eventuali condizioni eccezionali coinvolgenti
 * i socket tcp che dovrebbe connettere ai vicini
 */
static void handleNeighbourSocketException(struct peer_tcp* neighbour)
{
    int sockfd = neighbour->sockfd;

    unified_io_push(UNIFIED_IO_NORMAL, "Unexpect event on socket (%d)", sockfd);
    unified_io_push(UNIFIED_IO_NORMAL, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error closing socket (%d)", sockfd);
}

/** Funzione ausiliaria che avrà il compito
 * di gestire la ricezione di comandi
 * TCP_COMMAND_QUERY
 */
static void handle_TCP_COMMAND_QUERY(
            int cmdPipe,
            struct peer_tcp reachedPeers[],
            size_t* reachedNumber
            )
{
    /* query che si ha l'ordine di gestire */
    struct query query;
    const size_t qLen = sizeof(struct query);
    /* per stampare la query ricevuta */
    char buffer[64] = "";
    int i, limit;
    int found;

    /* SVOLGIMENTO protocollo REQ_DATA */

    /* legge la query dalla pipe */
    if (read(tcPipe_readEnd, (void*)&query, qLen) != (ssize_t)qLen)
        fatal("Error reading query from pipe");
    /* controllo di integrità */
    if(checkQuery(&query) != 0)
        fatal("Malformed query received by TCP thread");

    unified_io_push(UNIFIED_IO_NORMAL, "Handling query: %s",
        stringifyQuery(&query, buffer, sizeof(buffer)));

    /* 2) acquisizione del mutex */
    if (pthread_mutex_lock(&REQ_DATAmutex) != 0)
        fatal("pthread_mutex_lock");

    /* 3) controlla variabili */
    if (REQ_DATAflag && REQ_DATAisThisQuery(&query))
    {
        /* 4) controlla tutti i socket posseduti */
        unified_io_push(UNIFIED_IO_NORMAL, "REQ_DATA query matched!");
        limit = (int)*reachedNumber;
        found = 0; /* per verificare di avere almeno un invio */
        for (i = 0; i < limit; ++i)
        {
            if (reachedPeers[i].status == PCS_READY)
            {
                unified_io_push(UNIFIED_IO_NORMAL, "Sending query to peer [%u] via socket (%d)",
                    peer_data_extract_ID(&reachedPeers[i].data), reachedPeers[i].sockfd);
                if (messages_send_req_data(reachedPeers[i].sockfd, peerID, &query) == -1)
                {
                    unified_io_push(UNIFIED_IO_ERROR, "No neighbours to ask quesy result!");
#pragma GCC warning "TODO: manca la fase di pulizia!"
                    continue; /* al prossimo socket! */
                }
                /* aggiunge il descrittore a quelli controllati */
                if (set_add(REQ_DATAsocket, reachedPeers[i].sockfd) != 0)
                    fatal("set_add");
                found = 1; /* Ha trovato un vicino! */
            }
        }
        if (!found)
        {
            unified_io_push(UNIFIED_IO_ERROR, "No neighbours to ask query result!");
            pthread_cond_signal(&REQ_DATAcond);
        }
    }
    else
    {
        unified_io_push(UNIFIED_IO_ERROR, "REQ_DATA query mismatch!");
    }
    /* 5) termina fase SVOLGIMENTO */
    if (pthread_mutex_unlock(&REQ_DATAmutex) != 0)
        fatal("pthread_mutex_unlock");
}

/** Funzione autiliaria per la gestione dei
 * comandi provenienti dalla pipe dei comandi.
 *
 * Restituisce -1 quando è richiesta la
 * terminazione del thread TCP e 0 in
 * tutti gli altri casi.
 */
static int handle_pipe_command(
            int cmdPipe,
            struct peer_tcp reachedPeers[],
            size_t* reachedNumber
            )
{
    uint8_t tmpCmd;
    enum tcp_commands cmd;

    /* controlla che non abbia fatto disastri */
    assert(sizeof(tmpCmd) == CMD_SIZE);

    unified_io_push(UNIFIED_IO_NORMAL, "Handling pipe command...");
    /* legge il comando */
    if (read(cmdPipe, &tmpCmd, CMD_SIZE) != CMD_SIZE)
        fatal("reading from command pipe");

    cmd = tmpCmd;
    /* gestisce i comandi ricevuti */
    switch (cmd)
    {
    case TCP_COMMAND_EXIT:
        /* il thread sistema deve terminare */
        unified_io_push(UNIFIED_IO_NORMAL, "Cmd: TCP_COMMAND_EXIT");
        return -1;

    case TCP_COMMAND_CHECK_PEER:
        unified_io_push(UNIFIED_IO_NORMAL, "Cmd: TCP_COMMAND_CHECK_PEER");
        tryToReachNeighbours(reachedPeers, reachedNumber);
        break;

    case TCP_COMMAND_QUERY:
        unified_io_push(UNIFIED_IO_NORMAL, "Cmd: TCP_COMMAND_QUERY");
        handle_TCP_COMMAND_QUERY(cmdPipe, reachedPeers, reachedNumber);
        break;

    default:
        fatal("TCP - unknown command!");
        break;
    }

    return 0;
}

/** Codice del thread TCP. Sarà attivato al
 * momento della connessione al network.
 */
static void* TCP(void* args)
{
    struct thread_semaphore* ts;
    char buffer[128];
    size_t i;
    /* struttura con i peer riconosciuti */
    struct peer_tcp reachedPeers[MAX_TCP_CONNECTION];
    size_t reachedNumber = 0; /* totale peer raggiunti */
    /* per i tentativi di connessione */
    int connFd;
    sigset_t toBlock, originalSigMask;
    /* copia per accettare le connessioni */
    int listeningSocketFd = tcpFd; /* per avere un alias con un nome utile */
    /* per il monitoraggio dei socket */
    int maxFdNumber; /* numero del massimo Fd da controllare */
    fd_set readfd, writefd, exceptionfd; /* fd da controllare */
    int res; /* per non inserire chiamate a funzione dentro if */

    ts = thread_semaphore_form_args(args);
    if (ts == NULL)
        errExit("*** TCP ***\n");

    /* Azzera per questioni di leggibilità. */
    memset(reachedPeers, 0, sizeof(reachedPeers));

    /* firma da assegnare a tutti i messaggi del thread */
    if (unified_io_set_thread_name("TCP") != 0)
        fatal("TCP:unified_io_set_thread_name");

    /* si prepara a bloccare il segnale TERM_SUBSYS_SIGNAL */
    if (sigemptyset(&toBlock) == -1 || sigaddset(&toBlock, TERM_SUBSYS_SIGNAL) == -1
        || pthread_sigmask(SIG_BLOCK, &toBlock, &originalSigMask) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** TCP ***\n");

    unified_io_push(UNIFIED_IO_NORMAL, "Starting thread TCP...");
    unified_io_push(UNIFIED_IO_NORMAL, "Peer ID (%ld)", (long)peerID);
    unified_io_push(UNIFIED_IO_NORMAL, "Num. Neighbours (%ld)", (long)neighboursNumber);
    for (i = 0; i != neighboursNumber; ++i)
    {
        peer_data_as_string(&neighbours[i], buffer, sizeof(buffer));
        unified_io_push(UNIFIED_IO_NORMAL, "%d)-> %s", i, buffer);
        /* prova a raggiungerli */
        unified_io_push(UNIFIED_IO_NORMAL, "Trying to connect to %s", buffer);
        connFd = connectToPeer(&neighbours[i]);
        if (connFd == -1)
        {
            unified_io_push(UNIFIED_IO_ERROR, "Connectiona attempt failed!");
        }
        /* invia il messaggio di HELLO */
        else if (messages_send_hello_req(connFd, peerID, peer_data_extract_ID(&neighbours[i])) == -1)
        {
            if (connFd != -1) /* gestisce il fallimento della seconda condizione*/
                close(connFd);
            unified_io_push(UNIFIED_IO_ERROR, "Cannot send HELLO REQUEST!");
        }
        else
        {
            /* Successo! Aggiorna i dati del thread. */
            unified_io_push(UNIFIED_IO_NORMAL, "Successfully connected!");
            reachedPeers[reachedNumber].data = neighbours[i];
            reachedPeers[reachedNumber].sockfd = connFd;
            reachedPeers[reachedNumber].status = PCS_NEW;
            reachedPeers[reachedNumber].creation_time = time(NULL);
            ++reachedNumber;
        }
    }

    /* Ora è pronto */
    if (thread_semaphore_signal(ts, 0, NULL) == -1)
        errExit("*** TCP ***\n");

    unified_io_push(UNIFIED_IO_NORMAL, "Thread TCP running.");

    /* ciclo infinito a gestione delle connessioni  */
    while (1)
    {
        maxFdNumber = 0;
        /* prepara le liste di fd da controllare */
        FD_ZERO(&readfd); FD_ZERO(&writefd); FD_ZERO(&exceptionfd); /* azzera tutto */
        /* in lettura: listener e vicini */
        FD_SET(listeningSocketFd, &readfd); /* socket da leggere */
        maxFdNumber = max(maxFdNumber, listeningSocketFd);
        /* si pone anche in ascolto della pipe dei comandi */
        FD_SET(tcPipe_readEnd, &readfd);
        maxFdNumber = max(maxFdNumber, tcPipe_readEnd);
        /* vicini - socket per i dati */
        for (i = 0; i != reachedNumber; ++i)
        {
            switch (reachedPeers[i].status)
            {
            case PCS_NEW:
            case PCS_WAITING:
            case PCS_READY:
                /* aggiunge il socket a quelli da cui si possa leggere */
                FD_SET(reachedPeers[i].sockfd, &readfd);
                /* potrebbe capitare di gestire un errore */
                FD_SET(reachedPeers[i].sockfd, &exceptionfd);
                maxFdNumber = max(maxFdNumber, reachedPeers[i].sockfd);
                break;
            default:
                /* non lo considera */
                break;
            }
        }
        /* attesa sui messaggi e gestione della teminazione */
        errno = 0;
        res = pselect(maxFdNumber+1, &readfd, &writefd, &exceptionfd, NULL, &originalSigMask);
        if (res == -1) /* controlla che sia tutto a posto */
        {
            if (errno == EINTR)
                break;
            fatal("*** TCP:pselect ***");
        }
        unified_io_push(UNIFIED_IO_NORMAL, "TCP thread awoken!");
        if (res > 0) /* controlla che ci sia qualcosa ga gestire */
        {
            /* gestisce i vicini */
            for (i = 0; i != reachedNumber; ++i)
            {
                switch (reachedPeers[i].status)
                {
                case PCS_NEW:
                case PCS_WAITING:
                case PCS_READY:
                    /* lettura */
                    if (FD_ISSET(reachedPeers[i].sockfd, &readfd))
                    {
                        unified_io_push(UNIFIED_IO_NORMAL, "Read event on socket (%d)", reachedPeers[i].sockfd);
                        handleNeighbour(&reachedPeers[i]);
                    }
                    /* gestisce eventuali errori */
                    if (FD_ISSET(reachedPeers[i].sockfd, &exceptionfd))
                    {
                        unified_io_push(UNIFIED_IO_NORMAL, "Exceptional event on socket (%d)", reachedPeers[i].sockfd);
                        handleNeighbourSocketException(&reachedPeers[i]);
                    }
                    break;
                default:
                    /* non lo considera */
                    break;
                }
            }
            /* gestisce la ricezione di eventuali comandi */
#pragma GCC warning "Gestire la pipe dei comandi"
            if (FD_ISSET(tcPipe_readEnd, &readfd))
            {
                if (handle_pipe_command(tcPipe_readEnd,
                    reachedPeers, &reachedNumber) == -1)
                    break;
            }
            /* gestisce l'accettazione di nuovi peer */
            if (FD_ISSET(listeningSocketFd, &readfd))
            {
                /* nuova connessione in arrivo */
                acceptPeer(listeningSocketFd, reachedPeers, &reachedNumber);
            }
        }
    }

    unified_io_push(UNIFIED_IO_NORMAL, "Terminating TCP thread...");
    /* chiusura delle connessioni, scambio e ricezione dei dati dei registri */
    for (i = 0; i != reachedNumber; ++i)
    {
        connFd = reachedPeers[i].sockfd;
        switch (reachedPeers[i].status)
        {
        case PCS_NEW:
        case PCS_WAITING:
            /* chiusura di un socket inattivo - non serve fare niente */
            if (close(connFd) != 0)
                unified_io_push(UNIFIED_IO_ERROR, "Unexpected error while closing socket (%d)", connFd);
            break;
        case PCS_READY:
            /* chiusura di un socket già attivo, si avvisa l'altro */
            unified_io_push(UNIFIED_IO_NORMAL, "Sending [MESSAGES_DETATCH] via socket (%d)", connFd);
            if (messages_send_detatch_message(connFd, MESSAGES_DETATCH_OK) != 0)
                unified_io_push(UNIFIED_IO_ERROR, "Unexpected error while sending [MESSAGES_DETATCH] via socket (%d)", connFd);
            unified_io_push(UNIFIED_IO_NORMAL, "Closing socket (%d)", connFd);
            /* in questa versione non si inviano gli ultimi dati ai vicini */
            if (close(connFd) != 0)
                unified_io_push(UNIFIED_IO_ERROR, "Unexpected error while closing socket (%d)", connFd);
            break;
        default:
            /* suppress warning */
            break;
        }
    }
    unified_io_push(UNIFIED_IO_NORMAL, "Terminated TCP thread!");

    return NULL;
}


int TCPrun(uint32_t ID, struct peer_data* initNeighbours, size_t N)
{
    int i;

    if (running)
        return -1;

    if (N > MAX_NEIGHBOUR_NUMBER)
        errExit("*** TCPrun(N > MAX_NEIGHBOUR_NUMBER) ***\n");

    /* rende i dati disponibili al nuovo thread */
    peerID = ID; /* l'ID del peer */
    for (i = 0; i != (int)N; ++i) /* i dati per raggiungere i vicini */
        neighbours[i] = initNeighbours[i];

    neighboursNumber = N; /* il numero iniziale di vicini */

    if (start_long_life_thread(&TCP_tid, &TCP, NULL, NULL) == -1)
        return -1;

    running = 1;
    return 0;
}

int TCPclose(void)
{
    /* controlla se era stato attivato */
    if (!activated)
        return -1;

    if (running)
    {
        /* abbiamo un thread da fermare */
        /* invia il comando di EXIT */
        sendShutdownRequest();
        /* esegue una join */
        if (pthread_join(TCP_tid, NULL) != 0)
            errExit("*** TCPclose:pthread_join ***\n");
        running = 0;
    }

    if (close(tcpFd) != 0)
        return -1;

    /* chiude la pipe usata per mandare comandi al thread */
    if (close(tcPipe[0]) != 0 || close(tcPipe[1]) != 0)
        return -1;

    set_destroy(REQ_DATAsocket);
    REQ_DATAsocket = NULL;

    tcpFd = 0;
    activated = 0;
    return 0;
}

int TCPgetSocket(void)
{
    return activated ? tcpFd : -1;
}
