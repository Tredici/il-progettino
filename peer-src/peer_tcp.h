/** Funzioni per la gestione del sottosistema TCP
 * per la comunicazione trai peer.
 */


#ifndef PEER_TCP
#define PEER_TCP

/** Prepara il sottosistema TCP all'avvio ma non
 * lo avvia!
 * Sarà avviato solo dopo che il peer si sarà
 * connesso al network.
 * Crea il socket tcp e lo mette in ascolto per
 * connessioni sulla porta specificata.
 *
 * Va chiamato dal thread principale all'inizio.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int peer_tcp_init(int port);

/** Stoppa il sottosistema tcp.
 * Va chiamato dal thread principale al
 * termine dell'esecuzione del programma
 * e DOPO la chiusura del sottosistema
 * UDP.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int peer_tcp_close(void);

/** Fornisce il descrittore del socket
 * tcp oppure -1 in caso di errore.
 */
int peer_tcp_get_socket(void);


#endif
