#include "rb_tree.h"
#include <stdlib.h>
#include <string.h>

struct set
{
    struct rb_tree* T;
};


struct set* set_init(struct set* S)
{
    struct set* ans;

    if (S == NULL)
    {
        ans = (struct set*)malloc(sizeof(struct set));
    }
    else
    {
        ans = S;
    }

    ans->T = rb_tree_init(NULL);
    if (ans->T == NULL)
    {
        if (S == NULL)
            free(ans);

        return NULL;
    }

    return ans;
}

struct set* set_clear(struct set* S)
{
    if (S == NULL)
        return NULL;

    if (rb_tree_clear(S->T) == NULL)
        return NULL;

    return S;
}

void set_destroy(struct set* S)
{
    if (S == NULL)
        return;

    rb_tree_destroy(S->T);
    S->T = NULL;
}

int set_add(struct set* S, long int key)
{
    if (S == NULL)
        return -1;

    return rb_tree_set(S->T, key, NULL);
}

int set_has(const struct set* S, long int key)
{
    if (S == NULL)
        return -1;

    switch (rb_tree_get(S->T, key, NULL))
    {
    case 0:
        return 1;

    default:
        return 0;
    }
}

int set_remove(struct set* S, long int key)
{
    if (S == NULL)
        return -1;

    return rb_tree_remove(S->T, key, NULL);
}

ssize_t set_size(struct set* S)
{
    if (S == NULL)
        return -1;

    return rb_tree_size(S->T);
}

int set_empty(struct set* S)
{
    if (S == NULL)
        return -1;

    switch (rb_tree_size(S->T))
    {
    case 0:
        return 1;
    case -1:
        return -1;
    default:
        return 0;
    }
}

int set_min(struct set* S, long int* val)
{
    if (S == NULL || val == NULL)
        return -1;

    return rb_tree_min(S->T, val, NULL);
}

int set_max(struct set* S, long int* val)
{
    if (S == NULL || val == NULL)
        return -1;

    return rb_tree_max(S->T, val, NULL);
}

int set_prev(struct set* S, long int base, long int* val)
{
    if (S == NULL)
        return -1;

    return rb_tree_prev(S->T, base, val, NULL);
}

int set_next(struct set* S, long int base, long int* val)
{
    if (S == NULL)
        return -1;

    return rb_tree_next(S->T, base, val, NULL);
}

static void help_foreach(long int key, void* value, void* base)
{
    void(*f)(long int);

    /* per sopprimere il warning "unused parameter" */
    (void)value;

    /* questa è arte e l'arte non si spiega */
    /* Deriva dal fatto che il cast da un puntatore
     * a funzione a (void*) e viceversa è, per
     * lo standard del C, undefined behaviour */
    f = *(void(**)(long int))base;
    f(key);
}

int set_foreach(struct set* S, void(*fun)(long int))
{
    void(*f)(long int);

    if (S == NULL || fun == NULL)
        return -1;

    f = fun;

    return rb_tree_accumulate(S->T, &help_foreach, (void*)&f);
}

/** Struttura dati ausiliaria per permettere
 * di semplificare l'implementazione
 * di set_accumulate.
 */
struct setAccumulateData
{
    void(*f)(long int, void*);
    void* base;
};

static void help_accumulate(long int key, void* value, void* base)
{
    struct setAccumulateData* D;

    (void)value;
    D = (struct setAccumulateData*)base;
    D->f(key, D->base);
}

int set_accumulate(const struct set* S, void(*f)(long int, void*), void* base)
{
    struct setAccumulateData data;

    if (S == NULL || f == NULL)
        return -1;

    data.f = f;
    data.base = base;

    return rb_tree_accumulate(S->T, &help_accumulate, (void*)&data);
}

static void* NULL_fun(void* x)
{
    (void)x;
    return NULL;
}

struct set* set_clone(const struct set* S)
{
    struct set* ans;

    if (S == NULL)
        return NULL;

    ans = (struct set*)malloc(sizeof(struct set));
    if (ans == NULL)
        return NULL;

    memset(ans, 0, sizeof(struct set));
    ans->T = rb_tree_clone(S->T, NULL_fun);
    if (ans->T == NULL)
    {
        free(ans);
        return NULL;
    }

    return ans;
}

static void help_merge(long int key, void* base)
{
    struct set* S;

    S = (struct set*)base;
    set_add(S, key);
}

int set_merge(struct set* S1, const struct set* S2)
{
    if (S1 == NULL || S2 == NULL)
        return -1;

    return set_accumulate(S2, &help_merge, (void*)S1);
}

struct set* set_union(const struct set* S1, const struct set* S2)
{
    struct set* ans;

    if (S1 == NULL || S2 == NULL)
        return NULL;

    ans = set_clone(S1);
    if (ans == NULL)
        return NULL;

    if (set_merge(ans, S2) == -1)
    {
        set_destroy(ans);
        return NULL;
    }

    return ans;
}

static void help_purge(long int key, void* base)
{
    struct set* S;

    S = (struct set*)base;
    set_remove(S, key);
}

int set_purge(struct set* S1, const struct set* S2)
{
    if (S1 == NULL || S2 == NULL)
        return -1;

    return set_accumulate(S2, &help_purge, (void*)S1);
}

struct set* set_diff(const struct set* S1, const struct set* S2)
{
    struct set* ans;

    if (S1 == NULL || S2 == NULL)
        return NULL;

    ans = set_clone(S1);
    if (ans == NULL)
        return NULL;

    if (set_purge(ans, S2) != 0)
    {
        set_destroy(ans);
        return NULL;
    }

    return ans;
}
