#define _XOPEN_SOURCE 500 /* per pthread_kill */
#define _GNU_SOURCE /* per pipe2 */

#include "peer_udp.h"
#include "peer_tcp.h"
#include <pthread.h>
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include "../unified_io.h"
#include "../messages.h"
#include "../main_loop.h"
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/time.h>

/* segnale utilizzato per terminare il
 * ciclo del peer */
#define INTERRUPT_SIGNAL SIGUSR1

/* Una richiesta di boot viene
 * inviata al più 5 volte con un
 * timeout di più 2 secondi prima
 * di inviarne una nuova  */
#define MAX_BOOT_ATTEMPT 5
#define BOOT_TIMEOUT 2000 /* in millisecondi */

/** Tentativi e timeouto per
 * la disconnessione, per semplicità
 */
#define MAX_STOP_ATTEMPT MAX_BOOT_ATTEMPT
#define STOP_TIMEOUT BOOT_TIMEOUT

/** ID del main thread, per gestire
 * lo spegnimento del peer in modo
 * "forzato ma non distruttivo"
 */
pthread_t mainThreadID;

/** Serve a memorizzare la porta sulla
 * quale ascolta il thread UDP
 */
static int UDP_port;

/** Questo mutex e questo flag servono
 * per risolvere il problema che si
 * ha quando sia il main thread sia
 * il thread UDP si inviano a vicenda
 * un segnale di terminazione in
 * particolare si ottiene la seguente
 * sequenza di operazioni:
 *
 * 1) thread UDP invia un segnale al
 *      main thread e termina
 * 2) il main thread inizia ad
 *      eseguire il comando di stop
 * 3) il main thread invia un segnale
 *      al thread UDP per farlo
 *      terminare ma fallisce
 * 3) BUM!
 *
 * In particolare questo mutex
 * serve a invocare la primitiva
 * per scambiare messaggi tra i
 * thread in maniera atomica.
 */
static pthread_mutex_t termUDPguard = PTHREAD_MUTEX_INITIALIZER;
static int termUDPflag; /* messo a 1 da chi prende il mutex per primo */

/** Insieme di variabili che permettono
 * di ricordare se il peer è attualmente
 * connesso a una rete e quale sia il suo
 * ID nella rete.
 */
static volatile uint32_t peerID; /* ID del peer nella rete */
static volatile int ISPeerConnected; /* flag che indica la validità dell'altro parametro */
static pthread_mutex_t IDguard = PTHREAD_MUTEX_INITIALIZER;

/** Variabili globali che tengono
 * le informazioni per raggiungere
 * il discovery server per le
 * operazioni di shutdown
 */
static struct sockaddr_storage DSaddr;
static socklen_t DSaddrLen;

/** Variabili che servono a garantire la corretta
 * temporizzazione nell'utilizzo di UDPcheck.
 *
 * Protocollo:
 *  All'inizio di ogni giro:
 *      CHECKnumber=0
 *      CHECKflag=0
 *  1) il chiamante:
 *      prende il mutex
 *      invia un messaggio
 *      imposta CHECKflag a 1
 *      si mette in attesa sulla cond.var. per al più 2s
 *  2) il thread UDP (eventualmente):
 *      riceve la risposta
 *      controlla che sia corretta
 *          altrimenti lascia perdere
 *      cattura il mutex
 *      verifica CHECKflag!=0 && CHECKnumber==0
 *          altrimenti rilascia il mutex senza far nulla
 *      riempie CHECKneighbours e imposta opportunamente CHECKnumber
 *      spegne il CHECKflag
 *      segnala la cond.var.
 *  3) il chiamante:
 *      acquisisce il mutex
 *      controlla che non sia passato il timeout
 *      verifica CHECKflag==0
 *          altrimenti pone CHECKflag=0, rilascia il mutex e termina
 *      utilizza i dati in CHECKneighbours e CHECKflag per rispondere
 *      rilascia il mutex e termina
 */
static pthread_mutex_t CHECKguard = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t CHECKcond = PTHREAD_COND_INITIALIZER;
static struct peer_data CHECKneighbours[MAX_NEIGHBOUR_NUMBER];
static size_t CHECKnumber; /* se non 0 non bisogna scrivere */
static int CHECKflag; /* se c'è un thread in attesa */

/** Funzione ausiliaria che collabora con
 * UDPcheck per la ricezione delle informazioni
 * sui vicini attuali del peer.
 */
