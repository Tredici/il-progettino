#define _POSIX_C_SOURCE 199506L /* per pthread_sigmask */

#include "ds_udp.h"
#include <pthread.h>
#include "ds_peers.h"
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include "../unified_io.h"
#include "../messages.h"
#include <signal.h>
#include <sched.h>
#include <setjmp.h> /* arte */
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

/* segnale utilizzato per terminare il
 * ciclo del server */
#define INTERRUPT_SIGNAL SIGUSR1

/** Id del thead che gestisce il socket
 * UDP
 */
pthread_t UDP_tid;

/** Fino a che questa variabile
 * non sarà azzerata il ciclo del
 * sottosistema UDP
 */
volatile sig_atomic_t UDPloop = 1;

/* serve a gestire la terminazione del
 * ciclo del thread UDP */
sigjmp_buf sigSetJmp; /* bellissimo */
static void sigHandler(int sigNum)
{
    (void)sigNum;
    UDPloop = 0;
    siglongjmp(sigSetJmp, 1); /* magia */
}

/** Funzioni che gestiscono i vari casi.
 * Restituiscono 0 in caso di successo
 * e -1 in caso di errore.
 */
static int handle_MESSAGES_BOOT_REQ(int socketfd, void* buffer, size_t msgLen, struct sockaddr* source, socklen_t sourceLen)
{
    struct boot_req* req;
    struct ns_host_addr ns_source; /* indirizzo "compresso" del mittente */
    struct ns_host_addr* ns_tcp; /* per raggiungere socket TCP */
    char msgStr[32]; /* per stampare il contenuto del messaggio */
    char srcStr[32]; /* per stampare l'indirizzo del mittente */
    char svdStr[32]; /* per stampare ciò che sarà effettivamente memorizzato */
    /* per costruire la risposta */
    uint32_t newID;
    uint16_t length;
    const struct peer_data* neighbours[MAX_NEIGHBOUR_NUMBER];
    int i;
    struct boot_ack* ack;
    size_t ackLen;
    /** Pid associato alla richiesta di boot,
     * per intercettare i duplicati
     */
    uint32_t messagePID;

    unified_io_push(UNIFIED_IO_NORMAL, "Received msg [MESSAGES_BOOT_REQ]");
    /* verifica l'integrità del messaggio */
    if (messages_check_boot_req(buffer, msgLen) == 0)
    {
        req = (struct boot_req*)buffer;
        /* estrae il contenuto del messaggio */
        if (messages_get_boot_req_body(&ns_tcp, req) == -1)
            errExit("*** messages_get_boot_ack_body ***\n");

        /* lo trasforma in stringa */
        if (ns_host_addr_as_string(msgStr, sizeof(msgStr), ns_tcp) == -1)
                    errExit("*** ns_host_addr_as_string ***\n");
        /* ottiene in formato stringa l'indirizzo sorgente */
        if (sockaddr_as_string(srcStr, sizeof(srcStr), source, sourceLen) == -1)
            errExit("*** sockaddr_as_string ***\n");
        /* controlla se l'indirizzo speficificato nel messaggio è 0.0.0.0 o equivalente */
        if (ns_host_addr_any(ns_tcp) == 1)
        {
            if (ns_host_addr_update_addr(ns_tcp, source) == -1)
                errExit("*** ns_host_addr_update_addr ***\n");
        }
        /* rende in formato strinfa anche quello che sarà salvato */
        if (ns_host_addr_as_string(svdStr, sizeof(svdStr), ns_tcp) == -1)
            errExit("*** ns_host_addr_as_string ***\n");

        /* invia l'output al sottosistema di io */
        unified_io_push(UNIFIED_IO_NORMAL,
            "\tmsg:\t%s\n\tsource:\t%s\n\tsaved:\t%s", msgStr, srcStr, svdStr);

        /** Serve il pid del messaggio per intercettare
         * i duplicati
         */
        if (messages_get_pid((void*)req, &messagePID) == -1)
            errExit("*** messages_get_pid ***\n");

        /* ottiene anche l'indirizzo del mittente in un formato "compresso" */
        if (ns_host_addr_from_sockaddr(&ns_source, source) == -1)
            errExit("*** ns_host_addr_from_sockaddr ***\n");

        /** Il controllo dei messaggi duplicati viene eseguito
         * in maniera silente dalla funzione seguente
         */
        if (peers_add_and_find_neighbours(messagePID, &ns_source, ns_tcp, &newID, neighbours, &length) == -1)
            errExit("*** ns_host_addr_as_string ***\n");

        /* stampa i vicini */
        unified_io_push(UNIFIED_IO_NORMAL, "[ID:%d] N. neighbours: %d", newID, length);
        for (i = 0; i != length; ++i)
        {
            if (ns_host_addr_as_string(svdStr, sizeof(svdStr), &(neighbours[i]->ns_addr)) == -1)
                errExit("*** ns_host_addr_as_string ***\n");
            unified_io_push(UNIFIED_IO_NORMAL, "[ID:%d] neighbour (%d): %s", newID, i, svdStr);
        }

        /* ora dovrebbe inviare un messaggio di risposta al peer MESSAGES_BOOT_ACK */
        /* genera il messaggio */
        if (messages_make_boot_ack(&ack, &ackLen, req, newID, neighbours, (size_t)length) == -1)
                errExit("*** messages_make_boot_ack ***\n");

        /* lo invia a destinazione */
        if (sendto(socketfd, (void*)ack, ackLen, 0, source, sourceLen) != (ssize_t)ackLen)
                errExit("*** messages_make_boot_ack ***\n");

        /* infine libera la memoria */
        free((void*)ns_tcp);
    }
    else
    {
        unified_io_push(UNIFIED_IO_ERROR, "\tMalformed message");
    }

    return 0;
}

