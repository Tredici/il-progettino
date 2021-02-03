#include "peer_query.h"
#include <string.h>
#include <assert.h>
#include "../time_utils.h"
#include <stddef.h>
#include <errno.h>

struct query
{
    /* tipo della query */
    enum aggregation_type aggregation;
    /* "categoria" degli oggetti
     * da considerare */
    enum entry_type category;
    /* date di inizio e di fine
     * del periodo di interesse */
    struct tm begin, end;
};

/** Nel caso in cui la query chieda
 * un totale l'array contiene un unico
 * elemento, nel caso chieda la
 * variazione contiene tanti elementi
 * quanto è la differenza tra le date
 * agli estremi.
 */
struct answer
{
    struct query query;
    size_t length;
    int* data; /* array in memoria dinamica */
};

int checkQuery(const struct query* Q)
{
    if (Q == NULL)
        return -1;

    if (time_date_cmp(&Q->begin, &Q->end) > 0)
        return -1;

    switch (Q->aggregation)
    {
    case AGGREGATION_SUM:
        break;
    case AGGREGATION_DIFF:
        /* se le date sono uguali non ha senso */
        if (time_date_cmp(&Q->begin, &Q->end) == 0)
            return -1;
        break;
    default:
        return -1;
    }

    switch (Q->category)
    {
    case SWAB:
    case NEW_CASE:
        break;
    default:
        return -1;
    }

    return 0;
}

/** Struttura ausiliaria per calcolare
 * una query del tipo AGGREGATION_SUM
 */
struct calcAns
{
    int* data;
    enum entry_type type;
    int error; /* flag, se non 0 è un disastro */
};

static void calcAns_helper(void* reg, void* base)
{
    struct e_register* R;
    struct calcAns* ref;
    int ans;

    R = (struct e_register*) reg;
    ref = (struct calcAns*)base;
    ans = register_calc_type(R, ref->type);
    if (ans == -1)
        ref->error = -1;
    else
        *(ref->data) = ans;
}

static void calcAns_diff_helper(void* reg, void* base)
{
    struct e_register* R;
    struct calcAns* ref;
    int ans;

    R = (struct e_register*) reg;
    ref = (struct calcAns*)base;
    ans = register_calc_type(R, ref->type);
    if (ans == -1)
        ref->error = -1;
    else
        *(ref->data++) = ans;
}

int calcAnswer(
            struct answer** A,
            const struct query* Q,
            const struct list* l)
{
    struct answer* ans;
    int* data; /* dati */
    struct e_register* R;
    struct tm* date; /* data - giorno */
    struct calcAns calcData;
    size_t i, numRegisters;
    int* tmp;

    if (A == NULL || Q == NULL || l == NULL || checkQuery(Q) != 0)
        return -1;

    /* controlla la lista */
    /* lunghezza totale - compatibile con l'intervallo */
    if (list_size(l) != (ssize_t)time_date_diff(&Q->end, &Q->begin)+1)
        return -1;

    numRegisters = (size_t)list_size(l); /* serve nel caso delle differenze */
    /* la prima data - quella più recente */
    if (list_first(l, (void**)&R) == -1)
        return -1;
    date = register_date(R);
    errno = 0;
    if (date == NULL || time_date_cmp(date, &Q->end) != 0 || errno != 0)
        return -1;
    /* l'ultima data - quella più remota */
    if (list_last(l, (void**)&R) == -1)
        return -1;
    date = register_date(R);
    errno = 0;
    if (date == NULL || time_date_cmp(date, &Q->end) != 0 || errno != 0)
        return -1;

    ans = malloc(sizeof(struct answer));
    if (ans == NULL)
        return -1;

    switch (Q->aggregation)
    {
    case AGGREGATION_SUM:
        data = calloc(1, sizeof(int));
        if (data == NULL)
        {
            free(ans);
            return -1;
        }
        calcData.data = data; /* accumulatore */
        calcData.type = Q->category;
        calcData.error = 0;
        /* calcolo */
        list_accumulate((struct list*)l, &calcAns_helper, (void*)&calcData);
        /* controllo dell'errore */
        if (calcData.error)
        {
            free(data);
            free(ans);
            return -1;
        }
        ans->length = 1;
        ans->data = data;
        break;

    case AGGREGATION_DIFF:
        data = calloc(numRegisters, sizeof(int));
        if (data == NULL)
        {
            free(ans);
            return -1;
        }
        calcData.data = data; /* accumulatore */
        calcData.type = Q->category;
        calcData.error = 0;
        /* calcolo */
        list_accumulate((struct list*)l, &calcAns_diff_helper, (void*)&calcData);
        /* controllo dell'errore */
        if (calcData.error)
        {
            free(data);
            free(ans);
            return -1;
        }
        /* calcolo delle differenze */
        for (i = 1; i != numRegisters; ++i)
            data[i-1] -= data[i];
        /* passaggio dei dati */
        ans->length = --numRegisters;
        ans->data = data;
        break;
    }
    ans->query = *Q;
    *A = ans;

    return 0;
}