static void
handle_MESSAGES_CHECK_ACK(
            int socketfd,
            const void* buffer,
            size_t bufLen)
{
    const struct check_ack* ack;
    uint16_t port;
    uint8_t status;
    struct peer_data peer; /* infomazioni sul peer */
    /* vicini */
    struct peer_data neighbours[MAX_NEIGHBOUR_NUMBER];
    size_t length;
    int i;
    char neigStr[128];

    (void)socketfd; /* sopprime il warning */

    /* controlla l'integrità del messaggio */
    if (messages_check_check_ack(buffer, (size_t)bufLen) != 0)
    {
        unified_io_push(UNIFIED_IO_ERROR, "\tMalformed Message!");
        return;
    }
    /* casta di controllo */
    ack = (const struct check_ack*)buffer;

    if (messages_get_check_ack_body(ack, &port, &status,
        &peer, neighbours, &length) != 0)
        errExit("*** UDP:messages_get_check_ack_body ***\n");

    /* controlla se le porta è diversa da quella del peer */
    if ((int)port != UDP_port)
    {
        /* questo messaggio non era per noi */
        unified_io_push(UNIFIED_IO_ERROR,  "\tUnexpected message,"
            " specified port is (%d) instead of (%d)! Discarded.",
            (int)port, UDP_port);
        return;
    }
    /* INIZIO SEZIONE CRITICA */
    if (pthread_mutex_lock(&CHECKguard) != 0)
        abort();
    /* controlla che fosse richiesto di operare */
    if (CHECKflag && CHECKnumber == 0)
    {
        /* controlla lo status */
        if (status) /* caso di errore */
        {
            unified_io_push(UNIFIED_IO_ERROR, "\tBad response status!");
            /* segnala senza spegnere il flag: */
            pthread_cond_signal(&CHECKcond);
        }
        else
        {
            /* quanti peer ha trovato */
            unified_io_push(UNIFIED_IO_NORMAL, "\tTrovati (%d) neighbour", (int)length);
            /* copia i vicini */
            for (i = 0; i != (int)length; ++i)
            {
                peer_data_as_string(&neighbours[i], neigStr, sizeof(neigStr));
                unified_io_push(UNIFIED_IO_NORMAL, "\t%d)-> %s", (int)i, neigStr);
                CHECKneighbours[i] = neighbours[i];
            }
            /* segnala */
            pthread_cond_signal(&CHECKcond);
            CHECKnumber = length; /* numero di vicini */
            CHECKflag = 0; /* spegne il flag, richiesta servita */
        }
    }
    /* TERMINE SEZIONE CRITICA */
    if (pthread_mutex_unlock(&CHECKguard) != 0)
        abort();
}

/** APPROCCIO SEGUITO:
 * un thread PASSIVO che si occupa
 * ESCLUSIVAMENTE di vagliare i messaggi
 * in ARRIVO e di inoltrarli alle funzioni
 * che li debbono gestire
 *
 * più funzioni secondarie che hanno
 * esclusivamente un ruolo ATTIVO, dopodiché,
 * se la specifica lo richiede, si mettono in
 * attesa di una risposta tramite
 * l'interfaccia fornitagli dal thread
 * PASSIVO.
 *
 * PROBLEMA: in alcuni casi è necessario
 * implementare un timeout e multithreading e
 * segnali (la via più naturale per
 * implementare un timeout tramite alarm) non
 * lavorano bene insieme. alarm infatti non
 * permette di specificare il thread a cui
 * inviare il segnale (e non garantisce sia
 * quello che ha invocata), inoltre non ho
 * fiducia nell'interrompere un tentativo
 * di lock su un socket tramite un segnale.
 *
 * Ho bisogno quindi di metodi sicuri, offerti
 * da sistema operativo che offrono quindi
 * garanzia di robustezza e di mutua esclusione.
 * In particolare intendo utilizzare PIPES
 * (unidirezionali, a cui accede in scrittura il
 * thread passivo e il lettura la funzione
 * apposita). Farò eventualmente impego di
 * mutex e simili per evitare problemi dovuti
 * alla ricezione di risposte a messaggi mai
 * inviati.
 *
 * Per utilizzare un meccanismo di TIMEOUT farò
 * utilizzo di timerfd_create che, non facendo
 * uso di segnali, risulta quindi thread safe.
 */

/** Per lo scambio e la gestione dei messaggi
 * di boot da parte del server sarà utilizzato
 * un protocollo produttore consumatore
 * usando le seguenti strutture dati per
 * lavorare in mutua esclusione.
 */
static pthread_mutex_t BOOTguard = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t BOOTcond = PTHREAD_COND_INITIALIZER;
static volatile int BOOTflag; /* se a uno il pid è valido */
static volatile uint32_t BOOTpid;
static volatile struct boot_ack* BOOTack;
//int startPipe[2];

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
sigset_t toBlock; /* per ignorare il segnale */
sigjmp_buf sigSetJmp; /* bellissimo */
static void sigHandler(int sigNum)
{
    (void)sigNum;
    UDPloop = 0;
    siglongjmp(sigSetJmp, 1); /* magia */
}

/* fd del socket
 * è globale poiché deve essere
 * potenzialmente condiviso da
 * più thread */
int socketfd;

