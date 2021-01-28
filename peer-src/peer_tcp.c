#include "peer_tcp.h"
#include "../socket_utils.h"
#include "../thread_semaphore.h"
#include <pthread.h>
#include "peer_udp.h" /* per UDPcheck */
#include "messages.h"
#include <sys/types.h>
#include <sys/socket.h>

/** Parametro da usare per la listen
 * sul socket TCP
 */
#define MAX_TCP_CONNECTION 20

static int activated;

/** Descrittore di file
 * del socket tcp
 */
static int tcpFd;

/** Flag che indica se il thread tcp
 * era stato avviato con successo.
 */
static volatile int running;

int peer_tcp_init(int port)
{
    int sk;

    /* controlla che il sistema non sia già stato avviato */
    if (activated)
        return -1;

    sk = initTCPSocket(port, MAX_TCP_CONNECTION);
    if (sk == -1)
        return -1;

    tcpFd = sk;
    activated = 1;

    return 0;
}


int peer_tcp_close(void)
{
    /* controlla se era stato attivato */
    if (!activated)
        return -1;

    if (running)
    {
        /* abbiamo un thread da fermare */
        running = 0;
    }

    if (close(tcpFd) != 0)
        return -1;

    tcpFd = 0;
    activated = 0;
    return 0;
}

int peer_tcp_get_socket(void)
{
    return activated ? tcpFd : -1;
}
