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
static int handle_MESSAGES_BOOT_REQ(void* buffer, size_t msgLen, struct sockaddr* source, socklen_t sourceLen)
{
    struct ns_host_addr* ns_addr;
    char msgStr[32]; /* per stampare il contenuto del messaggio */
    char srcStr[32]; /* per stampare l'indirizzo del mittente */
    char svdStr[32]; /* per stampare ciò che sarà effettivamente memorizzato */

    unified_io_push(UNIFIED_IO_NORMAL, "Received msg [MESSAGES_BOOT_REQ]");
    /* verifica l'integrità del messaggio */
    if (messages_check_boot_req(buffer, msgLen) == 0)
    {
        /* estrae il contenuto del messaggio */
        if (messages_get_boot_req_body(&ns_addr, (struct boot_req*)buffer) == -1)
            errExit("*** messages_get_boot_ack_body ***\n");

        /* lo trasforma in stringa */
        if (ns_host_addr_as_string(msgStr, sizeof(msgStr), ns_addr) == -1)
                    errExit("*** ns_host_addr_as_string ***\n");
        /* ottiene in formato stringa l'indirizzo sorgente */
        if (sockaddr_as_string(srcStr, sizeof(srcStr), source, sourceLen) == -1)
            errExit("*** sockaddr_as_string ***\n");
        /* controlla se l'indirizzo speficificato nel messaggio è 0.0.0.0 o equivalente */
        if (ns_host_addr_any(ns_addr) == 1)
        {
            if (ns_host_addr_update_addr(ns_addr, source) == -1)
                errExit("*** ns_host_addr_update_addr ***\n");
        }
        /* rende in formato strinfa anche quello che sarà salvato */
        if (ns_host_addr_as_string(svdStr, sizeof(svdStr), ns_addr) == -1)
            errExit("*** ns_host_addr_as_string ***\n");

        /* invia l'output al sottosistema di io */
        unified_io_push(UNIFIED_IO_NORMAL,
            "\tmsg:\t%s\n\tsource:\t%s\n\tsaved:\t%s", msgStr, srcStr, svdStr);

    }
    else
    {
        unified_io_push(UNIFIED_IO_ERROR, "\tMalformed message");
    }

    return 0;
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
                handle_MESSAGES_BOOT_REQ(buffer, msgLen, (struct sockaddr*)&sender, senderLen);
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
