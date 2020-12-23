/**
 * Ricalca la mia idea di deque
 * Non so se qualcuno altro al mondo la condivida
 */

#ifndef DEQUE
#define DEQUE

/* deque */
struct deque;
/* iteratore della deque */
struct deque_i;

/**
 * Come sempre, se NULL alloca anche la 
 * memoria necessaria, altrimenti 
 * semplicemente inizializza il riferimento
 * passato
 */
struct deque* deque_init(struct deque*);

/** Per ottenere e settare la funzione di cleanup
 * da invocare al momento della "distruzione" di
 * una deue
 */
void (*deque_get_cleanup_f(struct deque*))(void*);
void (*deque_set_cleanup_f(struct deque*, void(*)(void*)))(void*);

#endif
