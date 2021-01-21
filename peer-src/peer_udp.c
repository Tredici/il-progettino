#define _XOPEN_SOURCE 500 /* per pthread_kill */
#define _GNU_SOURCE /* per pipe2 */

#include "peer_udp.h"
#include <pthread.h>
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include "../unified_io.h"
#include "../messages.h"
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>

/* segnale utilizzato per terminare il
 * ciclo del peer */
#define INTERRUPT_SIGNAL SIGUSR1

/* Una richiesta di boot viene
 * inviata al più 5 volte con un
 * timeout di più 2 secondi prima
 * di inviarne una nuova  */
#define MAX_BOOT_ATTEMPT 5
#define BOOT_TIMEOUT 2000 /* in millisecondi */

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

/* Segue la descrizione delle varie pipe utilizzate */
/** Pipe utilizzata per i messaggi di boot.
 * Vi accede in lettura solo
 */
int startPipe[2];


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
    void* msgCopy;

    while (UDPloop)
    {
        msgLen = recvfrom(socketfd, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&ss, &ssLen);
        /* controllo per errore */
        if (msgLen == -1)
            errExit("*** UDP ***\n");

        /* blocca il segnale e gestisce il messaggio */
        if (pthread_sigmask(SIG_BLOCK, &toBlock, NULL) != 0)
            errExit("*** pthread_sigmask ***\n");

        /* stampa le informazioni sul dato ricevuto */
        if (sockaddr_as_string(sendName, sizeof(sendName), (struct sockaddr*)&ss, &ssLen) == -1)
            errExit("*** sockaddr_as_string ***\n");

        unified_io_push(UNIFIED_IO_NORMAL, "Received message from %s", sendName);

        /* cerca di ricavare il tipo di messaggio ricevuto */
        switch (recognise_messages_type((void*)buffer))
        {
        case -1:
            unified_io_push(UNIFIED_IO_NORMAL, "\tCannot recognise type of message!", sendName);
            break;

        /* È stato ricevuto un messaggio dalla sentinella compromessa */
        case MESSAGES_MSG_UNKWN:
            unified_io_push(UNIFIED_IO_NORMAL, "\tNonstandard message received!", sendName);
            break;

        case MESSAGES_BOOT_ACK:
            unified_io_push(UNIFIED_IO_NORMAL, "\tMessage MESSAGES_BOOT_ACK!", sendName);
            msgCopy = messages_clone((void*)buffer);
            if (msgCopy == NULL)
                errExit("*** UDP:messages_clone ***");

            /* inserisce la copia del messaggio nella coda apposita */
            if (write(startPipe[1], msgCopy, sizeof(struct boot_ack)) != (ssize_t)sizeof(struct boot_ack))
                errExit("*** UDP:write ***");

            /* ora sarà l'altro thread a gestire il dato */
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

    /* INIZIO parte copiata dal ds */
    /* prepara l'handler per la terminazione del peer */
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
    /* FINE della parte copiata dal ds */

    /* inizializza la pipe da usare con UDPstart */
    if (pipe(startPipe) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** pipe2 ***\n");
    /* imposta la parte in lettura come nonblocking */
    if (fcntl(startPipe[0], F_SETFL, O_NONBLOCK) == -1)
    {
        /* chiude la pipe */
        close(startPipe[0]); close(startPipe[1]);
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
    }

    /* codice per gestire l'apertura del socket */
    requestedPort = *data;
    /* crea il socket UDP */
    socketfd = initUDPSocket(requestedPort);
    if (socketfd == -1)
    {
        /* chiude la pipe */
        close(startPipe[0]); close(startPipe[1]);
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
    }
    /* verifica la porta */
    usedPort = getSocketPort(socketfd);
    if (usedPort == -1 || (requestedPort != 0 && requestedPort != usedPort))
    {
        /* chiude la pipe */
        close(startPipe[0]); close(startPipe[1]);
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
    }
    /* metodo per fornire al chiamante
     * la porta utilizzata */
    *data = usedPort;

    /* socket creato, porta nota, si può iniziare */
    if (thread_semaphore_signal(ts, 0, NULL) == -1)
    {
        /* chiude la pipe */
        close(startPipe[0]); close(startPipe[1]);
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
            errExit("*** pthread_sigmask ***\n");

        while (UDPloop)
        {
            if (yield)
                sched_yield(); /* altruista */
            else
                yield = 1; /* forse la lascia al prossimo */

            /* esamina le singole richiesta 1 per 1
            * e se ne trova imposta yield=0 */

        }
    }
    /* mai raggiunto prima del termine */

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

    return listeningOn;
}

/* utilizza startPipe */
int UDPconnect(const char* hostname, const char* portname)
{
    /* per inviare la richiesta tramite socket */
    struct sockaddr_storage ss;
    socklen_t sl;
    /* per cercare una risposta, il timeout viene
     * gestito tramite poll */
    struct pollfd ps;
    int ans; /* per tenere il risultato */
    int attempt; /* quanti tentativi ha fatto fin ora */
    /* variabili usate per mostrare quanto inviato */
    struct ns_host_addr* ns_addr_send; /* mesg inviato */
    char msgBody[32]; /* per stampare il dato inviato */
    char destStr[32]; /* per stampare a chi viene inviato. */
    /* per la gestione della risposta */
    struct boot_ack* ack; /* puntatore alla risposta */
    ssize_t err;

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
    }
    else
    {
        ++pid;
    }

    ans = -1; /* errore di default */

    /* prova a ricavare l'indirizzo di destinazione */
    if (getSockAddr((struct sockaddr*)&ss, &sl, hostname, portname, 0, 0) == -1)
        return -1;

    /* svuota la pipe in lettura */
    errno = 0;
    while ((err = read(startPipe[0], (void*)&ack, sizeof(ack))) ==  sizeof(ack))
        ;
    switch (err)
    {
    case -1:
        if (errno != EWOULDBLOCK)
            errExit("*** UDPstart ***\n");
        /* pipe correttamente svuotata */
        break;

    default:
        /* errore irrecuperabile */
        errExit("*** UDPstart ***\n");
        break;
    }

    attempt = 0;
    /* ripete fino a che non connette */
    do {
        /* prova a inviare il messaggio */
        /* se fallisce qui c'è proprio un problema
         * di invio */
        if (messages_send_boot_req(socketfd, (struct sockaddr*)&ss, sl, socketfd, pid, &ns_addr_send) != 0)
            goto endBoot;

        /* resoconto di cosa è stato inviato a chi */
        if (ns_host_addr_as_string(msgBody, sizeof(msgBody), ns_addr_send) == -1)
            errExit("*** ns_host_addr_as_string ***\n");
        if (sockaddr_as_string(destStr, sizeof(destStr), (struct sockaddr*)&ss, sl) == -1)
            errExit("*** ns_host_addr_as_string ***\n");

        printf("Inviato messaggio di boot:\n\tdest: %s\n\tcont: %s\n", destStr, msgBody);

        /* si mette in attesa di un risultato o del timeout */
        memset(&ps, 0, sizeof(ps)); /* azzera per sicurezza */
        /* per attendere una notifica dal thread secondario */
        ps.fd = startPipe[0];
        ps.events = POLLIN;
        /* poll */
        switch (poll(&ps, 1, BOOT_TIMEOUT))
        {
        case 0:
            /* nulla, continua */
            continue;
        case 1:
            /* verifica che la risposta si valida */
            /* ATTENZIONE: l'integrità è già verificata
             * da thread secondario */
            if (ps.revents & POLLIN)
            {
                /* c'è un input */
                /* lo legge - usa read */
                ;
                /* vede quali sono i peer vicini */
                /* li pinga */
                /* a quel punto possiamo tonnare */
                /* è andata bene */
                ans = 0;
                /* possiamo interrompere il ciclo */
                goto endBoot;
            }
            break;

        default: /* restituito -1 */
            /* errore polling */
            goto endBoot;
        }
    } while (++attempt < MAX_BOOT_ATTEMPT);
endBoot:

    return ans;
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

    if (pthread_join(UDP_tid, NULL) != 0)
        return -1;

    return 0;
}
