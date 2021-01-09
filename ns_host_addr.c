#define _POSIX_C_SOURCE 201112L /* per getnameinfo */
#define _XOPEN_SOURCE 500 /* per snprintf */

#include "ns_host_addr.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

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
        ns_addr->ip_version = ipVersion(sk_addr->sa_family);
        ns_addr->port = ((struct sockaddr_in*) ns_addr)->sin_port;
        ns_addr->ip.v4 = ((struct sockaddr_in*) ns_addr)->sin_addr;
        break;

    case AF_INET6:
        ns_addr->ip_version = ipVersion(sk_addr->sa_family);
        ns_addr->port = ((struct sockaddr_in6*) ns_addr)->sin6_port;
        ns_addr->ip.v6 = ((struct sockaddr_in6*) ns_addr)->sin6_addr;
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
