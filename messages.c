#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

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

struct peer_data*
peer_data_init(
            struct peer_data* ptr,
            uint32_t ID,
            uint16_t order,
            const struct ns_host_addr* ns_addr)
{
    struct peer_data* ans;

    /* controllo parametri */
    if (ns_addr == NULL)
        return NULL;

    if (ptr == NULL)
        ans = malloc(sizeof(struct peer_data));
    else
        ans = ptr;
    memset(ans, 0, sizeof(struct peer_data));
    ans->ID = htonl(ID);
    ans->order = htons(order);
    ans->ns_addr = *ns_addr;

    return ans;
}

int peer_data_extract(
            const struct peer_data* ptr,
            uint32_t* ID,
            uint16_t* order,
            struct ns_host_addr* ns_addr)
{
    if (ptr == NULL)
        return -1;

    if (ID != NULL)
        *ID = ntohl(ptr->ID);
    if (order != NULL)
        *order = ntohs(ptr->order);
    if (ns_addr != NULL)
        *ns_addr = ptr->ns_addr;

    return 0;
}

char* peer_data_as_string(const struct peer_data* pd, char* buffer, size_t bufLen)
{
    char tmpBuf[64];
    char addrBuf[32];
    int err;
    uint32_t ID;
    uint16_t port;
    struct ns_host_addr addr;
    char* ans;

    if (pd == NULL)
        return NULL;

    if (peer_data_extract(pd, &ID, &port, &addr) == -1)
        return NULL;

    if (ns_host_addr_as_string(addrBuf, sizeof(addrBuf), &addr) == -1)
        return NULL;

    err = sprintf(tmpBuf, "[%d](%d)%s", (int)ID, (int)port, addrBuf);
    if (err == -1)
        return NULL;
    if (buffer != NULL)
    {
        if ((size_t)err+1 > bufLen)
            return NULL;

        ans = strcpy(buffer, tmpBuf);
    }
    else
        ans = strdup(tmpBuf);

    return ans;
}

int recognise_messages_type(const void* msg)
{
    struct messages_head* p;

    if (msg == NULL)
        return -1;

    p = (struct messages_head*)msg;
    if (p->sentinel != 0)
        return MESSAGES_MSG_UNKWN;

    return (int)ntohs(p->type);
}

void* messages_clone(const void* msg)
{
    void* ans;

    switch (recognise_messages_type(msg))
    {
    case MESSAGES_BOOT_REQ:
        ans = malloc(sizeof(struct boot_req));
        if (ans == NULL)
            return NULL;

        *(struct boot_req*)ans = *(struct boot_req*)msg;
        break;

    case MESSAGES_BOOT_ACK:
        ans = malloc(sizeof(struct boot_ack));
        if (ans == NULL)
            return NULL;

        *(struct boot_ack*)ans = *(struct boot_ack*)msg;
        break;

    case MESSAGES_SHUTDOWN_REQ:
        ans = malloc(sizeof(struct shutdown_req));
        if (ans == NULL)
            return NULL;

        *(struct shutdown_req*)ans = *(struct shutdown_req*)msg;
        break;

    case MESSAGES_SHUTDOWN_ACK:
        ans = malloc(sizeof(struct shutdown_ack));
        if (ans == NULL)
            return NULL;

        *(struct shutdown_ack*)ans = *(struct shutdown_ack*)msg;
        break;

    default:
        return NULL;
    }

    return ans;
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


int messages_check_boot_req(const void* buffer, size_t len)
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
            const struct peer_data** peers,
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
    ans->head.type = htons(MESSAGES_BOOT_ACK);
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

int messages_check_boot_ack(const void* buffer, size_t len)
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
            struct peer_data** neighbours,
            size_t* addrN)
{
    int i, j, len;
    struct peer_data* peer;

    if (ack == NULL || ID == NULL
        || neighbours == NULL || addrN == NULL)
        return -1;

    len = (int) ack->body.length;
    for (i = 0; i != len; ++i)
    {
        peer = malloc(sizeof(struct peer_data));
        if (peer == NULL)
        {
            /* distrugge gli elementi precedenti */
            for (j = 0; j != i; ++j)
            {
                free(neighbours[j]);
                neighbours[j] = NULL;
            }
            /* segnala l'errore */
            return -1;
        }
        /* altrimenti copia il valore */
        *peer = ack->body.neighbours[i];
        neighbours[i] = peer;
    }
    *addrN = (size_t)len;
    *ID = ack->body.ID;

    return 0;
}

int
messages_get_boot_ack_body_cp(
            const struct boot_ack* ack,
            uint32_t* ID,
            struct peer_data* neighboursArray,
            size_t* addrN)
{
    struct peer_data* neighbours[MAX_NEIGHBOUR_NUMBER];
    int i, limit;

    if (messages_get_boot_ack_body(ack, ID, neighbours, addrN) == -1)
        return -1;

    limit = (int)*addrN;
    for (i = 0; i != limit; ++i)
    {
        neighboursArray[i] = *neighbours[i];
        free((void*)neighbours[i]);
    }

    return 0;
}

int messages_cmp_boot_ack_pid(const struct boot_ack* ack, uint32_t pid)
{
    if (ack == NULL)
        return -1;

    return ntohl(ack->head.pid) == pid;
}

