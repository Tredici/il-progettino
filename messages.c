#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>

/** Contiene i primi
 * due cambi di un messaggio
 * ben strutturato
 */
struct messages_head
{
    uint16_t sentinel;
    uint16_t type;
};


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