/** Funzione ausiliaria per la gestione
 * dei messaggi di tipo MESSAGES_CHECK_REQ.
 */
static void
handle_MESSAGES_CHECK_REQ(int socketfd,
            void* buffer, size_t msgLen,
            struct sockaddr* source, socklen_t sourceLen)
{
    /* Porta. */
    uint16_t port;
    /* puntatore tipizzato */
    struct check_req* req;
    /* dati del peer in questione */
    const struct peer_data *peer;
    /* informazioni sui vicini */
    const struct peer_data* neighbours[MAX_NEIGHBOUR_NUMBER];
    uint16_t length;
    /* test */
    int err, i;
    char dataStr[64];

    unified_io_push(UNIFIED_IO_NORMAL, "Received msg [MESSAGES_CHECK_REQ]");
    if (messages_check_check_req(buffer, msgLen) != 0)
    {
        unified_io_push(UNIFIED_IO_ERROR, "\tMalformed message!");
        return;
    }
    req = (struct check_req*)buffer;
    /* estrae il contenuto del messaggio */
    if (messages_get_check_req_body(req, &port) != 0)
        errExit("*** messages_get_check_req_body ***\n");

    unified_io_push(UNIFIED_IO_NORMAL, "\tRichieste informazioni per la porta (%d)", (int)port);
    /* cerca i dati */
    err = peers_get_data_and_neighbours(port, &peer, neighbours, &length);
    if (err)
        unified_io_push(UNIFIED_IO_ERROR, "\tNessun peer trovato!");
    else
    {
        peer_data_as_string(peer, dataStr, sizeof(dataStr));
        unified_io_push(UNIFIED_IO_NORMAL, "\tPeer -> %s", dataStr);
        unified_io_push(UNIFIED_IO_NORMAL, "\tN. neighbours: %d", (int)length);
        /* ora stampa le informazioni su tutti i vicini */
        for (i = 0; i != (int)length; ++i)
        {
            peer_data_as_string(neighbours[i], dataStr, sizeof(dataStr));
            unified_io_push(UNIFIED_IO_NORMAL, "\t\t%d)-> %s", i, dataStr);
        }
    }

