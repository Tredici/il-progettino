/** Codice per attivare il sttosistema
 * udp del server.
 *
 * A differenza di quello per i peer
 * questo è molto di meno poiché il
 * server è per sua natura (quasi)
 * completamente passivo.
 */

#ifndef DS_UDP
#define DS_UDP

/** Avvia il thread a gestione del socket
 * UDP e restituisce il numero di porta
 * su cui il thread ascolta.
 *
 * L'argomento è il numero di porta da
 * utilizzare oppure 0 se una qualsiasi
 * porta va bene.
 *
 * In caso di errore ritorna -1;
 */
int UDPstart(int);

/** Termina il thread che gestisce il
 * socket UDP avviando tutta la fase
 * di terminazione dei peer.
 *
 * In caso di errore ritorna -1;
 */
int UDPstop(void);


#endif
