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
