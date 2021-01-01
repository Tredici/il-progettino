/**
 * Implementazione delle funzioni 
 */
#include "list.h"
#include <string.h>

typedef struct elem
{
    struct elem* next;
    void* val;
} elem;

/** Costruisce e inizializza un elemento
 * con il valore fornito
 */
elem* elem_init(void* val)
{
    elem* ans;

    ans = malloc(sizeof(elem));
    if (ans == NULL)
        return NULL;

    memset(ans, 0, sizeof(elem));
    ans->val = val;

    return ans;
}

struct list
{
    void(*cleanup_f)(void*);
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

void (*list_set_cleanup(struct list* l, void(*f)(void*)))(void*)
{
    void(*ans)(void*);
    ans = l->cleanup_f;
    l->cleanup_f = f;
    return ans;
}

ssize_t list_size(const struct list* l) {
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

void list_eliminate(struct list* l, int (*fun)(void*))
{
    void* val;
    elem* ptr;  /* per iterare */
    elem* curr; /* per eliminare */
    elem* prev; /* per mantenere il collegamento */

    /* controllo validità parametri */
    if (l == NULL || fun == NULL)
        return;

    prev = NULL; /* all'inizio non ci sono precedenti */
    ptr = l->first;
    while (ptr != NULL)
    {
        /* ora si lavora su questo */
        curr = ptr;
        /* e si prende di mira il prossimo */
        ptr = curr->next;

        if (0 != fun(curr->val))
        {
            /* mantiene la catena */
            if (prev == NULL)
            {
                /* nuovo primo */
                l->first = ptr;
            }
            else
            {
                /* elemento nel mezzo */
                prev->next = ptr;
            }

            /* libera l'elemento */
            free(curr);
            /* un elemento in meno */
            l->len--;
            /* il precednte non cambia */
            continue;
        }

        /* solo se non si elimina nulla */
        prev = curr;
    }
}

void list_accumulate(struct list* l, void (*fun)(void*, void*), void* base)
{
    elem* ptr;

    if (l == NULL)
        return;

    for (ptr = l->first; ptr != NULL; ptr = ptr->next)
        fun(ptr->val, base);
}

int list_prepend(struct list* l, void* val)
{
    elem* new_e;

    if (l == NULL)
        return -1;

    new_e = elem_init(val);
    if (new_e == NULL)
        return -1;

    new_e->next = l->first;
    l->first = new_e;
    l->len++;

    return 0;
}