void freeAnswer(struct answer* A)
{
    if (A->length != 0)
        free(A->data);
    free(A);
}

int initNsQuery(struct ns_query* nsQuery, const struct query* Q)
{
    if (nsQuery == NULL || Q == NULL)
        return -1;

    if (time_init_ns_tm(&nsQuery->begin, &Q->begin) == -1
        || time_init_ns_tm(&nsQuery->end, &Q->end) == -1)
        return -1;

    nsQuery->type = htons(Q->aggregation);
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

    Q->aggregation = ntohs(nsQuery->type);
    Q->category = ntohs(nsQuery->category);

    return 0;
}

int initNsAnswer(
            struct ns_answer** answer,
            size_t* answerLen,
            const struct answer* A)
{
    struct ns_answer* tmp;
    size_t tmpLen;
    size_t i;

    assert(sizeof(struct ns_answer) == offsetof(struct ns_answer, data));

    if (answer == NULL || answerLen == NULL || A == NULL || (!A->length ^ !A->data))
        return -1;

    /* assume che la risposta sia integra */
    tmpLen = sizeof(struct ns_answer) + sizeof(uint32_t)*A->length;
    tmp = malloc(tmpLen);
    memset(tmp, 0, tmpLen);

    /* rimepie la risposta */
    if (initNsQuery(&tmp->query, &A->query) == -1)
    {
        free(tmp);
        return -1;
    }
    tmp->lenght = htonl((uint32_t)A->length);
    for (i = 0; i != A->length; ++i)
        tmp->data[i] = htonl((uint32_t)A->data[i]);

    *answer = tmp;
    *answerLen = tmpLen;

    return 0;
}

int readNsAnswer(struct answer** Q,
            const struct ns_answer* nsQ,
            size_t nsLen)
{
    struct answer* ans;
    size_t i, len;
    int* data;

    if (Q == NULL || nsQ == NULL)
        return -1;

    if (nsLen != sizeof(struct ns_answer) + ntohl(nsQ->lenght)*sizeof(uint32_t))
        return -1;

    ans = malloc(sizeof(struct answer));
    if (ans == NULL)
        return -1;

    memset(ans, 0, sizeof(struct answer));

    if (readNsQuery(&ans->query, &nsQ->query) == -1)
    {
        free(ans);
        return -1;
    }
    len = (size_t)ntohl(nsQ->lenght);
    ans->length = len;

    data = calloc(len, sizeof(int));
    if (data == NULL)
    {
        free(ans);
        return -1;
    }
    for (i = 0; i != len; ++i)
        data[i] = ntohl(nsQ->data[i]);

    ans->data = data;
    *Q = ans;

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
    query->aggregation = type;
    query->category = category;
    query->begin = *begin;
    query->end = *end;

    return 0;
}

int readQuery(const struct query* query,
            enum aggregation_type* type,
            enum entry_type* category,
            struct tm* begin,
            struct tm* end)
{
    if (checkQuery(query) != 0)
        return -1;

    if (type != NULL)
        *type = query->aggregation;
    if (category != NULL)
        *category = query->category;
    if (begin != NULL)
        *begin = query->begin;
    if (end != NULL)
        *end = query->end;

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
    hash.aggr_type = query->aggregation;
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
