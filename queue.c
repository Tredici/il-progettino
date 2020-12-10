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
            free(ans->mutex);
            if (q == NULL)
                free(ans);

            return NULL;
        }

        err = pthread_cond_init(ans->cond, NULL);
        if(err)
        {
            free(ans->cond);
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
    if (q == NULL) 
        return -1;
    
    if (q->mutex != NULL)
    {
        
    }

    return 0;
}


