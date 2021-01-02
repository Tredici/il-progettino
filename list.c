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
    return l != NULL ? (ssize_t)l->len : -1;
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
        ;

    return ptr->val;
}

int list_eliminate(struct list* l, int (*fun)(void*))
{
    elem* ptr;  /* per iterare */
    elem* curr; /* per eliminare */
    elem* prev; /* per mantenere il collegamento */
    int ans;

    ans = 0;

    /* controllo validità parametri */
    if (l == NULL || fun == NULL)
        return -1;

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
            ++ans;
            /* il precedente non cambia */
            continue;
        }

        /* solo se non si elimina nulla */
        prev = curr;
    }

    return ans;
}

void list_accumulate(struct list* l, void (*fun)(void*, void*), void* base)
{
    elem* ptr;

    if (l == NULL || fun == NULL)
        return;

    for (ptr = l->first; ptr != NULL; ptr = ptr->next)
        fun(ptr->val, base);
}

struct list* list_map(const struct list* l,
                        void* (*map)(void*),
                        void (*clean_f)(void*))
{
    struct list* ans;
    elem* current; /* punta all'emento di l */
    /* punta all'ultimo elemento creato */
    elem* base;
    /* punta di volta in volta al nuovo elemento
     * che si sta creando */
    elem* new_e;

    /* argomenti validi? */
    if (l == NULL || map == NULL)
        return NULL;

    /* per ragioni di efficienza - farlo in O(n) */
    ans = list_init(NULL);
    if (ans == NULL)
        return NULL;

    list_set_cleanup(ans, clean_f);

    base = NULL;
    new_e = NULL;
    current = NULL;

    /* lista non vuota */
    if (l->first != NULL)
    {
        current = l->first;
        /* crea il primo elemento e lo aggiunge */
        base = elem_init(map(current->val));
        if (base == NULL)
        {
            list_destroy(ans);
            return NULL;
        }

        ans->first = base;
        ans->len++;

        /* ciclo per aggiungere gli elementi successivi */
        for (current = current->next; current != NULL; current = current->next)
        {
            /* prova a generare il nuovo elemento */
            new_e = elem_init(map(current->val));
            /* testa che sia andato tutto bene */
            if (new_e == NULL)
            {
                list_destroy(ans);
                return NULL;
            }

            base->next = new_e; /* aggiunta di un elmento */
            ans->len++; /* lunghezza aumentata */
            base = new_e; /* nuova base */
        }
    }

    return ans;
}

struct list* list_reduce(const struct list* l,
                        void* (*fun)(void*,void*),
                        void (*clean_f)(void*))
{
    struct list* ans;
    elem* e1,* e2;
    /* per gestire la nuova lista */
    elem* base, * new_e;

    /* controllo parametri */
    if (l == NULL || fun == NULL)
        return NULL;

    ans = list_init(NULL);
    if (ans == NULL)
        return NULL;

    list_set_cleanup(ans, clean_f);

    /* se la lista ha almeno un elemento */
    if (l->len > 1)
    {
        e1 = l->first;
        e2 = e1->next;

        new_e = elem_init(fun(e1->val, e2->val));
        if (new_e == NULL)
        {
            list_destroy(ans);
            return NULL;
        }
        ans->first = new_e;
        ans->len++;

        base = new_e;
        for (e1 = e2, e2 = e2->next; e2 != NULL; e1 = e2, e2 = e2->next)
        {
            new_e = elem_init(fun(e1->val, e2->val));
            if (new_e == NULL)
            {
                list_destroy(ans);
                return NULL;
            }
            base->next = new_e;
            ans->len++;
            base = new_e;
        }
    }

    return ans;
}

static void* identity(void* p)
{
    return p;
}

struct list* list_copy(const struct list* l, void* (*fun)(void*))
{
    return list_map(l, fun == NULL ? identity : fun, list_get_cleanup(l));
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

int list_append(struct list* l, void* val)
{
    elem* new_e;
    elem* ptr;

    if (l == NULL)
        return -1;

    new_e = elem_init(val);
    if (new_e == NULL)
        return -1;

    if (l->first == NULL)
    {
        /* lista inizialmente vuota */
        l->first = new_e;
    }
    else
    {
        /* scorre la lista fino alla fine */
        for (ptr = l->first; ptr->next != NULL; ptr = ptr->next)
            ;

        ptr->next = new_e;
    }
    l->len++;

    return 0;
}

#ifdef _LIST_DEBUG
#include <stdio.h>
static void print_elem(void* val)
{
    printf("[%ld] -> ", (long)val);
}
/* Stampa il contenuto di una lista considerando
 * i puntatori come dei long
 */
void list_debug(struct list* l)
{
    if (l == NULL)
    {
        printf(" *** list_debug(NULL) *** \n");
        return;
    }
    printf("{ ");
    list_foreach(l, print_elem);
    printf("NIL }\n");
}
#endif