    if (messages_send_check_ack(socketfd, source, sourceLen, port, err != 0,
        peer, neighbours, (size_t)length) == -1)
        errExit("*** messages_send_check_ack ***\n");
    unified_io_push(UNIFIED_IO_NORMAL, "\tSent response [MESSAGES_CHECK_ACK]");
}

/** Prova a spegnere tutti al più
 * 5 volte e si pone un tempo di
 * attesa di al più due secondi
 * tra un tentativo e l'altro
 */
#define SHUTDOWN_TIMEOUT 2000 /* ms */
#define SHUTDOWN_ATTEMPT 5

/** Si occupa di inviare a tutti i peer
 * i messaggi che ne comandano lo
 * spegnimento, ovvero messaggi di tipo:
 * MESSAGES_SHUTDOWN_REQ
 */
static void handlerPeersShutdown(int socketfd)
{
    int i;
    struct pollfd polled;
    char buffer[64];
    ssize_t dataLen;
    struct sockaddr_storage ss;
    socklen_t ssLen;
    char sender[32];
    struct shutdown_ack* ack;
    uint32_t ID, ID2;
    uint16_t port;

    memset(&polled, 0, sizeof(struct pollfd));
    polled.fd = socketfd;
    polled.events = POLLIN;

    unified_io_push(UNIFIED_IO_NORMAL, "Connected peers [%d]", (int)peers_number());

    for (i = 0; peers_number()>0 && i < SHUTDOWN_ATTEMPT; i++)
    {
        /* log di quello che sta facendo */
        unified_io_push(UNIFIED_IO_NORMAL, "Sending shutdown message, attempt [%d] of %d", i+1, SHUTDOWN_ATTEMPT);

        /* invia a tutti il segnale di spegnimento */
        if (peers_send_shutdown(socketfd) == -1)
            errExit("*** handlerPeersShutdown:peers_send_shutdown ***\n");

        /* si mette a gestire */
        switch (poll(&polled, 1, SHUTDOWN_TIMEOUT))
        {
        case -1:
            /* DISASTRO */
            errExit("*** handlerPeersShutdown:poll ***\n");
            break;

        case 0:
            /* niente da fare */
            unified_io_push(UNIFIED_IO_NORMAL, "No messages received");
            break;

        default:
            if (!(polled.revents & POLLIN))
            {
                errExit("*** poll: strange response ***\n");
            }

            errno = 0;
            /* per la prima volta uso l'operatore "," */
            while ((dataLen = recvfrom(socketfd, (void*)buffer, sizeof(buffer),
                MSG_DONTWAIT, (struct sockaddr*)&ss, (ssLen = sizeof(ss), &ssLen))) != -1)
            {
                unified_io_push(UNIFIED_IO_NORMAL, "\tReceived message");
                /* nel dubbio ripristina errno
                 * per il controllo dell'errore */
                errno = 0;

                /* verifica che il messaggio ricevuto sia di tipo
                 * MESSAGES_SHUTDOWN_ACK */
                if (recognise_messages_type((void*)buffer) != MESSAGES_SHUTDOWN_ACK)
                {
                    unified_io_push(UNIFIED_IO_NORMAL, "\tInvalid message type!");
                    continue;
                }
                unified_io_push(UNIFIED_IO_NORMAL, "\tReceived message [MESSAGES_SHUTDOWN_ACK]");
                /* controlla l'integrità del messaggio */
                if (messages_check_shutdown_ack((void*)buffer, (size_t)dataLen) == -1)
                {
                    unified_io_push(UNIFIED_IO_NORMAL, "\tMalformed message!");
                    continue;
                }

                ack = (struct shutdown_ack*)buffer;

                if (sockaddr_as_string(sender, sizeof(sender),
                    (struct sockaddr*)&ss, ssLen) == -1)
                    errExit("*** handlerPeersShutdown:sockaddr_as_string ***\n");

                /* estrae l'ID presente nel messaggio ricevuto */
                if (messages_get_shutdown_ack_body(ack, &ID) == -1)
                    errExit("*** handlerPeersShutdown:messages_get_shutdown_ack_body ***\n");

                /* cosa ha ricevuto e da chi */
                unified_io_push(UNIFIED_IO_NORMAL, "\tReceived shutdown ack form %s, ID [%ld]", sender, (long)ID);

                /* estrae la porta di origine, utilizzata
                 * per identificare il mittente */
                if (getSockAddrPort((struct sockaddr*)&ss, &port) == -1)
                    errExit("*** handlerPeersShutdown:messages_get_shutdown_ack_body ***\n");

                /* controlla se sia stato già rimosso
                 * (pacchetto duplicato) e in caso
                 * contrario rimuove il nuovo elemento */
                /* cerca l'ID del peer a quella porta */
                /* vede se coincide con quello atteso */
                if (peers_get_id((long)port, &ID2) == -1 || ID != ID2)
                {
                    unified_io_push(UNIFIED_IO_NORMAL, "\tNo peer with ID [%ld]", (long)ID);
                    continue;
                }
                /* lo elimina */
                if (peers_remove_peer((long)port) == -1)
                    errExit("*** handlerPeersShutdown:peers_remove_peer ***\n");

                unified_io_push(UNIFIED_IO_NORMAL, "\tRemoved peer with ID [%ld]", (long)ID);
            }
            if (errno != EWOULDBLOCK && errno != EAGAIN)
                errExit("*** handlerPeersShutdown:recvfrom ***\n");

            /* ci sono dei messaggi da gestire */
            break;
        }
    }
}

