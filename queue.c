#include "queue.h"

typedef struct elem
{
    elem* next;
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

/** Elimina e svolge il cleanup richiesto su una lista di elmenti
 */
static void elem_destroy_list(elem* e, void(*f)(void*));

struct queue
{
    void(*cleanup_f)(void*);
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
    elem* first;
    elem* last;
    size_t len; /*non necessario, per ora non usato*/
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


int queue_push(struct queue* q, void* data)
{
    elem* new_elem;

    if (q == NULL) 
        return -1;
    
    new_elem = elem_init(data);
    if (new_elem == NULL)
    {
        return NULL;
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

    if (q->mutex != NULL)
    {
        pthread_mutex_unlock(q->mutex);   
    }

    return 0;
}


int queue_pop(struct queue* q, void** data, int flag)
{
    void* ans;
    elem* old_elem;
    int err;

    if (q == NULL)
    {
        return -1;
    }

    if (q->mutex != NULL)
    {
        pthread_mutex_lock(q->mutex);

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
    ans = old_elem->val;
    *data = ans;

    free(old_elem);

    if (q->mutex != NULL)
    {
        pthread_mutex_unlock(q->mutex);
    }

    return 0;
}
