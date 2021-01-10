#define _XOPEN_SOURCE 500 /* per pthread_kill */
#define _GNU_SOURCE /* per pipe2 */

#include "peer_udp.h"
#include <pthread.h>
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include "../unified_io.h"
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

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

/* fd del socket
 * è globale poiché deve essere
 * potenzialmente condiviso da
 * più thread */
int socket;

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
    /* flag per evitare di sprecare
     * troppi cicli di CPU cedendone
     * il controllo al altri thread
     * se per un ciclo non si trova
     * a fare nulla */
    int yield;

    ts = thread_semaphore_form_args(args);
    if (ts == NULL)
        errExit("*** UDP ***\n");

    data = (int*)thread_semaphore_get_args(args);
    if (data == NULL)
        errExit("*** UDP ***\n");

    /* inizializza la pipe da usare con UDPstart */
    if (pipe2(startPipe, O_DIRECT) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** pipe2 ***\n");

    /* codice per gestire l'apertura del socket */
    requestedPort = *data;
    /* crea il socket UDP */
    socket = initUDPSocket(requestedPort);
    if (socket == -1)
    {
        /* chiude la pipe */
        close(startPipe[0]); close(startPipe[1]);
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** UDP ***\n");
    }
    /* verifica la porta */
    usedPort = getSocketPort(socket);
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
    unified_io_push("UDP thread running", UNIFIED_IO_NORMAL);
    /* qui va il loop di gestione delle richieste */
    yield = 0; /* il primo ciclo di sicuro non si lascia la CPU */
    UDPloop = 1;
    while (UDPloop)
    {
        if (yield)
            sched_yield(); /* altruista */
        else
            yield = 1; /* forse la lascia al prossimo */

        /* esamina le singole richiesta 1 per 1
         * e se ne trova imposta yield=0 */

    }
    /* mai raggiunto */

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

    ans = -1; /* errore di default */

    /* prova a ricavare l'indirizzo di destinazione */
    if (getSockAddr((struct sockaddr*)&ss, &sl, hostname, portname, 0, 0) == -1)
        return -1;

    attempt = 0;
    /* ripete fino a che non connette */
    do {
        /* prova a inviare il messaggio */
        /* se fallisce qui c'è proprio un problema
         * di invio */
        if (messages_send_boot_req(socket, (struct sockaddr*)&ss, sl, socket) != 0)
            goto failedBoot;

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
                goto failedBoot;
            }
            break;

        default: /* restituito -1 */
            /* errore polling */
            goto failedBoot;
        }
    } while (++attempt < MAX_BOOT_ATTEMPT);
failedBoot:

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
    /*if (pthread_kill(UDP_tid, SIGTERM) != 0)
        return -1;*/
    /* metodo semplice ma efficace per terminare
     * il ciclo del thread */
    UDPloop = 0;


    if (pthread_join(UDP_tid, NULL) != 0)
        return -1;

    return 0;
}
