#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

/** ATTENZIONE:
 * dato che in questo file andrò
 * a generare il contenuto dei
 * pacchetti da inviare via rete
 * utilizzerò più volte l'estensione
 * di GCC __attribute__ ((packed))
 * per essere sicuro che la struttura
 * delle strutture non sia alterata
 * attraverso la rete.
 */

/** Contiene i primi
 * due cambi di un messaggio
 * ben strutturato
 */
struct messages_head
{
    uint16_t sentinel;
    uint16_t type;
} __attribute__ ((packed));


int recognise_messages_type(void* msg)
{
    struct messages_head* p;

    if (msg == NULL)
        return -1;

    p = (struct messages_head*)msg;
    if (p->sentinel != 0)
        return MESSAGES_MSG_UNKWN;

    return (int)ntohs(p->type);
}

