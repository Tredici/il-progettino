/** Questo file espone tutte le funzioni
 * che saranno utilizzate per gestire le
 * richieste dei peer giunte al server e
 * ricavare le informazioni su questi.
 */

#ifndef DS_PEERS
#define DS_PEERS

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

#endif
