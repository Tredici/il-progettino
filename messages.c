#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

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

/** Contiene i primi
 * due cambi di un messaggio
 * ben strutturato
 */
struct messages_head
{
    uint16_t sentinel;
    uint16_t type;
} __attribute__ ((packed));


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

/** Questa struttura definisce
 * il formato di una richiesta
 * di boot
 */
struct boot_req
{
    /* header */
    struct messages_head head;
    /* corpo: contenuto */
    struct ns_host_addr body;
} __attribute__ ((packed));

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
    if (ns_host_addr_get_ip_version(req) == 0) /* caso raro in cui 0 segnala l'errore */
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
