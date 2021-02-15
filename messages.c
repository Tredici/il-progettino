#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <sys/uio.h>

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

uint32_t peer_data_extract_ID(const struct peer_data* ptr)
{
    uint32_t ID;

    if (peer_data_extract(ptr, &ID, NULL, NULL) == -1)
        return 0;

    return ID;
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

    if (buffer == NULL || bufLen != sizeof(struct check_req))
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
    ans->head.type = htons(MESSAGES_CHECK_ACK);
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

int messages_send_check_ack(
            int sockfd,
            const struct sockaddr* dest,
            socklen_t destLen,
            uint16_t port,
            uint8_t status,
            const struct peer_data* peer,
            const struct peer_data** neighbours,
            size_t length)
{
    struct check_ack* buffer;
    size_t bufLen;

    if (messages_make_check_ack(&buffer, &bufLen,
        port, status, peer, neighbours, length) != 0)
        return -1;

    if (sendto(sockfd, buffer, bufLen, 0, dest, destLen) != (ssize_t)bufLen)
    {
        free(buffer);
        return -1;
    }
    free(buffer);

    return 0;
}


int messages_check_check_ack(const void* buffer, size_t bufLen)
{
    struct check_ack* check;

    if (buffer == NULL || bufLen != sizeof(struct check_ack))
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

int messages_get_check_ack_body(
            const struct check_ack* ack,
            uint16_t* port,
            uint8_t* status,
            struct peer_data* peer,
            struct peer_data* neighbours,
            size_t* length)
{
    int i, limit;

    /* controllo parametri */
    if (ack == NULL || (!neighbours ^ !length))
        return -1;

    if (port != NULL)
        *port = ntohs(ack->body.port);

    if (status != NULL)
        *status = ack->body.status;

    if (!ack->body.status)
    {
        if (peer != NULL)
            *peer = ack->body.peer;

        if (neighbours != NULL)
        {
            limit = (int)ack->body.length;
            for (i = 0; i != limit; ++i)
                neighbours[i] = ack->body.neighbours[i];
            *length = (size_t)limit;
        }
    }

    return 0;
}

int messages_make_flood_req(
            struct flood_req** req,
            size_t* reqLen,
            uint32_t authID,
            uint32_t reqID,
            const struct tm* date,
            uint32_t length,
            uint32_t* signatures)
{
    struct flood_req* ans;
    size_t ansLen;
    size_t i;
    struct ns_tm ns_date;

    if (req == NULL || reqLen == NULL || date == NULL || (!length ^ !signatures))
        return -1;

    /* controllo che l'array di dimensione 0
     * abbia dimensione 0 */
    assert(sizeof(struct flood_req) == offsetof(struct flood_req, body.signatures));
    ansLen = sizeof(struct flood_req) + length*sizeof(uint32_t);
    ans = malloc(ansLen);
    if (ans == NULL)
        return -1;
    memset(ans, 0, ansLen);

    /* inizializza l'intestazione */
    ans->head.type = htons(MESSAGES_BOOT_ACK);

    /* inizializza il corpo */
    ans->body.authorID = htonl(authID);
    ans->body.reqID = htonl(reqID);
    if (time_init_ns_tm(&ns_date, date) == -1)
    {
        free(ans);
        return -1;
    }
    ans->body.date = ns_date; /* aggira il problema dell'allineamento */
    ans->body.length = htonl(length);
    for (i = 0; i != length; ++i) /* Riempie la "coda" */
        ans->body.signatures[i] = htonl(signatures[i]);

    *req = ans;
    *reqLen = ansLen;

    return 0;
}

int messages_send_flood_req(
            int sockFd,
            uint32_t authID,
            uint32_t reqID,
            const struct tm* date,
            uint32_t length,
            uint32_t* signatures)
{
    struct flood_req* req;
    size_t reqLen;

    if (sockFd < 0)
        return -1;

    if (messages_make_flood_req(&req, &reqLen,
        authID, reqID, date, length, signatures) == -1)
        return -1;

    if (send(sockFd, (void*)req, reqLen, 0) != (ssize_t)reqLen)
    {
        free(req);
        return -1;
    }

    free(req);
    return 0;
}

int messages_check_flood_req(
            const void* buffer,
            size_t bufLen)
{
    const struct flood_req* req;

    /* controllo che l'array di dimensione 0
     * abbia dimensione 0 */
    assert(sizeof(struct flood_req) == offsetof(struct flood_req, body.signatures));

    /* controllo sulla validità dei parametri;
     * attenzione al fatto che questi messaggi
     * abbiano dimensione variabile */
    if (buffer == NULL || bufLen < sizeof(struct flood_req))
        return -1;

    req = (const struct flood_req*)buffer;
    /* controllo dell'header */
    if (req->head.sentinel != 0 || req->head.type != htons(MESSAGES_BOOT_ACK))
        return -1;

    /* controlla il corpo */
    if (bufLen - offsetof(struct flood_req, body.signatures) != ntohl(req->body.length))
        return -1;

    return 0;
}

int messages_get_flood_req_body(
            const struct flood_req* req,
            uint32_t* authID,
            uint32_t* reqID,
            struct tm* date,
            uint32_t* length,
            uint32_t** signatures)
{
    uint32_t* arr;
    uint32_t i, tot;
    struct ns_tm ns_date;

    if (req == NULL || (length == NULL && signatures != NULL))
        return -1;

    /* valutazione campo per campo */
    if (signatures != NULL)
    {
        tot = ntohl(req->body.length);
        if (tot != 0)
        {
            arr = calloc(tot, sizeof(uint32_t));
            if (arr == NULL)
                return -1;
            for (i = 0; i != tot; ++i)
                arr[i] = ntohl(req->body.signatures[i]);
            *signatures = arr;
        }
        else
            *signatures = NULL;
        *length = tot;
    }
    else if (length != NULL)
        *length = ntohl(req->body.length);

    if (authID != NULL)
        *authID = ntohl(req->body.authorID);

    if (reqID != NULL)
        *reqID = ntohl(req->body.reqID);

    if (date != NULL)
    {
        ns_date = req->body.date;
        time_read_ns_tm(date, &ns_date);
    }

    return 0;
}

int messages_read_flood_req_body(
            int sockfd,
            uint32_t* authID,
            uint32_t* reqID,
            struct tm* date,
            uint32_t* sigNum,
            uint32_t** signatures)
{
    struct flood_req_body body;
    size_t bodyLen = sizeof(body);
    struct ns_tm ns_date;

    uint32_t* array = NULL;
    uint32_t i, lenght;

    /* integrità parametri */
    if (authID == NULL || reqID == NULL ||
        date == NULL || sigNum == NULL ||
        signatures == NULL)
        return -1;

    /* per debugging */
    if (recv(sockfd, (void*)&body, bodyLen, 0) != (ssize_t)bodyLen)
        return -1;
    /* lunhezza parte variabile */
    lenght = ntohl(body.length);
    if (lenght > 0)
    {
        /* alloca lo spazio necessario */
        array = calloc(lenght, sizeof(uint32_t));
        if (array == NULL) /* pasticcio! */
            return -1;

        /* legge la parte variabile */
        if (recv(sockfd, (void*)&array, (size_t)lenght, 0) != (ssize_t)lenght)
        {
            return -1;
        }
        /* mette in host order */
        for (i = 0; i != lenght; ++i)
            array[i] = htonl(array[i]);
    } /* altrimenti il corpo è vuoto */

    /* passaggio della data */
    ns_date = body.date; /* aggira il problema dell'allineamento */
    if (time_read_ns_tm(date, &ns_date) != 0)
        return -1;
    /* copia dei dati */
    *authID = ntohl(body.authorID);
    *reqID = ntohl(body.reqID);
    *signatures = array;
    *sigNum = lenght;

    return 0;
}

int messages_send_flood_ack(
            int sockFd,
            const struct flood_req* req,
            const struct e_register* R,
            const struct set* skip)
{
    struct flood_ack* ack;
    struct ns_entry* entryList;
    size_t entryLen;
    struct iovec iov[2];

    /* l'array in coda ha dimensione 0? */
    assert(sizeof(struct flood_ack) != offsetof(struct flood_ack, body.entry));
    if (sockFd < 0 || req == NULL)
        return -1;

    if (register_as_ns_array(R, &entryList, &entryLen, skip, NULL) == -1)
        return -1;
    iov[1].iov_base = entryList;
    iov[1].iov_len = entryLen * sizeof(struct ns_entry);

    ack = malloc(sizeof(struct flood_ack));
    if (ack == NULL)
    {
        free(entryList);
        return -1;
    }
    iov[0].iov_base = ack;
    iov[0].iov_len = sizeof(struct flood_ack);

    /* prepara l'header */
    memset(ack, 0, sizeof(struct flood_ack));
    ack->head.type = htonl(MESSAGES_REQ_ENTRIES);
    /* il corpo - parte copiata */
    ack->body.authorID = req->body.authorID;
    ack->body.reqID = req->body.reqID;
    ack->body.date = req->body.date;
    /* il corpo - parte nuova */
    ack->body.length = htonl(entryLen);

    /* invia i messaggi */
    if(writev(sockFd, iov, 2) != (ssize_t)(iov[0].iov_len + iov[1].iov_len))
    {
        free(ack);
        free(entryList);
        return -1;
    }

    free(ack);
    free(entryList);

    return 0;
}

int messages_make_req_data(
            struct req_data** req,
            size_t* reqLen,
            uint32_t authID,
            const struct query* query)
{
    struct req_data* ans;
    struct ns_query ns_query;

    if (req == NULL || reqLen == NULL || query == NULL)
        return -1;

    ans = malloc(sizeof(struct req_data));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct req_data));
    ans->head.type = htons(MESSAGES_REQ_DATA);

    ans->body.autorID = htonl(authID);
    if (initNsQuery(&ns_query, query) == -1)
    {
        free(ans);
        return -1;
    }
    ans->body.query = ns_query; /* aggira il problema dell'allineamento dei puntatori */

    *req = ans;
    *reqLen = sizeof(struct req_data);

    return 0;
}

