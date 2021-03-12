#include "deque.h"
#include <string.h>

struct delem
{
    struct delem* prev;
    struct delem* next;
    void* val;
};

typedef struct delem delem;

struct deque
{
    void(*cleanup_f)(void*);
    delem* first;
    delem* last;
    size_t len;
};

struct deque* deque_init(struct deque* d)
{
    struct deque* ans;

    if (d == NULL)
    {
        ans = (struct deque*)malloc(sizeof(struct deque));
        if (ans == NULL)
            return NULL;
    }
    else
    {
        ans = d;
    }

    memset(ans, 0, sizeof(struct deque));

    return ans;
}

void (*deque_get_cleanup_f(struct deque* d))(void*)
{
    if (d == NULL)
    {
        return NULL;
    }
    return d->cleanup_f; 
}

void (*deque_set_cleanup_f(struct deque* d, void(*cleanup_f)(void*)))(void*)
{
    void(*ans)(void*);

    if (d == NULL)
    {
        return NULL;
    }
    ans = d->cleanup_f;
    d->cleanup_f = cleanup_f;
    return ans; 
}

struct deque* deque_clear(struct deque* d)
{
    delem* iter, * next;

    if (d == NULL)
    {
        return NULL;
    }

    for (iter = next = d->first; next != NULL; iter = next)
    {
        if (d->cleanup_f != NULL)
            d->cleanup_f(iter->val);

        next = iter->next;
        free(iter);
    }

    d->first = NULL;
    d->last = NULL;
    d->len = 0;

    return d;
}

void deque_destroy(struct deque* d)
{
    if (deque_clear(d) != NULL)
        free(d);
}

ssize_t deque_size(const struct deque* d)
{
    if (d == NULL)
        return -1;

    return d->len;
}

/**
 * Funzione ausiliaria per inizializzare
 * elementi
 */
static delem* delem_init(void* val)
{
    delem* ans;

    ans = (delem*)malloc(sizeof(delem));
    if (ans == NULL)
        return NULL;

    memset(ans, 0, sizeof(delem));
    return ans;
}

/**
 * Aggiunge il primo elemento a una deque
 */
static int deque_start(struct deque* d, delem* e)
{
    if (d == NULL || d->len != 0)
        return -1;

    if (e->next != NULL || e->prev != NULL)
        return -1;

    d->first = e;
    d->last = e;
    d->len = 1;

    return 0;
}


int deque_append(struct deque* d, void* val)
{
    delem* e;

    if (d == NULL)
        return -1;

    e = delem_init(val);
    if (e == NULL)
        return -1;

    if (d->len == 0)
    {
        if(deque_start(d, e) != 0)
            return -1;
    }
    else
    {
        /* inserisce dalla parte di last */
        e->prev = d->last;
        d->last->next = e;
        d->last = e;
    }
    d->len++;

    return 0;
}

