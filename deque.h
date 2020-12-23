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


#endif