int messages_send_req_data(
            int sockfd,
            uint32_t authID,
            const struct query* query)
{
    struct req_data* req;
    size_t reqLen;

    if (messages_make_req_data(&req, &reqLen, authID, query) == -1)
        return -1;

    if (send(sockfd, req, reqLen, 0) != (ssize_t)reqLen)
    {
        free((void*)req);
        return -1;
    }

    free((void*)req);
    return 0;
}

int messages_read_req_data_body(
            int sockfd,
            uint32_t* authID,
            struct query* query)
{
    struct req_data_body body;

    if (authID == NULL || query == NULL)
        return -1;

    if (recv(sockfd, (void*)&body, sizeof(body), MSG_DONTWAIT) != (ssize_t)sizeof(body))
        return -1;

    /* dovrebbe essere tutto ok */
    if (readNsQuery(query, &body.query) != 0)
        return -1;
    *authID = ntohl(body.autorID);

    return 0;
}

int messages_send_hello_req(int sockfd, uint32_t senderID, uint32_t receiverID)
{
    /* non serve allocare memoria a parte in questo caso */
    struct hello_req msg;

    memset(&msg, 0, sizeof(struct hello_req));
    /* inizializza l'header */
    msg.head.type = htons(MESSAGES_PEER_HELLO_REQ);
    /* inizializza il corpo del messaggio */
    msg.body.authID = htonl(senderID);
    msg.body.destID = htonl(receiverID);

    /* ora lo invia */
    if (send(sockfd, (void*)&msg, sizeof(struct hello_req), 0) != (ssize_t)sizeof(struct hello_req))
        return -1;

    return 0;
}

