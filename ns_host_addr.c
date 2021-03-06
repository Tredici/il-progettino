#define _POSIX_C_SOURCE 201112L /* per getnameinfo */
#define _XOPEN_SOURCE 500 /* per snprintf */

#include "ns_host_addr.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

/** Due funzioni ausiliarie per scambiare in modo
 * sicuro via rete il formato degli indirizzi che
 * ci si invia via internet. */

/** Ottiene la versione di IP a partire
 * dalla famiglia dell'indirizzo.
 *
 * Il tipo utilizzato per il risultato
 * può essere trasferito in maniera sicura
 * attraverso la rete.
 *
 * Restituisce 0 in caso di errore.
 */
static uint8_t ipVersion(sa_family_t fam)
{
    switch (fam)
    {
    case AF_INET:
        return (uint8_t)4;

    case AF_INET6:
        return (uint8_t)6;

    default:
        /* 0 in caso di errore - scelta insolita ma necessaria */
        return 0;
    }
}

/** Fornisce la famiglia di indirizzo
 * a partire dalla versione di IP
 * specificata, è complementare a
 * ipVersion.
 *
 * Restituisce -1 in caso di errore.
 */
static sa_family_t ipFamily(uint8_t ver)
{
    switch (ver)
    {
    case 4:
        return AF_INET;
    case 6:
        return AF_INET6;

    default:
        /* idealmente mai raggiunto */
        return -1;
    }
}

int ns_host_addr_from_sockaddr(struct ns_host_addr* ns_addr, const struct sockaddr* sk_addr)
{
    if (ns_addr == NULL || sk_addr == NULL)
        return -1;

    switch (sk_addr->sa_family)
    {
    case AF_INET:
        memset(ns_addr, 0, sizeof(struct ns_host_addr));
        ns_addr->ip_version = ipVersion(sk_addr->sa_family);
        ns_addr->port = ((struct sockaddr_in*) sk_addr)->sin_port;
        ns_addr->ip.v4 = ((struct sockaddr_in*) sk_addr)->sin_addr;
        break;

    case AF_INET6:
        memset(ns_addr, 0, sizeof(struct ns_host_addr));
        ns_addr->ip_version = ipVersion(sk_addr->sa_family);
        ns_addr->port = ((struct sockaddr_in6*) sk_addr)->sin6_port;
        ns_addr->ip.v6 = ((struct sockaddr_in6*) sk_addr)->sin6_addr;
        break;

    default: /* tipo dell'indirizzo non riconosciuto */
        return -1;
    }

    return 0;
}

int sockaddr_from_ns_host_addr(struct sockaddr* sk_addr, socklen_t* sz, const struct ns_host_addr* ns_addr)
{
    if (ns_addr == NULL || sk_addr == NULL || sz == NULL)
        return -1;

    switch (ns_addr->ip_version)
    {
    case 4:
        /* azzera e assegna la dimensione del risultato */
        memset(sk_addr, 0, sizeof(struct sockaddr_in));
        *sz = sizeof(struct sockaddr_in);
        /* inizializza il risultato con i dati forniti */
        sk_addr->sa_family = ipFamily(ns_addr->ip_version);
        ((struct sockaddr_in*) sk_addr)->sin_port = ns_addr->port;
        ((struct sockaddr_in*) sk_addr)->sin_addr = ns_addr->ip.v4;
        break;

    case 6:
        /* azzera e assegna la dimensione del risultato */
        memset(sk_addr, 0, sizeof(struct sockaddr_in6));
        *sz = sizeof(struct sockaddr_in6);
        /* inizializza il risultato con i dati forniti */
        sk_addr->sa_family = ipFamily(ns_addr->ip_version);
        ((struct sockaddr_in6*) sk_addr)->sin6_port = ns_addr->port;
        ((struct sockaddr_in6*) sk_addr)->sin6_addr = ns_addr->ip.v6;
        break;

    default: /* tipo dell'indirizzo non riconosciuto */
        return -1;
    }

    return 0;
}

int ns_host_addr_as_string(char* buffer, size_t buffLen, const struct ns_host_addr* ns_addr)
{
    struct sockaddr_storage ss;
    socklen_t size = sizeof(struct sockaddr_storage);

    /* controllo validità dei parametri */
    if (buffer == NULL || buffLen == 0 || ns_addr == NULL)
        return -1;

    /* servono i dati in un formato sockaddr_* */
    if (sockaddr_from_ns_host_addr((struct sockaddr*)&ss, &size, ns_addr) == -1)
        return -1;

    return sockaddr_as_string(buffer, buffLen, (const struct sockaddr*)&ss, size);
}

