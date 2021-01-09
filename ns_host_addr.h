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
static uint8_t ipVersion(sa_family_t);

/** Fornisce la famiglia di indirizzo
 * a partire dalla versione di IP
 * specificata, è complementare a
 * ipVersion.
 *
 * Restituisce -1 in caso di errore.
 */
static sa_family_t ipFamily(uint8_t);

#endif