int messages_read_hello_req_body(
            int sockfd,
            uint32_t* senderID,
            uint32_t* receiverID)
{
    struct hello_req_body body;

    if (senderID == NULL || receiverID == NULL)
        return -1;

    if (recv(sockfd, (void*)&body, sizeof(body), MSG_DONTWAIT) != (ssize_t)sizeof(body))
        return -1;

    *senderID = ntohl(body.authID);
    *receiverID = ntohl(body.destID);
    return 0;
}

int messages_send_hello_ack(
            int sockfd,
            enum messages_hello_status status
            )
{
    /* non serve allocare memoria a parte in questo caso */
    struct hello_ack msg;

    memset(&msg, 0, sizeof(struct hello_ack));
    /* inizializza l'header */
    msg.head.type = htons(MESSAGES_PEER_HELLO_ACK);
    /* inizializza il corpo del messaggio */
    msg.body.status = htonl(status);

    /* ora lo invia */
    if (send(sockfd, (void*)&msg, sizeof(struct hello_ack), 0) != (ssize_t)sizeof(struct hello_ack))
        return -1;

    return 0;
}

int messages_read_hello_ack_body(
            int sockfd,
            enum messages_hello_status* status
            )
{
    struct hello_ack_body body;

    if (status == NULL)
        return -1;

    if (recv(sockfd, (void*)&body, sizeof(body), MSG_DONTWAIT) != (ssize_t)sizeof(body))
        return -1;

    *status = ntohl(body.status);
    return 0;
}

