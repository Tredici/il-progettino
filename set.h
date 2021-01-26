/** Ispirata (quello che mi ricordo di) std::set
 * questa struttura dati permette di gestire
 * facilmente alcune operazioni basilari su
 * insiemi di numeri interi
 */


#ifndef SET
#define SET

#include <stdlib.h>

struct set;

struct set* set_init(struct set*);
struct set* set_clear(struct set*);
/* da chiamare SEMPRE al termine dell'utilizzo */
void set_destroy(struct set*);


/* inserimento test e rimozione */
int set_add(struct set*, long int);
int set_has(struct set*, long int);
int set_remove(struct set*, long int);

/* size */
ssize_t set_size(struct set*);

/* 1 se vuoto -1 errore 0 altrimenti */
int set_empty(struct set*);

/* minimo massimo */
int set_min(struct set*, long int*);
int set_max(struct set*, long int*);

/* precedente successore */
int set_prev(struct set*, long int, long int*);
int set_next(struct set*, long int, long int*);

/* foreach */
int set_foreach(struct set*, void(*)(long int));

/* accumulate */
int set_accumulate(struct set*, void(*)(long int, void*), void*);

/* crea una copia del set */
struct set* set_clone(const struct set*);

#endif
