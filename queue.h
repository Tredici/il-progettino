/**
 * "Classe" queue per lo scambio di dati tra più thread
 * Supporta accesso concorrente in lettura e scrittura.
 */


#ifndef QUEUE
#define QUEUE

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct queue;

enum q_flag {
    Q_CONCURRENT = 0,   /*  */
    Q_SYNC,
    Q_DEFAULT = Q_CONCURRENT
};

/** Inizializza una queue per l'utilizzo, allocandola in memoria dinamica se necessario
 * 
 */
struct queue* queue_init(struct queue*, enum q_flag);
/**
 * 
 */
struct queue* queue_clear(struct queue*);
void queue_destroy(struct queue*);

size_t queue_get_size(const struct queue*);
size_t queue_empty(const struct queue*);

void (*queue_set_cleanup_f(struct queue*, void(*)(void*)))(void*);
void (*queue_get_cleanup_f(struct queue*))(void*);

/** Inserisce un nuovo elemento nella queue
 * 
 */
int queue_push(struct queue*, void*);

/** Estrae un elemento dalla coda
 * A seconda del valore del flag potrebbe bloccarsi o meno
 */
int queue_pop(struct queue*, void**, int);


#endif
