/** Codice del thread per gestire e
 * interagire con il thread che gestisce
 * il socket udp
 */

#ifndef PEER_UDP
#define PEER_UDP

#include "../messages.h"

/** Avvia il thread a gestione del socket
 * UDP e restituisce il numero di porta
 * su cui il processo ascolta.
 *
 * L'argomento è il numero di porta da
 * utilizzare oppure 0 se qualsiasi
 * porta va bene.
 *
 * In caso di errore ritorna -1;
 */
int UDPstart(int);

/** Metodo utilizzabile SOLO dal thread
 * principale!
 *
 * Prova a connettere il peer al network
 * contattando il server usando
 * l'indirizzo e la porta forniti.
 */
int UDPconnect(const char*, const char*);

/** Termina il processo che gestisce il
 * socket UDP
 *
 * In caso di errore ritorna -1;
 */
int UDPstop(void);

/** Fornisce la porta attualmente utilizzata
 * dal sottosistema UDP, oppure 0 se questo
 * non è attivo.
 */
int UDPport(void);

/** Cerca di contattare il server per
 * scoprire quali siano ora i vicini
 * del peer.
 *
 * Il tutto è svolto mediante l'impiego
 * di messaggi di tipo
 * MESSAGES_CHECK_REQ e MESSAGES_CHECK_ACK.
 *
 * Riempie l'array neighbours con le
 * informazioni sui peer fornite dal
 * server, il numero di vicini è
 * inserito nella variabile puntata
 * da length.
 *
 * Restituisce 0 in caso di successo e
 * -1 in caso di errore (es. timeout).
 */
int UDPcheck(struct peer_data* neighbours, size_t* length);

#endif
