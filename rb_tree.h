/** Espone una classe "albero rosso-nero"
 * Efficiente ed utile, da usare dove serve.
 */
#ifndef RB_TREE
#define RB_TREE

#include <stdlib.h>

struct rb_tree;

struct rb_tree* rb_tree_init(struct rb_tree*);
struct rb_tree* rb_tree_clear(struct rb_tree*);
void rb_tree_destroy(struct rb_tree*);

void (*rb_tree_set_cleanup_f(struct rb_tree*, void(*)(void*)))(void*);
void (*rb_tree_get_cleanup_f(struct rb_tree*))(void*);

int rb_tree_set(struct rb_tree*, long int, void*);
int rb_tree_get(struct rb_tree*, long int, void**);

/** Elimina un elemento dall'albero, eventualmente
 * ritornandone il valore se l'ultimo argomento non
 * è NULL, in caso contrario invoca su di esso
 * l'opportuna funzione di cleanup.
 *
 * Restituiscono 0 in caso di successo,
 * -1 altrimenti.
 */
int rb_tree_remove(struct rb_tree*, long int, void**);

/** Fornisce il numero di nodi dell'albero.
 */
ssize_t rb_tree_size(const struct rb_tree*);

/** Forniscono rispettivamente le coppie
 * (dipendentemente dal fatto che gli ultimi
 * argomenti non siano NULL) corrispondenti
 * ai valori delle chiavi massima e minima
 * nell'albero.
 *
 * Restituiscono 0 in caso di successo, -1
 * altrimenti.
 */
int rb_tree_min(struct rb_tree*, long int*, void**);
int rb_tree_max(struct rb_tree*, long int*, void**);

/** Permette di ottenere chiave e valore
 * del nodo successore di quello la cui
 * chiave viene specificata.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int rb_tree_next(struct rb_tree*, long int, long int*, void**);
/** Come rb_tree_next ma per il precedente
 */
int rb_tree_prev(struct rb_tree*, long int, long int*, void**);

/** In ordine, richiama la funzione argomento
 * su ciascuna coppia [key,value] dell'albero
 */
int rb_tree_foreach(struct rb_tree*, void(*)(long int, void*));

/** Permette di ottenere una copia dell'albero
 * fornito. La funzione argomento deve servire
 * unicamente a generare copie dei valori,
 * indipendentemente da quale siano le
 * corrispondenti chiavi.
 *
 * Restituisce il nuovo albero in caso di
 * successo e NULL in caso di errore.
 */
struct rb_tree* rb_tree_clone(const struct rb_tree*, void*(*)(void*));

/** L'idea alla fine è sempre quella.
 * Il terzo argomento della funzione e della
 * funzione argomento coincidono e possono
 * essere usati liberamente dall'utente.
 */
int rb_tree_accumulate(struct rb_tree*, void(*)(long int, void*, void*), void*);

#ifdef _RB_TREE_DEBUG
void rb_tree_debug(const struct rb_tree*);
long int rb_tree_check_integrity(const struct rb_tree*);
#endif

#endif
