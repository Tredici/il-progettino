/** Funzioni per la gestione del sottosistema TCP
 * per la comunicazione trai peer.
 */


#ifndef PEER_TCP
#define PEER_TCP

#include "../messages.h"

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
int TCPinit(int port);

/** Attiva il thread TCP fornendogli i
 * dati dei neighbour iniziali.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int TCPrun(uint32_t ID, struct peer_data* neighbours, size_t N);

/** Stoppa il sottosistema tcp.
 * Va chiamato dal thread principale al
 * termine dell'esecuzione del programma
 * e DOPO la chiusura del sottosistema
 * UDP.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int TCPclose(void);

/** Fornisce il descrittore del socket
 * tcp oppure -1 in caso di errore.
 */
int TCPgetSocket(void);

/** Invia ai vicini un messaggio di tipo
 * REQ_DATA per chiedere loro se hanno
 * già calcolato la query fornita.
 *
 * Se non si riceve una risposta entro 5
 * secondi termina con un errore.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int TCPreqData(const struct query* query);

/** Avvia una esecuzione del protocollo
 * FLOODING per ricercare tutte le entry
 * che dovrebbero appartenere alla data
 * indicata.
 *
 * Restituisce 0 in caso di successe e -1
 * in caso di errore.
 */
int TCPstartFlooding(const struct tm* date);

/** Sospende il chiamante per un po' fino
 * a che tutte le istanze del protocollo
 * FLOODING iniziate dal chiamante non
 * sono terminate - oppure scade il
 * timeout imposto.
 *
 * Ritorna 0 in caso di successo (tutto
 * è terminato correttamente) e -1 in
 * caso di timeout.
 */
int TCPendFlooding(void);

#endif
