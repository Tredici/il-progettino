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

int set_check(struct set* S, long int key)
{
    if (S == NULL)
        return -1;

    return rb_tree_get(S->T, key, NULL);
}

int set_remove(struct set* S, long int key)
{
    if (S == NULL)
        return -1;

    return rb_tree_remove(S->T, key, NULL);
}
