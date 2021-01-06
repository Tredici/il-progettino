#include "queue.h"
#include <errno.h>

typedef struct elem
{
    struct elem* next;
    void* val;
} elem;

/** Inizializza un elemento e lo ritorna
 */
static elem* elem_init(void* val)
{
    elem* ans;
    
    ans = malloc(sizeof(elem));
    if (ans == NULL)
        return NULL;
    
    memset(ans, 0, sizeof(elem));
    ans->val = val;

    return ans;
}

struct queue
{
    void(*cleanup_f)(void*);
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
    elem* first;
    elem* last;
    size_t len; /* numero di elementi presenti */
    /* contatori per le chiamate a pop e push */
    size_t npop, npush;
};

struct queue* queue_init(struct queue* q, enum q_flag flag) {
    struct queue* ans;
    int err;

    if (q != NULL)
    {
        ans = q;
    }
    else
    {
        ans = malloc(sizeof(struct queue));
        if (ans == NULL)
            return NULL;
    }
    memset(ans, 0, sizeof(struct queue));   /* Implicita assunzione che NULL sia 0 */

    switch (flag)
    {
    case Q_CONCURRENT:
        /* deve abilitare la struttura all'accesso concorrente */
        ans->mutex = malloc(sizeof(pthread_mutex_t));
        if (ans->mutex == NULL)
        {
            if (q == NULL)
                free(ans);
            return NULL;
        }
        err = pthread_mutex_init(ans->mutex, NULL);
        if (err != 0)
        {
            free(ans->mutex);
            if (q == NULL)
                free(ans);

            return NULL;
        }

        ans->cond = malloc(sizeof(pthread_cond_t));
        if (ans->cond == NULL)
        {
            pthread_mutex_destroy(ans->mutex);
            free(ans->mutex);
            if (q == NULL)
                free(ans);

            return NULL;
        }

        err = pthread_cond_init(ans->cond, NULL);
        if(err)
        {
            free(ans->cond);
            pthread_mutex_destroy(ans->mutex);
            free(ans->mutex);
            if (q == NULL)
                free(ans);
        }

        break;

    case Q_SYNC:
        /* accesso sequenziale, nessuna necessità di mutex&co */
        /* non serve far nulla */
        break;
    
    default:
        /* parametri non validi */
        if (q == NULL)
            free(ans);  /* Rilascia l'eventuale mmeoria allocata*/

        return NULL;
    }

    return ans;
}

inline static void destroy_list(elem* e, void(*cleanup_f)(void*))
{
    elem* next;

    while (e != NULL)
    {
        next = e->next;
        if (cleanup_f != NULL)
            cleanup_f(e->val);
        free(e);
        e = next;
    }
}

struct queue* queue_clear(struct queue* q)
{
    /* thread-safe */
    if (q->mutex != NULL)
    {
        pthread_mutex_lock(q->mutex);
    }

    destroy_list(q->first, q->cleanup_f);
    q->first = NULL;
    q->last = NULL;
    q->len = 0;
    q->npop = 0;
    q->npush = 0;

    if (q->mutex != NULL)
    {
        pthread_mutex_unlock(q->mutex);
    }

    return q;
}

void queue_destroy(struct queue* q)
{
    /* Non thread-safe! */
    destroy_list(q->first, q->cleanup_f);
    q->first = NULL;
    q->last = NULL;
    q->len = 0;
    q->npop = 0;
    q->npush = 0;

    if (q->mutex != NULL)
    {
        pthread_cond_destroy(q->cond);
        free(q->cond);
        pthread_mutex_destroy(q->mutex);
        free(q->mutex);
    }

    free(q);
}

size_t queue_get_size(const struct queue* q)
{
    size_t ans = 0;

    if (q == NULL)
    {
        return 0;
    }
    
    if (q->mutex)
    {
        pthread_mutex_lock(q->mutex);
    }

    ans = q->len;

    if (q->mutex)
    {
        pthread_mutex_unlock(q->mutex);
    }

    return ans;
}

size_t queue_empty(const struct queue* q)
{
    return queue_get_size(q) == 0;
}

void (*queue_set_cleanup_f(struct queue* q, void(*cleanup_f)(void*)))(void*)
{
    void(*ans)(void*);

    if (q == NULL)
    {
        return NULL;
    }

    ans = q->cleanup_f;
    q->cleanup_f = cleanup_f;

    return ans;
}

void (*queue_get_cleanup_f(struct queue* q))(void*)
{
    if (q == NULL)
    {
        return NULL;
    }
    return q->cleanup_f;
}

/**
 * Ritorna un valore negativo in caso di errore,
 * altrimeni il numero dell'inserimento effettuato.
 */
int queue_push(struct queue* q, void* data)
{
    elem* new_elem;
    int ans;

    if (q == NULL) 
        return -1;
    
    new_elem = elem_init(data);
    if (new_elem == NULL)
    {
        return -1;
    }

    if (q->mutex != NULL)
    {
        pthread_mutex_lock(q->mutex);   
    }

    if (q->last == NULL)
    {
        q->first = q->last = new_elem;
        pthread_cond_broadcast(q->cond);
    }
    else
    {
        q->last = q->last->next = new_elem;
    }

    q->len++;
    ans = q->npush++;

    if (q->mutex != NULL)
    {
        pthread_mutex_unlock(q->mutex);   
    }

    return ans;
}


/**
 * Ritorna un valore negativo in caso di errore,
 * altrimeni il numero dell'estrazione effettuata.
 */
int queue_pop(struct queue* q, void** data, int flag)
{
    elem* old_elem;
    int err;
    int ans;

    if (q == NULL)
    {
        return -1;
    }

    if (q->mutex != NULL)
    {
        pthread_mutex_lock(q->mutex);

        /* se lavora in maniera NONBLOCKING */
        if (flag != 0 && q->len == 0)
        {
            errno = ENODATA;
            pthread_mutex_unlock(q->mutex);
            return -1;
        }

        while (q->len == 0)
        {
            /* si blocca sulla variabile di condizione */
            pthread_cond_wait(q->cond, q->mutex);
        }
    }
    else if (q->len == 0)
    {
        /* Non ci sono dati e la concorrenza non è supportata */
        return -1;
    }

    q->len--;

    old_elem = q->first;
    if (q->len == 0)
    {
        q->first = q->last = NULL;
    }
    else
    {
        q->first = old_elem->next;
    }

    old_elem->next = NULL;
    *data = old_elem->val;
    ans = q->npop++;

    free(old_elem);

    if (q->mutex != NULL)
    {
        pthread_mutex_unlock(q->mutex);
    }

    return ans;
}
