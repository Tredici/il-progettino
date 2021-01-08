#include "messages.h"
#include <arpa/inet.h>
#include <stdlib.h>

int recognise_messages_type(void* msg)
{
    uint16_t* p;

    if (msg == NULL)
        return -1;

    p = (uint16_t*)msg;
    if (p[0] != 0)
        return MESSAGES_MSG_UNKWN;

    return (int)ntohs(p[0]);
}

