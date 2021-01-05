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
        ans = malloc(sizeof(struct set));
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

int set_has(struct set* S, long int key)
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

    if (S == NULL)
        return -1;

    return rb_tree_accumulate(S->T, &help_foreach, (void*)&f);
}
