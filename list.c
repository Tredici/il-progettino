/**
 * Implementazione delle funzioni 
 */
#include "list.h"

typedef struct elem
{
    elem* next;
    void* val;
} elem;


struct list
{
    void(*cleanup_f)(void);
    elem* first;
    size_t len;
};

struct list* list_init(struct list* l)
{
    struct list* ans;
    if(l == NULL)
    {
        ans = malloc(sizeof(struct list));
        if(ans == NULL)
        {
            return NULL;
        }
    }
    else 
    {
        ans = l;
    }
    ans->first = NULL;
    ans->len = 0;
    ans->cleanup_f = NULL;

    return ans;
}

void (*list_get_cleanup(const struct list* l))(void*)
{
    return l->cleanup_f;
}

void (*list_set_cleanup(struct list* l, void(*f)(void)))(void*)
{
    void(*ans)(void);
    ans = l->cleanup_f;
    l->cleanup_f = f;
    return ans;
}

int list_size(const struct list* l) {
    return l != NULL ? l->len : -1;
}
