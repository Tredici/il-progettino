#include "peer_udp.h"
#include <pthread.h>
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include "../unified_io.h"
#include <signal.h>
#include <sched.h>

/* flag che verifica che il thread non sia già stato avviato */
volatile int started;

/** Id del thead che gestisce il socket
 * UDP
 */
thread_t UDP_tid;

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
    /* fd del socket */
    int socket;
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
    if (usedPort == -1 || (requestedPort == 0 && requestedPort != usedPort))
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
    while (1)
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
    int status;
    int listeningOn;

    listeningOn = port;
    /* si mette in attesa dell'avvio del thread UDP */
    start_long_life_thread(&UDP_tid, &UDP, (void*)&listeningOn, NULL);
    if (status == -1)
        return -1;

    return listeningOn;
}

