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

int getSockAddr(struct sockaddr* sk_addr,
                socklen_t* sk_len,
                const char* hostname,
                const char* portname,
                int socktype,
                int sockprotocol)
{
    struct addrinfo hints;
    struct addrinfo* res, *iter;

    if (sk_addr == NULL || sk_len == NULL
    || hostname == NULL || portname == NULL)
        return -1;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC; /* ipv4|ipv6 */
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;

    if (getaddrinfo(hostname, portname, &hints, &res) != 0)
        return -1;

    for (iter = res; iter; iter = iter->ai_next)
    {
        /* controlla il tipo */
        if (socktype != 0)
            if (socktype != iter->ai_socktype)
                continue;
        /* controlla il protocollo */
        if (sockprotocol != 0)
            if (sockprotocol != iter->ai_protocol)
                continue;

        /* se arriva qui rispetta tutti i test */
        freeaddrinfo(res);
        *sk_addr = *iter->ai_addr;
        *sk_len = iter->ai_addrlen;
        return 0;
    }

    freeaddrinfo(res);

    /* non ha trovato nulla */
    return -1;
}

int getSockAddrPort(const struct sockaddr* sk_addr, uint16_t* port)
{
    /* controllo validità parametri */
    if (sk_addr == NULL || port == NULL)
        return -1;

    switch (sk_addr->sa_family)
    {
    case AF_INET:
        *port = ntohs(((struct sockaddr_in*) sk_addr)->sin_port);
        break;

    case AF_INET6:
        *port = ntohs(((struct sockaddr_in6*) sk_addr)->sin6_port);
        break;

    default: /* tipo dell'indirizzo non riconosciuto */
        return -1;
    }

    return 0;
}
