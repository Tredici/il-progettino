/**
 * Funzioni generiche per creare socket per client e server
 */

#ifndef SOCKET_UTILS
#define SOCKET_UTILS

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/** Crea e "ancora" un socket UDP sulla porta
 * specificata - oppure su una porta
 * casuale disponibile se viene fornito 0.
 *
 * Restituisce il corrispondente file
 * descriptor in caso di successo e -1
 * in caso di errore.
 */
int initUDPSocket(int);

/** Crea e ancora un socket del tipo
 * specificato sulla porta fornita.
 *
 * Restituisce il corrispondente file
 * descriptor in caso di successo e -1
 * in caso di errore.
 */
int initSocket(int, int);

int getSocketPort(int);
/** Riempie una struttura sockaddr
 * (insieme al corrispondente socklen_t)
 * con le informazioni per raggiungere
 * un host remoto utilizzando l'hostname
 * e la porta specificati nel terzo e
 * quarto argomento.
 *
 * Gli argomenti quinto e sesto se non 0
 * specificano rispettivamente:
 *  +socket type richiesto:
 *          ex. SOCK_STREAM SOCK_DGRAM
 *  +socket protocol richiesto:
 *      (vedi /etc/protocols e socket(2))
 *
 * Restituisce 0 in caso di successo, -1
 * in caso di errore.
 */
int getSockAddr(struct sockaddr*, socklen_t*, const char*, const char*, int, int);

/** Estrae il numero di porta da un oggetto
 * di tipo struct sockaddr.
 *
 * Restituisce 0 in caso di successo, -1
 * in caso di errore.
 */
int getSockAddrPort(const struct sockaddr*, uint16_t*);

#endif