/** Funzione ausiliaria che si occupa
 * di gestire la ricezione di un messaggio
 * di tipo MESSAGES_SHUTDOWN_REQ.
 *
 * Restituisce 0 se bisogna avviare la
 * procedura di spegnimento oppure -1
 * se il messaggio ricevuto era malformato.
 */
static int
handle_MESSAGES_SHUTDOWN_REQ(
            int socketfd,
            const void* buffer,
            size_t bufferLen,
            const struct sockaddr* sender,
            socklen_t senderLen)
{
    /* ID contenuto nella risposta che deve essere
     * uguale all'ID del peer affinché il messaggio
     * sia ritenuto valido */
    uint32_t msgPeerID;
    struct shutdown_req* req;
    /* flag che informa se gestire lo spegnimento */
    int terminate = 0;

    /* controlla l'integrità dei parametri */
    if (socketfd < 0 || buffer == NULL || bufferLen == 0 || sender == NULL || senderLen == 0)
        errExit("*** UDP:handle_MESSAGES_SHUTDOWN_REQ ***\n");

    /* controlla l'integrità del messaggio */
    if (messages_check_shutdown_req(buffer, bufferLen) == -1)
        return -1;

    req = (struct shutdown_req*)buffer;

    /* estrae l'ID contenuto del messaggio */
    if (messages_get_shutdown_req_body(req, &msgPeerID) == -1)
        return -1;

    /* ora deve controllare se l'ID coincide con quello del peer */
    /* in maniera sicura */
    if (pthread_mutex_lock(&IDguard) != 0)
        errExit("*** UDP:pthread_mutex_lock ***\n");

    /* se il peer è connesso al network e il
     * messaggio è effettivamente per lui */
    if (ISPeerConnected != 0 && peerID == msgPeerID)
        terminate = 1;

    /* rilascia il mutex */
    if (pthread_mutex_unlock(&IDguard) != 0)
        errExit("*** UDP:pthread_mutex_unlock ***\n");

    if (terminate == 0)
        return -1;

    unified_io_push(UNIFIED_IO_NORMAL, "\tSent MESSAGES_SHUTDOWN_ACK!");
    /* inva la risposta */
    if (messages_send_shutdown_response(socketfd, sender, senderLen, req) == -1)
        errExit("*** UDP:messages_send_shutdown_response ***\n");

    /* verso lo spegnimento */
    return 0;
}

/** Funzione ausiliaria per la
 * .
 */
static int
handle_MESSAGES_BOOT_ACK(
            int socketfd,
            const void* buffer,
            size_t bufferLen)
{
    int ans;
    uint32_t pid; /* message pid */
    struct boot_ack* ack;
    struct boot_ack* copy;

    /* silenzia il warning */
    (void)socketfd;

    /* verifica gli argomenti */
    if (socketfd < 0 || buffer == NULL || bufferLen == 0)
        errExit("*** UDP:handle_MESSAGES_SHUTDOWN_REQ ***\n");

    /* controlla l'integrità del messaggio */
    if (messages_check_boot_ack((void*)buffer, bufferLen) == -1)
    {
        unified_io_push(UNIFIED_IO_NORMAL, "\tMalformed message!");
        return -1;
    }

    ack = (struct boot_ack*)buffer;

    /* sezione critica */
    if (pthread_mutex_lock(&BOOTguard) != 0)
        errExit("*** UDP:pthread_mutex_lock ***\n");

    /* se c'è una richiesta in coda */
    ans = -1; /* di default si fallisce */
    if (BOOTflag)
    {
        if (messages_get_pid(ack, &pid) == -1)
            errExit("*** UDP:messages_get_pid ***\n");

        /* controlla che la richiesta coincida
         * con il messaggio inviato */
        if (pid == BOOTpid)
        {
            /* clona il messaggio */
            copy = (struct boot_ack*)messages_clone((void*)buffer);
            if (copy == NULL)
                errExit("*** UDP:messages_clone ***");
            /* mette la copia dove il main thread
             * lo leggerà */
            BOOTack = (volatile struct boot_ack*)copy;
            ans = 0;
            /* segnala il main thread */
            if (pthread_cond_signal(&BOOTcond) != 0)
                errExit("*** UDP:pthread_cond_signal ***");
            unified_io_push(UNIFIED_IO_NORMAL, "\tReceived requested response!");
        }
        else
            unified_io_push(UNIFIED_IO_NORMAL, "\tWrong response pid!");
    }

    /* termine della sezione critica */
    if (pthread_mutex_unlock(&BOOTguard) != 0)
        errExit("*** UDP:pthread_mutex_unlock ***\n");

    return ans;
}

/** Funzione ausiliaria che si
 * occupa di tutta la parte
 * relativa alla ricezione
 * dei messaggi dal socket UDP
 */
