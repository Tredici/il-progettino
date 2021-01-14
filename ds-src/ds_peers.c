#include "ds_peers.h"
#include "../rb_tree.h"
#include "../ns_host_addr.h"
#include <pthread.h>

/** Struttura che sarà usata per
 * mantenere i dati associati ad
 * ogni peer presente nella rete.
 */
struct peer
{
    long int id; /* id progressivo associato ai peer */
    struct ns_host_addr ns_addr; /* dati per raggiungerlo */
};


/** Le operazioni gestite da qui vanno
 * gestite utilizzando dei mutex per
 * evitare disastri a causa
 * dell'interazione tra il thread
 * principale e quello a gestione del
 * sottosistema UDP.
 */

pthread_mutex_t guard = PTHREAD_MUTEX_INITIALIZER;
struct rb_tree* tree;
/* serve per assegnare un id progressivo a ciascun
 * peer si connetta alla rete */
long int counter;

int peers_init(void)
{
    if (tree != NULL)
        return -1;

    tree = rb_tree_init(NULL);
    counter = 0;
    rb_tree_set_cleanup_f(tree, &free);
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

