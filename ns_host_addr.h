/** ns sta per "network safe"
 */

/** Bisogna fare utilizzo dell'estensione
 * di GCC __attribute__ ((packed)) e di
 * dipi interi di lunghezza fissa per
 * rendere il formato della struttura
 * indipendente dalle convezioni seguite
 * dall'host
 */

#ifndef NS_HOST_ADDR
#define NS_HOST_ADDR

#include <arpa/inet.h>

/** Questa struttura raccoglie in
 * un formato trasferibile in modo
 * sicuro via rete le informazioni
 * per contattare un dato host
 */
struct ns_host_addr
{
    uint8_t ip_version; /* versione di ip - 4 o 6 */
    uint16_t port; /* porta utilizzata - network order */
    /* cosa che mi tocca fare per poter trasferire
     * indirizzi sia ipV4 sia ipV6 */
    union boot_req_ip_addr
    {
        /* questa cosa funziona solo perché
         * queste informazioni sono di default
         * in network order */
        struct in_addr v4;
        struct in6_addr v6;
    } ip __attribute__ ((packed));
};


#endif