static void UDPReadLoop()
{
    char buffer[1024];
    struct sockaddr_storage ss;
    socklen_t ssLen;
    ssize_t msgLen;
    char sendName[32];

    while (UDPloop)
    {
        /* bisogna inizializzare questa variabile prima di invocare recvfrom */
        ssLen = sizeof(struct sockaddr_storage);
        msgLen = recvfrom(socketfd, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&ss, &ssLen);
        /* controllo per errore */
        if (msgLen == -1)
            errExit("*** UDP ***\n");

        /* blocca il segnale e gestisce il messaggio */
        if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
            errExit("*** pthread_sigmask ***\n");

        /* stampa le informazioni sul dato ricevuto */
        if (sockaddr_as_string(sendName, sizeof(sendName), (struct sockaddr*)&ss, ssLen) == -1)
            errExit("*** UDP:sockaddr_as_string ***\n");

        unified_io_push(UNIFIED_IO_NORMAL, "Received message from %s", sendName);

        /* cerca di ricavare il tipo di messaggio ricevuto */
        switch (recognise_messages_type((void*)buffer))
        {
        case -1:
            unified_io_push(UNIFIED_IO_NORMAL, "\tCannot recognise type of message!");
            break;

        /* È stato ricevuto un messaggio dalla sentinella compromessa */
        case MESSAGES_MSG_UNKWN:
            unified_io_push(UNIFIED_IO_NORMAL, "\tNonstandard message received!");
            break;

        case MESSAGES_BOOT_ACK:
            unified_io_push(UNIFIED_IO_NORMAL, "\tMessage MESSAGES_BOOT_ACK!");
            /* messaggi */
            if (handle_MESSAGES_BOOT_ACK(socketfd, buffer, (size_t)msgLen) == 0)
            {
                /* ricevuta la risposta attesa */
                /* memorizza globalmente l'indirizzo del server */
                DSaddr = ss;
                DSaddrLen = ssLen;
            }

            /* ora sarà l'altro thread a gestire il dato */
            break;

        case MESSAGES_SHUTDOWN_REQ:
            unified_io_push(UNIFIED_IO_NORMAL, "\tMessage MESSAGES_SHUTDOWN_REQ!", sendName);
            /* verifica se bisogna morire */
            if (handle_MESSAGES_SHUTDOWN_REQ(socketfd, buffer, (size_t)msgLen, (struct sockaddr*)&ss, ssLen) == 0)
            {
                unified_io_push(UNIFIED_IO_NORMAL, "\tMessage MESSAGES_SHUTDOWN_REQ!", sendName);
                /** gestione del problema descritto prima
                 * della dichiarazione del mutex
                 */
                /* INIZIO SEZIONE CRITICA */
                if (pthread_mutex_lock(&termUDPguard) != 0)
                    errExit("*** UDP:pthread_mutex_lock ***\n");
                if (termUDPflag == 0)
                {
                    termUDPflag = 1;
                    if (pthread_kill(mainThreadID, MAIN_LOOP_TERMINATION_SIGNAL) != 0)
                        errExit("*** UDP:pthread_kill ***\n"); /*scoppia*/
                }
                /* TERMINE SEZIONE CRITICA */
                if (pthread_mutex_unlock(&termUDPguard) != 0)
                    errExit("*** UDP:pthread_mutex_unlock ***\n");
                if (close(socketfd) != 0)
                    errExit("*** UDP:close ***\n");
                pthread_exit(NULL); /* fine */
                errExit("*** UDP:shutdown ***\n");
                abort();/* mai raggiunto */
            }
            else
                unified_io_push(UNIFIED_IO_NORMAL, "\tInvalid shutdown request!", sendName);
            /* normale, il messaggio era farlocco, si va avanti */
            break;

        case MESSAGES_CHECK_ACK:
            unified_io_push(UNIFIED_IO_NORMAL, "\tMessage MESSAGES_CHECK_ACK!");
            /* leggere la descrizione sopra la definizione di CHECKguard
             * per capire il ruolo */
            handle_MESSAGES_CHECK_ACK(socketfd, buffer, (size_t)msgLen);
            break;

        default:
            unified_io_push(UNIFIED_IO_NORMAL, "\tBad message received!", sendName);
            break;
        }

        /* sblocca il segnale e riprende il ciclo */
        if (pthread_sigmask(SIG_UNBLOCK, &toBlock, NULL) != 0)
            errExit("*** pthread_sigmask ***\n");
    }
}

/** Funzione ausiliaria che si occupa di staccare
 * il peer corrente dal network.
 */