/** Funzione ausiliaria che si occupa di gestire
 * il caso in cui un peer si vuole staccare dal
 * gruppo.
 *
 * Si occupa anche di gestire il caso in cui
 * arriva un messaggio duplicato
 */
static void
handle_peer_DETATCH(
            int sockfd,
            const void* buffer,
            size_t bufferLen,
            struct sockaddr* sender,
            socklen_t senderLen)
{
    uint16_t senderPort; /* porta che ha inviato il messaggio */
    uint32_t senderID; /* ID nel messaggio ricevuto */
    uint32_t foundID; /* ID associato a tale porta nel registro */

    /* parametri validi */
    if (sockfd < 0 || buffer == NULL || bufferLen == 0 || sender == NULL || senderLen == 0)
        errExit("*** UDP:handle_peer_DETATCH ***\n");

    /* integrità dei messaggi */
    if (messages_check_shutdown_req((void*)buffer, bufferLen) != 0)
        return; /* messaggio malformato */

    /* estrae l'ID presente nel messaggio */
    if (messages_get_shutdown_req_body((struct shutdown_req*)buffer, &senderID) != 0)
        errExit("*** UDP:handle_peer_DETATCH ***\n");

    /* se poi il peer effettivamente esisteva elimina il peer dal registro */
    /* estrae la porta da cui è stato inviato il messaggio */
    if (getSockAddrPort(sender, &senderPort) != 0)
        errExit("*** UDP:getSockAddrPort ***\n");

    unified_io_push(UNIFIED_IO_NORMAL, "\tPeer [ID:%ld] on port [%ld] requires disconnection", (long)senderID, (long)senderPort);

    /* vada come vada, risponde al messaggio */
    if (messages_send_shutdown_response(sockfd, sender, senderLen, (struct shutdown_req*)buffer) == -1)
        errExit("*** UDP:messages_send_shutdown_response ***\n");

    /* esiste tale peer? */
    if (peers_get_id((long)senderPort, &foundID) == 0)
    {
        if (foundID != senderID)
        {
            /* elimina quello atteso */
            unified_io_push(UNIFIED_IO_NORMAL, "\tPeer on port [%ld] has ID [%ld] that's NOT [%ld]", (long)senderPort, (long)senderID, (long)foundID);
        }
        else
        {
            /* elimina il dimenticato */
            unified_io_push(UNIFIED_IO_NORMAL, "\tFound peer [ID:%ld] on port [%ld]", (long)senderID, (long)senderPort);
        }
        /* la porta è comunque stata occupata perciò si elimina il mittente lo stesso */
        if (peers_remove_peer((long)senderPort) == -1)
            errExit("*** UDP:peers_remove_peer ***\n");

        unified_io_push(UNIFIED_IO_NORMAL, "\tRemoved peer [ID:%ld] on port [%ld]", (long)foundID, (long)senderPort);
    }
    else
    {
        unified_io_push(UNIFIED_IO_NORMAL, "\tNo peer [ID:%ld] on port [%ld]", (long)senderID, (long)senderPort);
    }
}

