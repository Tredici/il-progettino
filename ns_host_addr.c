#include "ns_host_addr.h"


static uint8_t ipVersion(sa_family_t fam)
{
    switch (fam)
    {
    case AF_INET:
        return (uint8_t)4;

    case AF_INET6:
        return (uint8_t)6;

    default:
        /* 0 in caso di errore - scelta insolita ma necessaria */
        return 0;
    }
}
static sa_family_t ipFamily(uint8_t ver)
{
    switch (ver)
    {
    case 4:
        return AF_INET;
    case 6:
        return AF_INET6;

    default:
        /* idealmente mai raggiunto */
        return -1;
    }
}