static void
handle_peer_close(int sockfd)
{
    uint32_t msgID;
    int i;
    /* per il polling */
    struct pollfd fd;
    int timeout;
    /* messaggio */
    char buffer[256];
    ssize_t msgLen;

    /* se connesso invia al server un messaggio di shutdown e si
     * mette in attesa di una risposta */
    /* usa messaggi di tipo MESSAGES_SHUTDOWN_REQ */
    /* e aspetta una risposta di tipo MESSAGES_SHUTDOWN_ACK */
    /* sezione critica */
    if (pthread_mutex_lock(&IDguard) != 0)
        errExit("*** UDP:pthread_mutex_lock ***\n");

    /* deve essere connesso per disconnettersi */
    if (ISPeerConnected)
    {
        /* al più MAX_STOP_ATTEMPT tentativi */
        for (i = 0;  i < MAX_STOP_ATTEMPT; ++i)
        {
            unified_io_push(UNIFIED_IO_NORMAL,
                "Try peer disconnection: attempt [%d] of [%d]",
                i+1, MAX_STOP_ATTEMPT);

            /* invia il messaggio di risposta */
            if (messages_send_shutdown_req(sockfd, (struct sockaddr*)&DSaddr, DSaddrLen, peerID) == -1)
                errExit("*** UDP:messages_send_shutdown_req ***\n");

            unified_io_push(UNIFIED_IO_NORMAL, "\tSent messages [MESSAGES_SHUTDOWN_REQ]");
            /* si mette in attesa del messaggio */
            /* assunzione semplicistica di non ricevere messaggi farlocchi spuri */
            timeout = STOP_TIMEOUT; /* imposta il timeout */
            /* prepara la struttura per il polling */
            memset(&fd, 0, sizeof(fd));
            fd.fd = sockfd;
            fd.events = POLLIN;
            errno = 0;
            switch (poll(&fd, 1, timeout))
            {
            case -1:
                errExit("*** UDP:poll ***\n");
                break;
            case 0:
                /* no data avaible */
                unified_io_push(UNIFIED_IO_NORMAL, "\tNo message received");
                break;
            default:
                /* risultati disponibili */
                /* legge fino a che non esaurisce i messaggi */

                /* do while */
                do
                {
                    /* non serve ottenere informazioni su chi invia */
                    msgLen = recv(sockfd, (void*)buffer, sizeof(buffer), MSG_DONTWAIT);
                    if (msgLen == -1) /* fine, esci */
                        break;


                    /* check del messaggio per vedere se è quello giusto */
                    if (recognise_messages_type((void*)buffer) != MESSAGES_SHUTDOWN_ACK)
                    {
                        unified_io_push(UNIFIED_IO_NORMAL, "\tWrong message type");
                        continue; /* cerca un altro messaggio */
                    }
                    unified_io_push(UNIFIED_IO_NORMAL, "\tReceived message [MESSAGES_SHUTDOWN_ACK]");

                    /* integro? */
                    if (messages_check_shutdown_ack((void*)buffer, msgLen) != 0)
                    {
                        unified_io_push(UNIFIED_IO_NORMAL, "\tMalformed message");
                        continue; /* malformato, ne aspetta un altro */
                    }

                    /* non dovrebbe mai fallire */
                    if (messages_get_shutdown_ack_body((struct shutdown_ack*)buffer, &msgID) != 0)
                        errExit("*** UDP:messages_get_shutdown_ack_body ***\n");

                    /* il messaggio era per questo peer */
                    if (msgID != peerID)
                    {
                        unified_io_push(UNIFIED_IO_NORMAL,
                            "\tWrong ID in msg Body: [%ld] instead of [%ld]",
                            msgID, peerID);
                        continue; /* passa al prossimo */
                    }

                    unified_io_push(UNIFIED_IO_NORMAL, "\tValid message [MESSAGES_SHUTDOWN_ACK] received!");

                    /* messaggio valido, possiamo ottenere una risposta */
                    break;
                } while (msgLen != -1);
                if (msgLen == -1)
                {
                    if (errno != EWOULDBLOCK || errno != EAGAIN)
                        errExit("*** UDP:poll ***\n");
                    /* next loop */
                }

                break;
            }
            /* se è vero significa che abbiamo ricevuto una risposta dal DS */
            if (msgLen != -1)
                break;
        }
    }

    if (pthread_mutex_unlock(&IDguard) != 0)
        errExit("*** UDP:pthread_mutex_unlock ***\n");
}


/** Funzione che rappresenta il corpo del
 * thread che gestira il socket
 */
