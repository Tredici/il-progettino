/** Espone una classe "albero rosso-nero"
 * Efficiente ed utile, da usare dove serve.
 */
#ifndef RB_TREE
#define RB_TREE

#include <stdlib.h>
#include <string.h>

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
 * l'opportuna funzione di cleanup
 */
int rb_tree_remove(struct rb_tree*, long int, void**);

/** Fornisce il numero di nodi dell'albero.
 */
ssize_t rb_tree_size(const struct rb_tree*);

#ifdef _RB_TREE_DEBUG
void rb_tree_debug(const struct rb_tree*);
long int rb_tree_check_integrity(const struct rb_tree*);
#endif

#endif
