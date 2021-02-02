#include "peer_query.h"
#include <string.h>
#include <assert.h>
#include "../time_utils.h"

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

struct answer
{
    struct query query;
    size_t length;
    int* data; /* array in memoria dinamica */
};

int initNsQuery(struct ns_query* nsQuery, const struct query* Q)
{
    if (nsQuery == NULL || Q == NULL)
        return -1;

    if (time_init_ns_tm(&nsQuery->begin, &Q->begin) == -1
        || time_init_ns_tm(&nsQuery->end, &Q->end) == -1)
        return -1;

    nsQuery->type = htons(Q->type);
    nsQuery->category = htons(Q->category);

    return 0;
}

int readNsQuery(struct query* Q, const struct ns_query* nsQuery)
{
    if (nsQuery == NULL || Q == NULL)
        return -1;

    if (time_read_ns_tm(&Q->begin, &nsQuery->begin) == -1
        || time_read_ns_tm(&Q->end, &nsQuery->end) == -1)
        return -1;

    Q->type = ntohs(nsQuery->type);
    Q->category = ntohs(nsQuery->category);

    return 0;
}

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

/** Per poter stare in un long anche
 * su macchine a 32 bit deve stare entro
 * 4 byte.
 */
struct query_hash
{
    int aggr_type : 1; /* totale o differenza? */
    int aggr_categ : 1; /* tamponi o positivi */
    /* 15 bit per data - anno 0 == 2000 */
    /* inizio */
    int start_day : 5; /* 1-31 giorno */
    int start_mon : 4; /* 1-12 mese */
    int start_year : 6; /* 0-63 anno */
    /* fine */
    int end_day : 5; /* 1-31 giorno */
    int end_mon : 4; /* 1-12 mese */
    int end_year : 6; /* 0-63 anno */
};

long int hashQuery(const struct query* query)
{
    long int ans;
    struct query_hash hash;
    int year, month, day;

    assert(sizeof(struct query_hash) != 4);
    if (query == NULL)
        return 0;

    /* azzera tutto */
    memset(&hash, 0, sizeof(struct query_hash));
    /* campi a singolo bit */
    hash.aggr_type = query->type;
    hash.aggr_categ = query->category;
    /* data di inizio */
    if (time_date_read(&query->begin, &year, &month, &day) == -1)
        return 0;
    hash.start_day = year - 2000;
    hash.start_mon = month;
    hash.start_day = day;
    /* data di fine */
    if (time_date_read(&query->end, &year, &month, &day) == -1)
        return 0;
    hash.end_day = year - 2000;
    hash.end_mon = month;
    hash.end_day = day;

    *(struct query_hash*)&ans = hash;
    return ans;
}