int sockaddr_as_string(char* buffer, size_t buffLen,
        const struct sockaddr* sk_addr, socklen_t skLen)
{
    char testBuffer[64];
    int testLen;
    char hostname[56];
    char portname[8];

    if (sk_addr == NULL || skLen == 0 || ipVersion(sk_addr->sa_family) == 0)
        return -1;

    if (getnameinfo(sk_addr, skLen,
            hostname, sizeof(hostname),
            portname, sizeof(portname), NI_NUMERICSERV) != 0)
        return -1;

    testLen = snprintf(testBuffer, sizeof(testBuffer),
                "[ipv%d]%s:%s", (int)ipVersion(sk_addr->sa_family),
                hostname, portname);
    if (testLen == -1 || (size_t)testLen > buffLen)
        return -1;

    strcpy(buffer, testBuffer);

    return testLen;
}

int ns_host_addr_set_port(struct ns_host_addr* ns_addr, uint16_t port)
{
    if (ns_addr == NULL)
        return -1;

    ns_addr->port = htons(port);
    return 0;
}

int ns_host_addr_get_port(const struct ns_host_addr* ns_addr, uint16_t* port)
{
    if (ns_addr == NULL || port == NULL)
        return -1;

    *port = ntohs(ns_addr->port);
    return 0;
}

uint8_t ns_host_addr_get_ip_version(const struct ns_host_addr* ns_addr)
{
    if (ns_addr == NULL)
        return 0;

    return ns_addr->ip_version;
}

sa_family_t ns_host_addr_get_ip_family(const struct ns_host_addr* ns_addr)
{
    if (ns_addr == NULL)
        return 0;

    return ipFamily(ns_addr->ip_version);
}

int ns_host_addr_any(const struct ns_host_addr* ns_addr)
{
    struct in_addr ipv4any = { INADDR_ANY };

    if (ns_addr == NULL)
        return -1;

    switch (ns_addr->ip_version)
    {
    case 4:
        /* IPv4 */
        return memcmp(&ns_addr->ip.v4, &ipv4any, sizeof(ipv4any)) == 0;
    case 6:
        /* IPv6 */
        return memcmp(&ns_addr->ip.v6, &in6addr_any, sizeof(in6addr_any)) == 0;

    default:
        return -1;
    }
}

int ns_host_addr_update_addr(struct ns_host_addr* ns_addr, const struct sockaddr* sk_addr)
{
    struct ns_host_addr ans;
    uint16_t port;

    if (ns_addr == NULL || sk_addr == NULL)
        return -1;

    if (ns_host_addr_from_sockaddr(&ans, sk_addr) == -1)
        return -1;

    if (ns_host_addr_get_port(ns_addr, &port) == -1)
        return -1;

    if (ns_host_addr_set_port(&ans, port) == -1)
        return -1;

    *ns_addr = ans;

    return 0;
}

int ns_host_addr_send(int socketfd, const void* buffer, size_t len, int flag, const struct ns_host_addr* ns_addr)
{
    struct sockaddr_storage ss;
    socklen_t ssLen;

    if (buffer == NULL || len == 0 || ns_addr == NULL)
        return -1;

    if (sockaddr_from_ns_host_addr((struct sockaddr*)&ss, &ssLen, ns_addr) == -1)
        return -1;

    if (sendto(socketfd, buffer, len, flag, (struct sockaddr*)&ss, ssLen) != (ssize_t)len)
        return -1;

    return 0;
}

int connect_to_ns_host_addr(const struct ns_host_addr* ns_addr)
{
    struct sockaddr_storage ss;
    socklen_t ssLen;
    int sk;

    if (ns_addr == NULL)
        return -1;

    /* cerca di inizializzare le strutture dati per connettersi */
    if (sockaddr_from_ns_host_addr((struct sockaddr*)&ss, &ssLen, ns_addr) != 0)
        return -1;

    switch (ns_host_addr_get_ip_family(ns_addr))
    {
    case AF_INET: /* IPv4 */
        sk = socket(AF_INET, SOCK_STREAM, 0);
        break;
    case AF_INET6: /* IPv6 */
        sk = socket(AF_INET6, SOCK_STREAM, 0);
        break;
    default:
        return -1;
    }
    /* controlla che il socket sia stato creato correttamente */
    if (sk == -1)
        return -1;
    /* cerca di connettersi */
    if (connect(sk, (struct sockaddr*)&ss, ssLen) != 0)
    {
        close(sk);
        return -1;
    }
    return sk;
}