int messages_get_pid(const void* head, uint32_t* pid)
{
    if (head == NULL || pid == NULL)
        return -1;

    *pid =  ntohl(((struct messages_head*)head)->pid);
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

int messages_check_shutdown_req(const void* buffer, size_t len)
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
    struct shutdown_ack* ans;

    /* controllo parametri */
    if (ack == NULL || sz == NULL || req == NULL)
        return -1;

    ans = malloc(sizeof(struct shutdown_ack));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct shutdown_ack));

    ans->head.type = htons(MESSAGES_SHUTDOWN_ACK);
    ans->head.pid = req->head.pid;
    ans->body.ID = req->body.ID;

    *sz = sizeof(struct shutdown_ack);
    *ack = ans;
    return 0;
}

int messages_check_shutdown_ack(const void* buffer, size_t len)
{
    struct shutdown_ack* ack;

    /* controllo parametri */
    if (buffer == NULL || len != sizeof(struct shutdown_ack))
        return -1;

    ack = (struct shutdown_ack*)buffer;

    /* controllo dell'header */
    if (ack->head.sentinel != 0 || ntohs(ack->head.type) != MESSAGES_SHUTDOWN_ACK)
        return -1;

    return 0;
}

int messages_get_shutdown_ack_body(const struct shutdown_ack* ack, uint32_t* ID)
{
    if (ack == NULL || ID == NULL)
        return -1;

    *ID = ntohl(ack->body.ID);
    return 0;
}

int messages_send_shutdown_response(
            int sockfd,
            const struct sockaddr* sk_addr,
            socklen_t skLen,
            const struct shutdown_req* req)
{
    struct shutdown_ack* ack;
    size_t ackLen;

    /* controllo parametri */
    if (sockfd < 0 || sk_addr == NULL || skLen == 0 || req == NULL)
        return -1;

    if (messages_make_shutdown_ack(&ack, &ackLen, req) == -1)
        return -1;

    if (sendto(sockfd, (void*)ack, ackLen, 0, sk_addr, skLen) != (ssize_t)ackLen)
    {
        free((void*)ack);
        return -1;
    }
    free((void*)ack);

    return 0;
}

int messages_check_check_req(const void* buffer, size_t bufLen)
{
    struct check_req* check;

    if (buffer == NULL || bufLen == sizeof(struct check_req))
        return -1;

    check = (struct check_req*)buffer;
    if (check->head.sentinel != 0 || ntohs(check->head.type) != MESSAGES_CHECK_REQ)
        return -1;

    return 0;
}

int messages_make_check_req(struct check_req** buffer, size_t* sz, uint16_t port)
{
    struct check_req* ans;

    if (buffer == NULL || sz == NULL)
        return -1;

    ans = malloc(sizeof(struct check_req));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct check_req));
    ans->head.type = htons(MESSAGES_CHECK_REQ);
    ans->body.port = htons(port);

    *buffer = ans;
    *sz = sizeof(struct check_req);

    return 0;
}

int
messages_get_check_req_body(
            const struct check_req* req,
            uint16_t* port)
{
    if (req == NULL || port == NULL)
        return -1;

    *port = ntohs(req->body.port);
    return 0;
}

int
messages_send_check_req(
            int sockfd,
            const struct sockaddr* dest,
            socklen_t destLen,
            uint16_t port)
{
    struct check_req* req;
    size_t len;

    if (messages_make_check_req(&req, &len, port) == -1)
        return -1;

    if (sendto(sockfd, (void*)req, len, 0, dest, destLen) == -1)
    {
        free((void*)req);
        return -1;
    }

    free((void*)req);
    return 0;
}

int messages_make_check_ack(
            struct check_ack** buffer,
            size_t* bufLen,
            uint16_t port,
            uint8_t status,
            const struct peer_data* peer,
            const struct peer_data** neighbours,
            size_t length)
{
    struct check_ack* ans;
    int i;

    if (bufLen == NULL || bufLen == NULL)
        return -1;


    if (!status && (
        peer == NULL
        || (neighbours == NULL && length != 0)
        || length > MAX_NEIGHBOUR_NUMBER))
        return -1;

    ans = malloc(sizeof(struct check_ack));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct check_ack));
    /* prepara l'header */
    ans->head.type = htons(MESSAGES_SHUTDOWN_ACK);
    /* prepara il corpo */
    ans->body.port = htons(port);
    ans->body.status = status;

    /* solo in caso di stato nullo (tutto ok) */
    if (!status)
    {
        /* inserisce i dati del peer */
        ans->body.peer = *peer;
        /* inserisce i vicini */
        ans->body.length = length;
        for (i = 0; i != (int)length; ++i)
        {
            if (neighbours[i] == NULL)
            {
                free(ans);
                return -1;
            }
            ans->body.neighbours[i] = *neighbours[i];
        }
    }

    *bufLen = sizeof(struct check_ack);
    *buffer = ans;

    return 0;
}

int messages_check_check_ack(const void* buffer, size_t bufLen)
{
    struct check_ack* check;

    if (buffer == NULL || bufLen == sizeof(struct check_ack))
        return -1;

    check = (struct check_ack*)buffer;
    if (check->head.sentinel != 0 || ntohs(check->head.type) != MESSAGES_CHECK_ACK)
        return -1;

    /* controllo sul corpo del messaggio */
    if (check->body.length > MAX_NEIGHBOUR_NUMBER)
        return -1;

    return 0;
}

int messages_get_check_ack_status(const struct check_ack* ack)
{
    return ack == NULL ? -1 : ack->body.status;
}
