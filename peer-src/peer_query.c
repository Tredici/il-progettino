#include "peer_query.h"
#include <string.h>

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

int buildQuery(struct query* query,
            enum aggregation_type type,
            enum entry_type category,
            const struct tm* begin,
            const struct tm* end)
{
    if (query == NULL || begin == NULL || end == NULL)
        return -1;

    memset(query, 0, sizeof(struct query));
    query->type = type;
    query->category = category;
    query->begin = *begin;
    query->end = *end;

    return 0;
}
