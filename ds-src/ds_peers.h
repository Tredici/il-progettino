/** Questo file espone tutte le funzioni
 * che saranno utilizzate per gestire le
 * richieste dei peer giunte al server e
 * ricavare le informazioni su questi.
 */

#ifndef DS_PEERS
#define DS_PEERS

#include "../ns_host_addr.h"
#include "../messages.h" /* per struct peer_data */

/** Inizializza il sottosistema a gestione
 * dei peers.
 *
 * Va chiamato dal thread principale e
 * NECESSARIAMENTE all'inizio, prima di
 * inizializzare il sottosistema UPD.
 */
int peers_init(void);

/** Ripulisce tutte le risorse allocate
 * dal sottosistema a gestione dei peers.
 *
 * Va chiamato dal thread principale alla
 * fine del lavoro.
 */
int peers_clear(void);

/** Aggiunge un peer all'insieme di quelli
 * "tracciati" e ne restituisce i vicini.
 *
 * Il primo argomento serve a intercettare
 * le richieste ricevute come duplicate.
 *
 * Il secondo argomento identifica il nuovo
 * peer, il terzo permette di ottenere
 * l'ID fornito dal sistema al nuovo peer
 * e gli ultimi due argomenti permetto
 * di ottenere  rispettivamente un elenco
 * di peer, ottenibile mediante un array
 * di puntatori a struct ns_host_addr che
 * deve essere già stato precedentemente
 * allocato dal chiamante, e la lunghezza
 * di questo.
 */
int
peers_add_and_find_neighbours(
            uint32_t loginPid, /* pid della richiesta */
            const struct ns_host_addr* ns_addr, /* indirizzo mittente */
            const struct ns_host_addr* ns_tcp, /* indirizzo nel messaggio */
            uint32_t* newID, /* pseudo ID fornito */
            const struct peer_data** neighbours, /* lista dei vicini */
            uint16_t* length); /* numero dei vicini */

/** Trova i vicini di un peer dato il suo
 * identificativo (coincidente con il suo
 * numero di porta in questo sistema 
 * semplificato).
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int peers_find_neighbours(long int, const struct peer_data**, uint16_t*);

/** Stampa l'elenco dei peer al momento
 * connessi insieme ai loro dati.
 */
int peers_showpeers(void);

/** Se l'argomento è NULL mostra i vicini
 * di ogni peer, altrimenti mostra i
 * vicini solo del peer indicato.
 */
int peers_showneighbour(long int*);

/** Invia a tutti i peer un messaggio
 * di shutdown utilizzando il socket
 * fornito come argomento.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int peers_send_shutdown(int);

/** Restituisce semplicemente
 * il numero di peer.
 *
 * Restituisce -1 in caso di
 * errore e il numero di peer
 * "connessi" altrimenti.
 */
int peers_number(void);

/** Ottiene l'ID del peer (se esiste)
 * identificato dalla chiave data
 * (che in questa semplice implementa
 * zione coincide con il numero di porta
 * utilizzato dal socket UDP).
 *
 * Restituisce 0 in caso di successo
 * oppure -1 in caso di errore (ad
 * esempio se non esiste alcun peer
 * identificato da tale numero di porta)
 */
int peers_get_id(long int, uint32_t*);

/** Rimuove il peer identificato dalla
 * chiave fornita.
 *
 * Restituisce 0 in caso di successo
 * oppure -1 in caso di errore.
 */
int peers_remove_peer(long int);

#endif