static void* UDP(void* args)
{
    struct thread_semaphore* ts;
    int* data;
    /* porta decisa dal padre */
    int requestedPort;
    /* porta usata dal thread */
    int usedPort;

    /* per il segnale di terminazione */
    struct sigaction toStop;

    ts = thread_semaphore_form_args(args);
    if (ts == NULL)
        errExit("*** UDP ***\n");

    data = (int*)thread_semaphore_get_args(args);
    if (data == NULL)
        errExit("*** UDP ***\n");

    /* firma da assegnare a tutti i messaggi del thread */
    if (unified_io_set_thread_name("UDP") != 0)
        fatal("UDP:unified_io_set_thread_name");

    /* INIZIO parte copiata dal ds */
    /* prepara l'handler per la terminazione del peer */
    memset(&toStop, 0, sizeof(struct sigaction));
    toStop.sa_handler = &sigHandler;
    if (sigfillset(&toStop.sa_mask) != 0)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP:sigaction ***\n");
        pthread_exit(NULL);
    }
    /* si prepara per bloccare il segnale da usare poi */
    if (sigemptyset(&toBlock) != 0)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP:sigemptyset ***\n");
        pthread_exit(NULL);
    }
    /* inserisce il segnale nella maschera */
    if (sigaddset(&toBlock, INTERRUPT_SIGNAL) != 0)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP:sigaddset ***\n");
        pthread_exit(NULL);
    }
    /* lo blocca per tutto il tempo necessario */
    if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP:pthread_sigmask ***\n");
        pthread_exit(NULL);
    }
    /* ora imposta l'opportuno handler */
    if (sigaction(INTERRUPT_SIGNAL, &toStop, NULL) != 0)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP:sigaction ***\n");
        pthread_exit(NULL);
    }
    /* FINE della parte copiata dal ds */

    /* codice per gestire l'apertura del socket */
    requestedPort = *data;
    /* crea il socket UDP */
    socketfd = initUDPSocket(requestedPort);
    if (socketfd == -1)
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
        pthread_exit(NULL);
    }
    /* verifica la porta */
    usedPort = getSocketPort(socketfd);
    if (usedPort == -1 || (requestedPort != 0 && requestedPort != usedPort))
    {
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
        pthread_exit(NULL);
    }
    /* metodo per fornire al chiamante
     * la porta utilizzata */
    *data = usedPort;

    /* socket creato, porta nota, si può iniziare */
    if (thread_semaphore_signal(ts, 0, NULL) == -1)
    {
        errExit("*** UDP ***\n");
    }

    /* a questo punto il padre può riprendere */
    unified_io_push(UNIFIED_IO_NORMAL, "UDP thread running");
    /* qui va il loop di gestione delle richieste */
    UDPloop = 1;

    /* CHECKPOINT */
    if (sigsetjmp(sigSetJmp, 1) == 0)
    {
        /* ora è tutto pronto e si può sbloccare il segnale */
        if (pthread_sigmask(SIG_UNBLOCK, &toBlock, NULL) != 0)
            errExit("*** UDP:pthread_sigmask ***\n");

        /* il ciclo è gestito altrove per ragioni
         * di leggibilità */
        UDPReadLoop();
        /* mai raggiunto se non in caso di terminazione
         * precoce */
    }
    /* mai raggiunto prima del termine */

    /* gestisce il distacco del
     * peer dal network */
    handle_peer_close(socketfd);

    /* chiude il socket */
    if (close(socketfd) != 0)
        errExit("*** UDP:close ***\n");

    return NULL;
}

int UDPstart(int port)
{
    int listeningOn;

    listeningOn = port;
    /* si mette in attesa dell'avvio del thread UDP */
    if (start_long_life_thread(&UDP_tid, &UDP, (void*)&listeningOn, NULL) == -1)
        return -1;

    UDP_port = listeningOn;

    return listeningOn;
}

int UDPisConnected()
{
    return ISPeerConnected;
}