/** Corpo del thread che gestisce il
 * socket UDP.
 */
static void* UDP(void* args)
{
    struct thread_semaphore* ts;
    int* data;
    /* porta decisa dal padre */
    int requestedPort;
    /* porta usata dal thread */
    int usedPort;
    /* fd del socket */
    int socket;
    /* variabili per gestire i messaggi in arrivo */
    ssize_t msgLen;
    char buffer[1024];
    struct sockaddr_storage sender;
    socklen_t senderLen;
    char srcStr[32]; /* per stampare l'indirizzo del mittente */
    char timeStr[64]; /* per stampare ts ricezione messaggi */
    time_t currTime;

    /* per il segnale di terminazione */
    struct sigaction toStop;
    sigset_t toBlock; /* per ignorare il segnale */

    ts = thread_semaphore_form_args(args);
    if (ts == NULL)
        errExit("*** UDP ***\n");

    data = (int*)thread_semaphore_get_args(args);
    if (data == NULL)
        errExit("*** UDP ***\n");

    /* attiva il sottosistema per gestire i peer */
    if (peers_init() == -1)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** sigaction ***\n");
        pthread_exit(NULL);
    }

    /* prepara l'handler di terminazione */
    memset(&toStop, 0, sizeof(struct sigaction));
    toStop.sa_handler = &sigHandler;
    if (sigfillset(&toStop.sa_mask) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** sigaction ***\n");

    /* si prepara per bloccare il segnale da usare poi */
    if (sigemptyset(&toBlock) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** sigemptyset ***\n");
    /* inserisce il segnale nella maschera */
    if (sigaddset(&toBlock, INTERRUPT_SIGNAL) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** sigaddset ***\n");
    /* lo blocca per tutto il tempo necessario */
    if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** pthread_sigmask ***\n");

    /* ora imposta l'opportuno handler */
    if (sigaction(INTERRUPT_SIGNAL, &toStop, NULL) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** sigaction ***\n");

    requestedPort = *data;

    /* crea il socket UDP */
    socket = initUDPSocket(requestedPort);
    if (socket == -1)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
    }
    /* verifica la porta */
    usedPort = getSocketPort(socket);
    if (usedPort == -1 || (requestedPort != 0 && requestedPort != usedPort))
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
    }
    /* metodo per fornire al chiamante
     * la porta utilizzata */
    *data = usedPort;

    /* socket creato, porta nota, si può iniziare */
    if (thread_semaphore_signal(ts, 0, NULL) == -1)
        errExit("*** UDP ***\n");

    /* a questo punto il padre può riprendere */
    unified_io_push(UNIFIED_IO_NORMAL, "UDP thread running");

    /* qui va il loop di gestione delle richieste */
    UDPloop = 1;

    /* CHECKPOINT */
    if (sigsetjmp(sigSetJmp, 1) == 0)
    {
        /* ora è tutto pronto e si può sbloccare il segnale */
        if (pthread_sigmask(SIG_UNBLOCK, &toBlock, NULL) != 0)
            errExit("*** pthread_sigmask ***\n");

        while (UDPloop)
        {
            /* ascolta per una richiesta */
            senderLen = sizeof(sender);
            msgLen = recvfrom(socket, (void*)buffer, sizeof(buffer),
                0, (struct sockaddr*)&sender, &senderLen);

            /* gestisce una richiesta alla volta */
            if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
                errExit("*** pthread_sigmask ***\n");

            /* ora quello che viene fatto qui è al sicuro
             * da interruzioni anomale */
            /* controlla che tutto sia andato bene */
            if (msgLen == -1)
                errExit("*** recvfrom ***\n");

            /* orario e mittente del pacchetti */
            if (sockaddr_as_string(srcStr, sizeof(srcStr), (struct sockaddr*)&sender, senderLen) == -1)
                errExit("*** sockaddr_as_string ***\n");
            currTime = time(NULL);
            if (ctime_r(&currTime, timeStr) == NULL)
                errExit("*** ctime_r ***\n");
            timeStr[strlen(timeStr)-1] = '\0';
            unified_io_push(UNIFIED_IO_NORMAL, "[%s\b] packet from %s", timeStr, srcStr);

            /* inizia il lavoro sul pacchetto */
            /* controlla se è regolare */
            switch (recognise_messages_type((void*)buffer))
            {
            case -1:
                /* errore - non dovrebbe mai accadere */
                errExit("*** recognise_messages_type ***\n");
                break;

            case MESSAGES_MSG_UNKWN:
                /* sentinella violata, quasi certamente
                 * è un messaggio di debugging */
                unified_io_push(UNIFIED_IO_NORMAL, "Received msg [MESSAGES_MSG_UNKWN]");
                break;

            case MESSAGES_BOOT_REQ:
                /* ricevuta richiesta di boot da gestire */
                handle_MESSAGES_BOOT_REQ(socket, buffer, msgLen, (struct sockaddr*)&sender, senderLen);
                break;

            case MESSAGES_SHUTDOWN_REQ:
                unified_io_push(UNIFIED_IO_NORMAL, "Received msg [MESSAGES_SHUTDOWN_REQ]");
                /* un peer si vuole staccare, gestisce il caso */
                handle_peer_DETATCH(socket, (void*)buffer, msgLen, (struct sockaddr*)&sender, senderLen);
                break;

            case MESSAGES_CHECK_REQ:
                handle_MESSAGES_CHECK_REQ(socket, (void*)buffer, msgLen, (struct sockaddr*)&sender, senderLen);
                break;

            default:
                /* ricevuto un tipo di messaggio sconosciuto */
                break;
            }

            if (pthread_sigmask(SIG_UNBLOCK, &toBlock, NULL) != 0)
                errExit("*** pthread_sigmask ***\n");
        }
    }
    /* ora il segnale è di nuovo bloccato */
    /* mai raggiunto prima del termine */

    /** svolge tutte le operazioni
     * necessarie allo spegnimento
     * dei peer */
    handlerPeersShutdown(socket);

    if (close(socket) != 0)
        errExit("*** close ***\n");

    /* distrugge gli ultimi registri dei peer */
    if (peers_clear() == -1)
        errExit("*** peers_clear ***\n");

    return NULL;
}

int UDPstart(int port)
{
    int listeningOn;

    listeningOn = port;
    /* si mette in attesa dell'avvio del thread UDP */
    if (start_long_life_thread(&UDP_tid, &UDP, (void*)&listeningOn, NULL) == -1)
        return -1;

    return listeningOn;
}

int UDPstop(void)
{
    /* controlla che fosse partito */
    if (UDP_tid == 0)
        return -1;

    /* invia il segnale di terminazione */
    /* assumo che il thread non termini mai
     * prima del tempo */
    if (pthread_kill(UDP_tid, INTERRUPT_SIGNAL) != 0)
        return -1;

    /* attende la terminazione del thread */
    if (pthread_join(UDP_tid, NULL) != 0)
        return -1;

    return 0;
}
