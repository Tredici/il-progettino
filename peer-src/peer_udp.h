/** Codice del thread per gestire e
 * interagire con il thread che gestisce
 * il socket udp
 */

#ifndef PEER_UDP
#define PEER_UDP

/** Avvia il thread a gestione del socket
 * UDP e restituisce il numero di porta
 * su cui il processo ascolta.
 *
 * In caso di errore ritorna -1;
 */
int UDPstart(int);

/** Termina il processo che gestisce il
 * socket UDP
 *
 * In caso di errore ritorna -1;
 */
int UDPstop(void);


#endif
