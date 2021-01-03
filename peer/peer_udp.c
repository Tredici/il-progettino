#include "peer_udp.h"
#include <pthread.h>
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include "../commons.h"
#include <signal.h>

/* flag che verifica che il thread non sia già stato avviato */
volatile int started;

/** Funzione che rappresenta il corpo del
 * thread che gestira il socket
 */
static void* UDP(void* args)
{
    struct thread_semaphore* ts;

    if (args == NULL)
    {
        errExit("*** UDP ***\n");
    }

    return NULL;
}


int UDPstart(int port)
{

}

