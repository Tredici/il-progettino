#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

/** ATTENZIONE:
 * dato che in questo file andrò
 * a generare il contenuto dei
 * pacchetti da inviare via rete
 * utilizzerò più volte l'estensione
 * di GCC __attribute__ ((packed))
 * per essere sicuro che la struttura
 * delle strutture non sia alterata
 * attraverso la rete.
 */


int recognise_messages_type(void* msg)
{
    struct messages_head* p;

    if (msg == NULL)
        return -1;

    p = (struct messages_head*)msg;
    if (p->sentinel != 0)
        return MESSAGES_MSG_UNKWN;

    return (int)ntohs(p->type);
}


int messages_make_boot_req(void** buffer, size_t* sz, int socket)
{
    struct boot_req* ans;

    /* per ottenere informazioni sul socket a disposizione */
    struct sockaddr_storage ss;
    socklen_t size = sizeof(struct sockaddr_storage);

    /* controllo sulla validità degli argomenti */
    if (buffer == NULL || sz == NULL)
    {
        return -1;
    }

    /* ottiene le informazioni sul socket da usare */
    if (getsockname(socket, (struct sockaddr*) &ss, &size) != 0)
    {
        return -1;
    }

    /* se siamo arrivati qui forse le cose funzionano */
    ans = malloc(sizeof(struct boot_req)); /* allochiamo il buffer */
    memset(ans, 0, sizeof(struct boot_req));
    /* prepariamo l'header */
    ans->head.sentinel = 0;
    ans->head.type = htons(MESSAGES_BOOT_REQ);

    /** bisogna trasferire:
     * +versione di IP
     * +porta
     * +indirizzo nella versione usata */
    if (ns_host_addr_from_sockaddr(&ans->body, (struct sockaddr*)&ss) == -1)
        return -1;

    *buffer = (void*)ans;
    *sz = sizeof(sizeof(struct boot_req));

    return 0;
}


int messages_check_boot_req(void* buffer, size_t len)
{
    struct boot_req* req;
    uint16_t port;

    if (buffer == NULL || len != sizeof(struct boot_req))
        return -1;

    req = (struct boot_req*)buffer;

    /* controllo dell'header */
    if (req->head.sentinel != 0 || ntohs(req->head.type) != MESSAGES_BOOT_REQ)
        return -1;

    /* controlla la validità della versione IP */
    if (ns_host_addr_get_ip_version(&req->body) == 0) /* caso raro in cui 0 segnala l'errore */
        return -1;

    /* controlla che la porta non sia 0 */
    if (ns_host_addr_get_port(req, &port) == -1 || port == 0)
        return -1;

    return 0;
}

int messages_send_boot_req(int sockfd, const struct sockaddr* dest, socklen_t destLen, int sk)
{
    void* bootmsg;
    size_t msgLen;
    ssize_t sendLen;

    if (dest == NULL)
        return -1;

    if (messages_make_boot_req(&bootmsg, msgLen, sk) == -1)
        return -1;

    sendLen = sendto(sockfd, bootmsg, msgLen, 0, dest, destLen);
    if (sendLen != msgLen)
        return -1;

    return 0;
}

int
messages_get_boot_body(struct ns_host_addr** ns_addr,
            const struct boot_req* req)
{
    struct ns_host_addr* ans;
    /* controllo dei parametri */
    if (ns_addr == NULL || req == NULL)
        return -1;

    ans = malloc(sizeof(struct ns_host_addr));
    if (ans == NULL)
        return -1;

    *ans = req->body;
    *ns_addr = ans;
    return 0;
}

int
messages_make_boot_ack(struct boot_ack** buffer,
            size_t* sz,
            const struct boot_req* req,
            const struct ns_host_addr** peers,
            size_t nPeers)
{
    struct boot_ack* ans;
    int i;

    /* verifica integrità dei parametri */
    if (buffer == NULL || sz == NULL || req == NULL
        || (peers == NULL && nPeers != 0)
        || nPeers > MAX_NEIGHBOUR_NUMBER )
        return -1;

    /* prepara la potenziale risposta */
    ans = malloc(sizeof(struct boot_ack));
    memset(ans, 0, sizeof(struct boot_ack));

    /* prepara l'header */
    ans->head.sentinel = 0;
    ans->head.type = MESSAGES_BOOT_ACK;
    /* prepara il corpo */
    ans->body.length = nPeers;
    for (i = 0; i < nPeers; ++i)
        ans->body.neighbours[i] = *peers[i];

    *buffer = ans;
    *sz = sizeof(struct boot_ack);
    return 0;
}
