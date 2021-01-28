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

/** Descrittore di file
 * del socket tcp
 */
static int tcpFd;

int peer_tcp_init(int port)
{
    int sk;

    /* controlla che il sistema non sia già stato avviato */
    if (tcpFd)
        return -1;

    sk = initTCPSocket(port, MAX_TCP_CONNECTION);
    if (sk == -1)
        return -1;

    tcpFd = sk;

    return 0;
}
