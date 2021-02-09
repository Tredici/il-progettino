#define _POSIX_C_SOURCE 200112L /* per pselect */

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

/** Flag che indica se il thread tcp
 * era stato avviato con successo.
 */
static volatile int running;

int TCPinit(int port)
{
    int sk;
    int flag;

    /* controlla che il sistema non sia già stato avviato */
    if (activated)
        return -1;

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
    PCS_CLOSED, /* il socket è stato chiuso */
    PCS_ERROR   /* qualcosa è fallito in una operazione sul socket
                 * lo stato è da considerarsi inconsistente */
};

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
    size_t i;
    size_t usedSlots = *reachedNumber; /* numero di slot nell'array di peer occupati */
    time_t currentTime; /* orario di ricezione della nuova richesta di connessione */
    char timeAsString[32] = ""; /* per mettere l'orario in formato stringa */

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
    if (i == MAX_TCP_CONNECTION) /* non c'è più spazio - bisogna rifiutare */
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
        /* azzera lo slot */
        memset(&peers[i], 0, sizeof(peers[i]));
        peers[i].sockfd = newFd; /* salva il fd del socket per dopo */
        peers[i].status = PCS_WAITING; /* segnala che aspetta una richiesta di hello */
        peers[i].creation_time = currentTime; /* segnala l'orario di ricezione */
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

    if (messages_read_hello_req_body(sockfd, &senderID, &maybeME) == -1)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Error reading from socket (%d)", sockfd);
        /* va al gestore dell'error */
        goto onError;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "In HELLO REQ: [senderID:%lu] [maybeME:%lu]", (unsigned long)senderID, (unsigned long)maybeME);

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

    unified_io_push(UNIFIED_IO_NORMAL, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d)", sockfd);
}

/** Funzione ausiliaria per la gestione
 * di messaggi di tipo MESSAGES_REQ_DATA.
 */
static void handle_MESSAGES_REQ_DATA(struct peer_tcp* neighbour)
{
    uint32_t authID; /* in realtà inutile */
    struct query query; /* realmente usato */
    int sockfd = neighbour->sockfd;
    char queryStr[32] = "";
    const struct answer* answer;

    /* parsing del corpo della richiesta */
    if (messages_read_req_data_body(sockfd, &authID, &query) != 0)
    {
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred reading body of [MESSAGES_REQ_DATA] via socket (%d)", sockfd);
        goto onError;
    }
    unified_io_push(UNIFIED_IO_NORMAL, "Received query from socke (%d)", sockfd);
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
    /* cambia lo stato */
    neighbour->status = PCS_ERROR;
    /* chiude il socket */
    unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d)", sockfd);
    if (close(sockfd) != 0)
        unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d)", sockfd);
}

/** Gestisce la ricezione di un messaggio
 * e l'eventuale aggiornamento dello stato
 * da parte di un socket
 */
static void handleNeighbour(struct peer_tcp* neighbour)
{
    /* buffer per contenere "messaggi brevi" */
    char buffer[128];
    /* posizione dalla quale si inizerà a contenere il corpo del messaggio */
    char* body = &buffer[sizeof(struct messages_head)];
    /* evita */
    int sockfd = neighbour->sockfd;
    enum messages_hello_status hello_ack_status;

    unified_io_push(UNIFIED_IO_NORMAL, "Event affected socket (%d)", sockfd);
    switch (readMessageHeader(sockfd, (void*)buffer))
    {
    case -1:
        /* si segna l'errore - il socket è ora invalidato */
        neighbour->status = PCS_ERROR;
        unified_io_push(UNIFIED_IO_ERROR, "Failed read data from socket (%d), maybe EOF reached?", sockfd);
        unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d).", sockfd);
        if (close(sockfd) != 0)
            unified_io_push(UNIFIED_IO_ERROR, "Error occurred while closing socket (%d).", sockfd);
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
        if (messages_read_hello_ack_body(sockfd, &hello_ack_status) != -1)
        {
            /* il socket è andato, ciao! */
            unified_io_push(UNIFIED_IO_ERROR, "Error while reading data from socket (%d)", sockfd);
            unified_io_push(UNIFIED_IO_ERROR, "Closing socket (%d)", sockfd);
            if (close(sockfd) != 0)
                unified_io_push(UNIFIED_IO_ERROR, "Error closing socket (%d)", sockfd);
            return; /* termina la funzione */
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

/** Codice del thread TCP. Sarà attivato al
 * momento della connessione al network.
 */
static void* TCP(void* args)
{
    struct thread_semaphore* ts;
    char buffer[128];
    size_t i, j;
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
            //goto onError;
        }
        /* invia il messaggio di HELLO */
        else if (messages_send_hello_req(connFd, peerID, peer_data_extract_ID(&neighbours[i])) == -1)
        {
            if (connFd != -1) /* gestisce il fallimento della seconda condizione*/
                close(connFd);
            unified_io_push(UNIFIED_IO_ERROR, "Cannot send HELLO REQUEST!");
            //goto onError;
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
            errExit("*** TCP:pselect ***\n");
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
        /* invia il segnale */
        if (pthread_kill(TCP_tid, TERM_SUBSYS_SIGNAL) != 0)
            errExit("*** TCPclose:pthread_kill ***\n");
        /* esegue una join */
        if (pthread_join(TCP_tid, NULL) != 0)
            errExit("*** TCPclose:pthread_join ***\n");
        running = 0;
    }

    if (close(tcpFd) != 0)
        return -1;

    tcpFd = 0;
    activated = 0;
    return 0;
}

int TCPgetSocket(void)
{
    return activated ? tcpFd : -1;
}