/* utilizza startPipe */
int UDPconnect(const char* hostname, const char* portname)
{
    /* per inviare la richiesta tramite socket */
    struct sockaddr_storage ss;
    socklen_t sl;
    int ans; /* per tenere il risultato */
    int attempt; /* quanti tentativi ha fatto fin ora */
    /* variabili usate per mostrare quanto inviato */
    struct ns_host_addr* ns_addr_send; /* mesg inviato */
    char msgBody[32]; /* per stampare il dato inviato */
    char destStr[32]; /* per stampare a chi viene inviato. */
    /* per la gestione della risposta */
    struct boot_ack* ack; /* puntatore alla risposta */
    int err;
    /* per la gestione della risposta */
    uint32_t offeredID;
    struct peer_data peersAddrs[MAX_NEIGHBOUR_NUMBER];
    size_t peersNum;
    /* per il timeout di attesa della risposta */
    struct timeval now;
    struct timespec waitTime;
    int i;
    char neigbourStr[64];

    /* codice per assegnare il pseudo id ai messaggi
     * inviati */
    static int once = 1; /* per usare il tutto solo la prima volta */
    static uint32_t pid; /* pid da associare a ciascuna nuova richiesta */

    if (once) /* solo la prima volta */
    {
        once = 0;
        srand(time(NULL));
        /* primo valore casuale */
        pid = (uint32_t)rand();

        /* salva il thread ID del main thread */
        mainThreadID = pthread_self();
    }
    else
    {
        ++pid;
    }

    ans = -1; /* errore di default */

    /* prova a ricavare l'indirizzo di destinazione */
    if (getSockAddr((struct sockaddr*)&ss, &sl, hostname, portname, 0, 0) == -1)
        return -1;

    attempt = 0;
    /* ripete fino a che non connette */
    do {
        /* in questa nuova versione del progetto si usa un
         * protocollo produttore consumatore per comunicare
         * tra i thread */
        /* inizio sezione critica */
        if (pthread_mutex_lock(&BOOTguard) != 0)
            errExit("*** main:pthread_mutex_lock ***\n");
        /* accende il flag - richiesta pendente */
        BOOTflag = 1;
        /* imposta il pid della richiesta che ci si aspetta */
        BOOTpid = pid;

        /* controllo di consistenza */
        if (BOOTack != NULL)
            errExit("*** main:inconsistenza-BOOTack ***\n");

        /* prova a inviare il messaggio */
        /* se fallisce qui c'è proprio un problema
         * di invio */
        if (messages_send_boot_req(socketfd, (struct sockaddr*)&ss, sl, socketfd, pid, &ns_addr_send) != 0)
            goto endBoot;

        /* resoconto di cosa è stato inviato a chi */
        if (ns_host_addr_as_string(msgBody, sizeof(msgBody), ns_addr_send) == -1)
            errExit("*** main:ns_host_addr_as_string ***\n");
        if (sockaddr_as_string(destStr, sizeof(destStr), (struct sockaddr*)&ss, sl) == -1)
            errExit("*** main:sockaddr_as_string ***\n");

        unified_io_push(UNIFIED_IO_NORMAL, "Inviato messaggio di boot:\n\tdest: %s\n\tcont: %s\n", destStr, msgBody);

        /*si mette in attesa della risposta*/
        /* sarà il thread secondario a controllare
         * l'integrità del messaggio di risposta */
        /* imposta l'attesa massima sulla variabile di condizione */
        /* copiato dalla sezione EXAMPLE di pthread_cond_timedwait */
        gettimeofday(&now, NULL);
        waitTime.tv_nsec = now.tv_usec*1000;
        waitTime.tv_sec =  now.tv_sec + BOOT_TIMEOUT/1000;
        /* per ora ignoro wake up improvvisi */
        /* pthread_cond_timedwait non usa errno */
        if ((err = pthread_cond_timedwait(&BOOTcond, &BOOTguard, &waitTime)) != 0 && BOOTack == NULL)
        {
            if (err != ETIMEDOUT)
                errExit("*** main:pthread_cond_timedwait ***\n");

            /* spegne il flag, non aspettiamo più nulla */
            BOOTflag = 0;

            if (pthread_mutex_unlock(&BOOTguard) != 0)
                errExit("*** main:pthread_cond_timedwait ***\n");
            /* niente in questo ciclo, andiamo oltre */
            continue;
        }

        /* se arriva qui il messaggio è arrivato entro il
         * timeout */
        ack = (struct boot_ack*)BOOTack;
        BOOTack = NULL;
        BOOTflag = 0; /* spegne il flag */

        /* fine sezione critica, così da evitare che
         * il secondo thread si impunti */
        if (pthread_mutex_unlock(&BOOTguard) != 0)
            errExit("*** main:pthread_cond_timedwait ***\n");

        /* si mette in attesa di un risultato o del timeout */
        /* per attendere una notifica dal thread secondario */
            /* verifica che la risposta si valida */
            /* ATTENZIONE: l'integrità è già verificata
             * da thread secondario */

        /* c'è un input */
        /* lo legge - usa read */
        ;
        unified_io_push(UNIFIED_IO_NORMAL, "Received response from ds\n");

        /* prende il messaggio e lo gestisce,
            * siamo già certi della sia consistenza */
        peersNum = MAX_NEIGHBOUR_NUMBER; /* non necessario ma non si sa mai */
        if (messages_get_boot_ack_body_cp(ack, &offeredID, peersAddrs, &peersNum) == -1)
            errExit("*** main:messages_get_boot_ack_body ***\n");

        /* imposta l'ID del peer */
        if (pthread_mutex_lock(&IDguard) != 0)
            errExit("*** main:pthread_mutex_lock ***\n");
        /* controlla che non sia già connesso */
        if (ISPeerConnected)
            errExit("*** ALREADY CONNECTED! ***\n");
        /* imposta l'ID del peer */
        peerID = offeredID;
        /* accende il flag */
        ISPeerConnected = 1;
        if (pthread_mutex_unlock(&IDguard) != 0)
            errExit("*** main:pthread_mutex_unlock ***\n");

        unified_io_push(UNIFIED_IO_NORMAL, "Peer connesso a una rete con ID [%ld]\n", (long)offeredID);

        /* libera la memoria del messaggio */
        free((void*)ack);

        /* stampa le informazioni appena ottenute sui vicini */
        unified_io_push(UNIFIED_IO_NORMAL, "Numero neighbours: [%ld]\n", (long)peersNum);
        for (i = 0; i != (int)peersNum; ++i)
        {
            peer_data_as_string(&peersAddrs[i], neigbourStr, sizeof(neigbourStr));
            unified_io_push(UNIFIED_IO_NORMAL, "\t%d) - %s\n", (long)peersNum, neigbourStr);
        }

        /* si può adesso avviare correttamente il thread TCP */
        if (TCPrun(offeredID, peersAddrs, peersNum) != 0)
            errExit("*** TCPrun ***\n");
        /* a quel punto possiamo tornare */
        /* è andata bene */
        ans = 0;
        /* possiamo interrompere il ciclo */
        break;

    } while (++attempt < MAX_BOOT_ATTEMPT);
endBoot:

    return ans;
}

