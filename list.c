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
    if (l == NULL)
    {
        ans = malloc(sizeof(struct list));
        if (ans == NULL)
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

struct list* list_clear(struct list* l)
{
    elem* ptr, *next;

    if (l->cleanup_f != NULL)
        list_foreach(l, l->cleanup_f);

    for (ptr = l->first; ptr; ptr = next) {
        next = ptr->next;
        free(ptr);
    }

    l->first = NULL;
    l->len = 0;

    return l;
}

void list_destroy(struct list* l) {
    list_clear(l);
    free(l);
}

void (*list_get_cleanup(struct list* l))(void*)
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

size_t list_size(const struct list* l) {
    return l != NULL ? l->len : -1;
}

void list_foreach(struct list* l, void (*fun)(void*))
{
    elem* p;

    for (p = l->first; p; p = p->next)
    {
        fun(p->val);
    }
}

void* list_get_item(struct list* l, size_t index) {
    elem* ptr;
    size_t i;
    
    if (index >= l->len)
    {
        errno = EINVAL;
        return NULL;
    }

    for (i = 0, ptr = l->first; i != index && ptr; ++i)
    {

    }

    return ptr->val;
}