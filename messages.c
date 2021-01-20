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


int messages_make_boot_req(struct boot_req** buffer, size_t* sz, int socket, uint32_t pid)
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
    ans->head.pid = htonl(pid);

    /** bisogna trasferire:
     * +versione di IP
     * +porta
     * +indirizzo nella versione usata */
    if (ns_host_addr_from_sockaddr(&ans->body, (struct sockaddr*)&ss) == -1)
        return -1;

    *buffer = ans;
    *sz = sizeof(struct boot_req);

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
    if (ns_host_addr_get_port(&req->body, &port) == -1 || port == 0)
        return -1;

    return 0;
}

int messages_send_boot_req(int sockfd, const struct sockaddr* dest, socklen_t destLen, int sk, uint32_t pid, struct ns_host_addr** ns_addr)
{
    struct boot_req* bootmsg;
    struct ns_host_addr* ns_addr_val;
    size_t msgLen;
    ssize_t sendLen;

    if (dest == NULL)
        return -1;

    /* verifica, se il chiamante ha richiesto
     * una copia del messaggio, di avere abbastanza
     * memoria per fornirgliela */
    if (ns_addr != NULL)
    {
        ns_addr_val = malloc(sizeof(struct ns_host_addr));
        if (ns_addr_val == NULL)
            return -1;
    }
    else
        ns_addr_val = NULL;

    if (messages_make_boot_req(&bootmsg, &msgLen, sk, pid) == -1)
    {
        free((void*)ns_addr_val);
        return -1;
    }

    sendLen = sendto(sockfd, (void*)bootmsg, msgLen, 0, dest, destLen);
    if ((size_t)sendLen != msgLen)
    {
        free((void*)ns_addr_val);
        free((void*)bootmsg);
        return -1;
    }
    /* copia il corpo del messaggio, se richiesto */
    if (ns_addr_val != NULL)
    {
        *ns_addr_val = bootmsg->body;
        /* e lo fornisce */
        *ns_addr = ns_addr_val;
    }

    free((void*)bootmsg);

    return 0;
}

int
messages_get_boot_req_body(struct ns_host_addr** ns_addr,
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
            uint32_t ID,
            const struct ns_host_addr** peers,
            size_t nPeers)
{
    struct boot_ack* ans;
    size_t i;

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
    ans->head.pid = req->head.pid;
    /* prepara il corpo */
    ans->body.ID = ID;
    ans->body.length = nPeers;
    for (i = 0; i < nPeers; ++i)
        ans->body.neighbours[i] = *peers[i];

    *buffer = ans;
    *sz = sizeof(struct boot_ack);
    return 0;
}

int messages_check_boot_ack(void* buffer, size_t len)
{
    struct boot_ack *ack;

    if (buffer == NULL || len != sizeof(struct boot_ack))
        return -1;

    ack = (struct boot_ack*)buffer;

    /* controllo dell'header */
    if (ack->head.sentinel != 0 || ntohs(ack->head.type) != MESSAGES_BOOT_ACK)
        return -1;

    /* controllo del corpo */
    if (ack->body.length > MAX_NEIGHBOUR_NUMBER)
        return -1;

    return 0;
}

int
messages_get_boot_ack_body(
            const struct boot_ack* ack,
            uint32_t* ID,
            struct ns_host_addr** ns_addrs,
            size_t* addrN)
{
    int i, j, len;
    struct ns_host_addr* ns;

    if (ack == NULL || ID == NULL
        || ns_addrs == NULL || addrN == NULL)
        return -1;

    len = (int) ack->body.length;
    for (i = 0; i != len; ++i)
    {
        ns = malloc(sizeof(struct ns_host_addr));
        if (ns == NULL)
        {
            /* distrugge gli elementi precedenti */
            for (j = 0; j != i; ++j)
            {
                free(ns_addrs[j]);
                ns_addrs[j] = NULL;
            }
            /* segnala l'errore */
            return -1;
        }
        /* altrimenti copia il valore */
        *ns = ack->body.neighbours[i];
        ns_addrs[i] = ns;
    }
    *addrN = (size_t)len;
    *ID = ack->body.ID;

    return 0;
}

int messages_cmp_boot_ack_pid(const struct boot_ack* ack, uint32_t pid)
{
    if (ack == NULL)
        return -1;

    return ack->head.pid == pid;
}

int messages_get_pid(const void* head, uint32_t* pid)
{
    if (head == NULL || pid == NULL)
        return -1;

    *pid =  ((struct messages_head*)head)->pid;
    return 0;
}

int messages_make_shutdown_req(struct shutdown_req** req, size_t* sz, uint32_t ID)
{
    struct shutdown_req* ans;

    if (req == NULL || sz == NULL)
        return -1;

    ans = malloc(sizeof(struct shutdown_req));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct shutdown_req));

    /* non serve alcun pid perché non ha senso
     * pensare a dei duplicati */
    ans->head.type = htons(MESSAGES_SHUTDOWN_REQ);
    /* il corpo è estremamente semplice */
    ans->body.ID = htonl(ID);

    *req = ans;
    *sz = sizeof(struct shutdown_req);
    return 0;
}

int messages_check_shutdown_req(void* buffer, size_t len)
{
    struct shutdown_req* req;

    /* controllo parametri */
    if (buffer == NULL || len != sizeof(struct shutdown_req))
        return -1;

    req = (struct shutdown_req*)buffer;

    /* controllo dell'header */
    if (req->head.sentinel != 0 || ntohs(req->head.type) != MESSAGES_SHUTDOWN_REQ)
        return -1;

    return 0;
}

int messages_send_shutdown_req(int sockfd, const struct sockaddr* dest, socklen_t destLen, uint32_t ID)
{
    struct shutdown_req* req;
    size_t sz;

    if (dest == NULL || destLen == 0)
        return -1;

    if (messages_make_shutdown_req(&req, &sz, ID) == -1)
        return -1;

    if (sendto(sockfd, (void*)req, sz, 0, dest, destLen) != (ssize_t)sz)
    {
        free((void*)req);
        return -1;
    }
    free((void*)req);

    return 0;
}

int messages_send_shutdown_req_ns(int sockfd, const struct ns_host_addr* dest, uint32_t ID)
{
    struct sockaddr_storage ss;
    socklen_t ssLen;

    if (dest == NULL)
        return -1;

    if (sockaddr_from_ns_host_addr((struct sockaddr*)&ss, &ssLen, dest) == -1)
        return -1;

    return messages_send_shutdown_req(sockfd, (struct sockaddr*)&ss, ssLen, ID);
}

int messages_get_shutdown_req_body(const struct shutdown_req* req, uint32_t* ID)
{
    if (req == NULL || ID == NULL)
        return -1;

    *ID = ntohl(req->body.ID);
    return 0;
}

int messages_make_shutdown_ack(struct shutdown_ack** ack, size_t* sz, const struct shutdown_req* req)
{
    struct shutdown_req* ans;

    /* controllo parametri */
    if (ack == NULL || sz == NULL || req == NULL)
        return -1;

    ans = malloc(sizeof(struct shutdown_req));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct shutdown_req));

    ans->head.type = htons(MESSAGES_BOOT_ACK);
    ans->head.pid = req->head.pid;
    ans->body.ID = req->body.ID;

    *sz = sizeof(struct shutdown_req);
    *ack = ans;
    return 0;
}
