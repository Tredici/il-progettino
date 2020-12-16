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

int rb_tree_set(struct rb_tree*, long int, void*);


#endif
