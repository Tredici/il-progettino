#include "socket_utils.h"


/** The Linux Programming Interface pag. 1229
 * 
 */
int initUDPSocket(int port)
{
    int sckt, err;
    int sockOpt; /* Per setsockopt */

    struct addrinfo hints;
    struct addrinfo* res, *iter;

    char s_port[10];

    sprintf(s_port, "%d", port);

    memset(&hints, 0, sizeof(struct addrinfo));
    /* Inizializzazione */
    hints.ai_family = AF_UNSPEC; /* si accetta IPv4 ed IPv6 */
    hints.ai_socktype = SOCK_DGRAM;
    /* AI_PASSIVE: useremo il risultato per una bind (0.0.0.0)
     * AI_NUMERICSERV: il servizio è dato dal numero di porta
     * AI_ADDRCONFIG: per non avere ipv6 se non possiamo usarli
     */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_ADDRCONFIG;

    /* Ottiene i candidati da utilizzare */
    err = getaddrinfo(NULL, s_port, &hints, &res);
    if (err != 0)
    {
        return -1;
    }

    /* Fa tutti i tentativi necessari per creare il socket */
    for (iter = res; iter; iter = iter->ai_next)
    {
        /* Prova a creare un socket UDP */
        sckt = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (sckt == -1)
            continue;   /* proviamo il prossimo */

        /* Permette di impostare il riutilizzo della porta specificata */
        sockOpt = 1;
        err = setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(sockOpt));
        if (err != 0)
        {
            close(sckt);    /* Chiude il socket sul quale si ha provato ad operare */
            freeaddrinfo(res);  /* in caso di errore libera la memoria di prima */
            return -1;
        }

        if (bind(sckt, iter->ai_addr, iter->ai_addrlen) == 0)
            break;  /* È riuscito a connettere il socket */

        close(sckt);    /* Chiude e riprova con un altro indirizzo */
    }

    /* Libera la memoria allocata */
    freeaddrinfo(res);
    
    if (iter == NULL)
        return -1;  /* Le ha provate tutte ma ha fallito */

    return sckt;
}

int getSocketPort(int socket)
{
    struct sockaddr_storage ss;
    socklen_t size = sizeof(struct sockaddr_storage);
    int port;

    if (getsockname(socket, (struct sockaddr*) &ss, &size) != 0)
    {
        return -1;
    }

    switch (ss.ss_family)
    {
    case AF_INET:
        /* IPv4 */
        port = ntohs(((struct sockaddr_in*) &ss)->sin_port);
        break;

    case AF_INET6:
        /* IPv4 */
        port = ntohs(((struct sockaddr_in6*) &ss)->sin6_port);
        break;

    default:
        return -1;
    }

    return port;
}