int UDPstop(void)
{
    /* controlla che fosse partito */
    if (UDP_tid == 0)
        return -1;

    /** gestisco il problema evidenziato sopra
     * la descrizione del mutex
     */
    /* INIZIO SEZIONE CRITICA */
    if (pthread_mutex_lock(&termUDPguard) != 0)
        errExit("*** UDP:pthread_mutex_lock ***\n");

    if (termUDPflag == 0)
    {
        termUDPflag = 1;
        /* invia il segnale di terminazione */
        if (pthread_kill(UDP_tid, INTERRUPT_SIGNAL) != 0)
            errExit("*** UDP:pthread_kill ***\n");
    }

    /* TERMINE SEZIONE CRITICA */
    if (pthread_mutex_unlock(&termUDPguard) != 0)
        errExit("*** main:pthread_mutex_unlock ***\n");

    /* alla fine bisogna sempre rilasciare
     * le risorse allocate per il thread */
    if (pthread_join(UDP_tid, NULL) != 0)
        return -1;

    UDP_port = 0;

    return 0;
}

int UDPport(void)
{
    return UDP_port;
}

/** Quanti secondi al più può aspettare
 * per la risoluzione della chiamata?
 */
#define UDPcheckTIMEOUT 2

int UDPcheck(
            struct peer_data* neighbours,
            size_t* length)
{
    /* per garantire la mutua esclusione nell'uso
     * di questa funzionalità */
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    /* per l'attesa sulla cond.var. */
    struct timeval now;
    struct timespec timeout;
    int retcode;
    int ans = 0; /* ottimista */
    int i;

    /* controllo validità degli argomenti */
    if (neighbours == NULL || length == NULL)
        return -1;

    /* se non si è connessi ritorna immediatamente */
    if (pthread_mutex_lock(&mutex) != 0)
        abort();
    if (!ISPeerConnected)
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    if (pthread_mutex_unlock(&mutex) != 0)
        abort();
    /* OK: la funzionalità è disponibile */
    /* solo un thread per volta può invocare questa
     * funzione */
    /* SEZIONE CRITICA ESTERNA */
    if (pthread_mutex_lock(&mutex) != 0)
        abort();

    /* SEZIONE CRITICA INTERNA - vedere descrizione
     * sopra la dichiarazione del mutex */
    if(pthread_mutex_lock(&CHECKguard) != 0)
        abort();
    /* vale: CHECKnumber==0&&CHECKflag==0 */

    /* invia il messaggio */
    if (messages_send_check_req(socketfd, (struct sockaddr*)&DSaddr,
        DSaddrLen, (uint16_t)UDP_port) != 0)
        abort();
    /* accende il flag */
    CHECKflag = 1;
    /* pausa temporizzata - vedi man pthread_cond_timedwait */
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + UDPcheckTIMEOUT; /* imposta il timeout */
    timeout.tv_nsec = now.tv_usec * 1000;
    retcode = 0;
    while (CHECKflag && retcode != ETIMEDOUT)
    {
        retcode = pthread_cond_timedwait(&CHECKcond, &CHECKguard, &timeout);
    }
    /* Perché si è sbloccato? */
    if (retcode == ETIMEDOUT && CHECKflag)
    {
        ans = -1; /* andata male */
        errno = ETIMEDOUT; /* casomai interessi */
        CHECKflag = 0; /* ripristina lo stato */
    }
    else if (CHECKflag == 0)/* ci sono dei dati */
    {
        /* copia i risultati per renderli disponibili al chiamante */
        for (i = 0; i != (int)CHECKnumber; ++i) /* passa le info sui vicini */
            neighbours[i] = CHECKneighbours[i];

        *length = CHECKnumber; /* passa il numero di vicini  */
        CHECKnumber = 0; /* alla fine ripristina lo stato */
    }
    else /* errore prematuro - status della risposta anomalo */
    {
        CHECKflag = 0;
        ans = 1;
    }

    /* TERMINE SEZIONE CRITICA INTERNA */
    if (pthread_mutex_unlock(&CHECKguard) != 0)
        abort();

    /* TERMINE SEZIONE CRITICA ESTERNA */
    if (pthread_mutex_unlock(&mutex) != 0)
        abort();

    return ans;
}
