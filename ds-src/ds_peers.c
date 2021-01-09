#include "ds_peers.h"
#include "../rb_tree.h"
#include <pthread.h>

/** Le operazioni gestite da qui vanno
 * gestite utilizzando dei mutex per
 * evitare disastri a causa
 * dell'interazione tra il thread
 * principale e quello a gestione del
 * sottosistema UDP.
 */

pthread_mutex_t guard = PTHREAD_MUTEX_INITIALIZER;
struct rb_tree* tree;

int peers_init(void)
{
    if (tree != NULL)
        return -1;

    tree = rb_tree_init(NULL);
}

int peers_clear(void)
{
    if (tree == NULL)
        return -1;

    /* questo sistema strano di
     * locking è usato per evitare
     * interferenze con l'altro thread */
    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    rb_tree_destroy(tree);
    tree = NULL;
    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}

