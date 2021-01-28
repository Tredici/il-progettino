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
#include <sys/socket.h>

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
} __attribute__ ((packed));

/** Inizializza un oggetto struct ns_host_addr
 * a partire da un oggetto struct sockaddr
 *
 * Il primo argomento, utilizzato per fornire
 * il risultato, non può essere NULL.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 */
int ns_host_addr_from_sockaddr(struct ns_host_addr*, const struct sockaddr*);

/** Inizializza un oggetto struct sockaddr,
 * ottenendone anche la dimensione,
 * a partire da un oggetto struct ns_host_addr
 *
 * I primi due argomenti, utilizzati per fornire
 * i risultati, non possono essere NULL.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 */
int sockaddr_from_ns_host_addr(struct sockaddr*, socklen_t*, const struct ns_host_addr*);

/** Fornisce tramite il puntatore a char fornito
 * una rappresentazione in formato stringa
 * della struttura ns_host_addr.
 *
 * La dimensione dell'array fornito è data dal
 * secondo parametro.
 *
 * Restituisce -1 in caso di errore, oppure il
 * numero di caratteri scritti nell'array dato
 * MENO il carattere '\0' di terminazione.
 *
 * Il formato utilizzato è:
 *      [ip(versione)](hostname):(porta)
 */
int ns_host_addr_as_string(char*, size_t, const struct ns_host_addr*);

/** Fornisce tramite il puntatore a char fornito
 * una rappresentazione in formato stringa
 * della struttura sockaddr data.
 *
 * La dimensione dell'array fornito è data dal
 * secondo parametro.
 *
 * Restituisce -1 in caso di errore, oppure il
 * numero di caratteri scritti nell'array dato
 * MENO il carattere '\0' di terminazione.
 *
 * Il formato utilizzato è:
 *      [ip(versione)](hostname):(porta)
 */
int sockaddr_as_string(char*, size_t, const struct sockaddr*, socklen_t);

/** Cambia il valore della porta della
 * struct ns_host_addr con quello fornito in
 * input.
 *
 * Il valore fornito rispetta l'endianness
 * dell'host.
 *
 * Restituisce -1 in caso di errore e 0 in caso
 * di successo.
 */
int ns_host_addr_set_port(struct ns_host_addr*, uint16_t);

/** Fornisce il valore della porta della
 * struct ns_host_addr con quello fornito in
 * input.
 *
 * Il valore fornito rispetta l'endianness
 * dell'host.
 *
 * Restituisce -1 in caso di errore e 0 in caso
 * di successo.
 *
 * NOTA: mi trovo a usare un argomento per
 * restituire un valore poiché su alcune macchine
 * gli interi sono da 16 bit e vorrei rendere
 * l'interfaccia di questa funzione portabile.
 */
int ns_host_addr_get_port(const struct ns_host_addr*, uint16_t*);

/** Ottiene la versione di IP specificata
 * nell'oggetto struct ns_host_addr.
 *
 * Restituisce 0 in caso di errore.
 */
uint8_t ns_host_addr_get_ip_version(const struct ns_host_addr*);

/** Ottiene la famiglia di indirizzo IP
 * nell'oggetto struct ns_host_addr.
 *
 * Restituisce -1 in caso di errore.
 */
sa_family_t ns_host_addr_get_ip_family(const struct ns_host_addr*);

/** Verifica se l'indirizzo specificato
 * è 0.0.0.0 o 0::0.
 * Restituisce 1 se è l'indirizzo di
 * loopback e 0 se è un altro.
 *
 * Restituisce -1 in caso di errore.
 */
int ns_host_addr_any(const struct ns_host_addr*);

/** Può essere necessario cambiare
 * l'indirizzo IP indicato in una
 * struct ns_host_addr. Questa funzione
 * lo fa prendendo l'indirizzo dall'oggetto
 * struct sockaddr fornito e inserendolo
 * nella struttura fornita.
 *
 * Restituisce 0 in caso di successo,
 * -1 in caso di errore.
 */
int ns_host_addr_update_addr(struct ns_host_addr*, const struct sockaddr*);

/** Simula il comportamento di sendto
 * utilizzando per identificare la
 * destinazione.
 * Tutti gli argomenti corrispondono
 * a quelli di sendto eccetto l'ultimo
 * che sostituisce i due parametri per
 * identificare la destinazione.
 *
 * Restituisce 0 in caso di successo
 * (se riesce a inviare tanti byte
 * quanti sono quelli richiesti)
 * oppure -1 in caso di errore.
 */
int ns_host_addr_send(int, const void*, size_t, int, const struct ns_host_addr*);

/** Crea un socket TCP e cerca di
 * connetterlo all'indirizzo fornito
 * tramite l'oggetto struct ns_host_addr.
 *
 * Restituisce il file descriptor
 * del nuovo socket in caso di
 * successo e -1 in caso di errore.
 */
int connect_to_ns_host_addr(const struct ns_host_addr*);

#endif
