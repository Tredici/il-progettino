

#include "ds_udp.h"
#include <pthread.h>
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include "../unified_io.h"
#include <signal.h>
#include <sched.h>

/** Id del thead che gestisce il socket
 * UDP
 */
pthread_t UDP_tid;

/** Fino a che questa variabile
 * non sarà azzerata il ciclo del
 * sottosistema UDP
 */
volatile sig_atomic_t UDPloop = 1;

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
    UDPloop = 1;
    while (UDPloop)
    {
    
        /* gestisce una richiesta alla volta */

    }
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
    if (pthread_kill(UDP_tid, SIGTERM) != 0)
        return -1;

    /* attende la terminazione del thread */
    if (pthread_join(UDP_tid, NULL) != 0)
        return -1;

    return 0;
}