int messages_send_detatch_message(
            int sockfd,
            enum detatch_status status
            )
{
    struct message_detatch msg;

    memset(&msg, 0, sizeof(msg));
    /* prepara l'header */
    msg.head.type = htons(MESSAGES_DETATCH);
    /* prepara il corpo */
    msg.body.status = htonl(status);

    /* invia il messaggio */
    if (send(sockfd, (void*)&msg, sizeof(msg), 0) != (ssize_t)sizeof(msg))
        return -1;

    return 0;
}

int messages_send_empty_reply_data(
            int sockfd,
            enum messages_reply_data_status status,
            struct query* query
            )
{
    struct reply_data msg;
    /* lunghezza della parte da inviare */
    const size_t lenght = sizeof(msg);
    struct ns_query ns_query;

    /* non ha senso un messaggio vuoto in caso di successo */
    if (status == MESSAGES_REPLY_DATA_OK)
        return -1;
    memset(&msg, 0, sizeof(msg));
    /* prepara l'header */
    msg.head.type = htons(MESSAGES_REPLY_DATA);
    /* prepara il corpo */
    msg.body.status = htonl(status); /* stato */
    if (status == MESSAGES_REPLY_DATA_NOT_FOUND)
    {
        if (query == NULL)
            return -1;
        /* inserisce la query nel messaggio di risposta */
        if (initNsQuery(&ns_query, query) != 0)
            return -1;
        msg.body.answer.query = ns_query;
    }

    /* invia solo la parte iniziale del messaggio */
    if (send(sockfd, (void*)&msg, lenght, 0) != (ssize_t)lenght)
        return -1;

    return 0;
}

