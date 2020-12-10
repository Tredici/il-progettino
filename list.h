/*
 * Poter gestire delle liste è molto utile.
 * A tale fine creo l'equivalente di una
 * classe list<T>, sebbene sia molto minimale.
 */

#ifndef LIST
#define LIST

#include <stdlib.h> /* Definisce NULL */
#include <errno.h>

struct list;

/**Equivalente a un costruttore.
 * @params:
 *  +puntatore alla lista da inizializzare, NULL
 * @return:
 *  +puntatore alla lista inizializzata
 * 
 * Inizializza una lista:
 *  -se l'argomento è NULL alloca internamente un oggetto list e lo restituisce
 *  -se l'argomento è valido inizializza correttamente la struttura
 */
struct list* list_init(struct list*);

/**(Quasi) equivalente a un distruttore
 * elimina tutti gli elementi della lista invocando sul contenuto l'apposita funzione di cleanup, se specificata. 
 * Riporta la lista allo stato iniziale
 */ 
struct list* list_clear(struct list*);

/** Come sopra ma in più libera la memoria 
 * 
 */
void list_destroy(struct list*);

/** Fornisce il numero di elementi nella lista
 * 
 */
size_t list_size(const struct list*);

/** Per specificare l'eventuale funzione di cleanup
 * 
 * Rermette di impostare una funzione di cleanup da invocare su ogni elemento della lista in caso di distruzione (clear) della stessa. Restituisce il precedente handler. 
 * 
 */
void (*list_set_cleanup(struct list*, void(*)(void)))(void*);

/**  Restituisce la funzione di cleanup corrente
 * 
 */
void (*list_get_cleanup(struct list*))(void*);


/** Accede all'elemento i esimo della lista e restituisce il puntatore a esso
 * 
 */
void* list_get_item(struct list*, size_t);

/** Imposta un nuovo valore per l'elemento n-esimo della lista e ritorna il valore precedente 
 * 
 */ 
void* list_set_item(struct list*, size_t, void*);

/** Itera lungo la lista ed applica ad ogni elemento una data funzione.
 * 
 */
void list_foreach(struct list*, void (*)(void*));

#endif
