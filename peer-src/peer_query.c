#include "peer_query.h"

struct query
{
    /* tipo della query */
    enum aggregation_type type;
    /* "categoria" degli oggetti
     * da considerare */
    enum entry_type category;
    /* date di inizio e di fine
     * del periodo di interesse */
    struct tm begin, end;
};