int messages_send_reply_data_answer(
            int sockfd,
            const struct answer* answer
            )
{
    struct reply_data msg;
    struct iovec iov[2];
    ssize_t total;

    if (answer == NULL)
        return -1;

    memset(&msg, 0, sizeof(msg));
    /* prepara la parte iniziale del messaggio - "maxi-header" */
    msg.head.type = htons(MESSAGES_REPLY_DATA);
    msg.body.status = htonl(MESSAGES_REPLY_DATA_OK);
    iov[0].iov_base = &msg;
    iov[0].iov_len = offsetof(struct reply_data, body.answer);

    /* prepara la seconda parte del messaggio */
    if (initNsAnswer((struct ns_answer**)&iov[1].iov_base, &iov[1].iov_len, answer) == -1)
        return -1;

    /* si prepara all'invio */
    total = (ssize_t)iov[0].iov_len + (ssize_t)iov[1].iov_len;
    if (writev(sockfd, iov, 2) != total)
    {
        free(iov[1].iov_base); /* libera il corpo della query */
        return -1;
    }

    free(iov[1].iov_base); /* libera il corpo della query */
    return 0;
}

int messages_read_reply_data_body(
            int sockfd,
            enum messages_reply_data_status* status,
            struct query** query,
            struct answer** answer
            )
{

    /* stato della risposta */
    enum messages_reply_data_status teStatus;
    struct reply_data_body msgBody;
    void* buffer; /* buffer ausiliario */
    void* data; /* puntatore alla parte del buffer che terrà la parte variabile */
    size_t length; /* lunghezza in byte della parte di lunghezza variabile */

    if (status == NULL || query == NULL || answer == NULL)
        return -1;

    /** Prova a leggere il corpo del messaggio, sfrutta il fatto
     * che il corpo del messaggio è sempre mandato intero */
    if (recv(sockfd, (void*)&msgBody, sizeof(msgBody), MSG_DONTWAIT) != (ssize_t)sizeof(msgBody))
        return -1;

    /* stato della rispsota */
    teStatus = ntohl(msgBody.status);
    switch (teStatus)
    {
    case MESSAGES_REPLY_DATA_ERROR:
        /* è andata male */
        *answer = NULL;
        *query = NULL;
        break;
    case MESSAGES_REPLY_DATA_NOT_FOUND:
        /* nessuna risposta ma la query c'è */
        /* prova ad allocare lo spazio per la risposta */
        buffer = malloc(sizeof(struct query));
        if (buffer == NULL)
            return -1;
        /* prova a leggere la query */
        if (readNsQuery((struct query*)buffer, &msgBody.answer.query) != 0)
        {
            free(buffer);
            return -1;
        }
        /* fornisce i puntatori alla chiamante */
        *query = (struct query*)buffer;
        *answer = NULL;
        break;
    case MESSAGES_REPLY_DATA_OK:
        /* Ricevuta una risposta valida! */
        /* byte nella parte di lunghezza variabile */
        length = (size_t)ntohl(msgBody.answer.lenght)*sizeof(msgBody.answer.data[0]);
        if (length == 0) /* questa parte non ha senso che sia 0 */
            return -1;
        /* alloca un buffer abbastanza grande per tenere tutti i dati */
        buffer = malloc(sizeof(struct ns_answer) + length);
        if (buffer == NULL)
            return -1;
        /* azzera tutto per sicurezza */
        memset(buffer, 0, sizeof(struct ns_answer) + length);
        /* inserisce nel buffer la parte iniziale della risposta */
        *(struct ns_answer*)buffer = msgBody.answer;
        /* puntatore dove salvare la parte fissa della variabile */
        data = &(((struct ns_answer*)buffer)->data[0]);
        /* legge quello che serve */
        if ((ssize_t)length != recv(sockfd, data, length, 0))
        {
            free(buffer);
            return -1;
        }
        /* ottiene la risposta in un formato "valido" */
        if (readNsAnswer(answer, buffer, sizeof(struct ns_answer) + length))
        {
            free(buffer);
            return -1;
        }
        /* ora il buffer non serve più */
        free(buffer);
        /* prepara il puntatore alla risposta e ha finito
         * soluzione "fantasiosa" dato che conosco il
         * formato delle strutture struct answer */
        *query = (struct query*)*answer;
        break;
    default:
        /* stato non riconosciuto */
        return -1;
    }
    /* fornisce lo stato al chiamante */
    *status = teStatus;

    return teStatus;
}
