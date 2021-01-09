#include "ns_host_addr.h"
#include <stdlib.h>

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
