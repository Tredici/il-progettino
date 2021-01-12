#define _POSIX_C_SOURCE 199506L /* per pthread_sigmask */

#include "ds_udp.h"
#include <pthread.h>
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
    UDPloop = 0;
    siglongjmp(sigSetJmp, 1); /* magia */
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

    /* per il segnale di terminazione */
    struct sigaction toStop;
    sigset_t toBlock; /* per ignorare il segnale */

    ts = thread_semaphore_form_args(args);
    if (ts == NULL)
        errExit("*** UDP ***\n");

    data = (int*)thread_semaphore_get_args(args);
    if (data == NULL)
        errExit("*** UDP ***\n");

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
    unified_io_push("UDP thread running", UNIFIED_IO_NORMAL);

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
                break;

            case MESSAGES_BOOT_REQ:
                /* ricevuta richiesta di boot da gestire */
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
