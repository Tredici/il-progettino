#define _GNU_SOURCE /* per i mutex ricorsivi */

#include "ds_peers.h"
#include "../rb_tree.h"
#include "../ns_host_addr.h"
#include "../commons.h"
#include "../messages.h"
#include "../unified_io.h"
#include <pthread.h>
#include <stdio.h>

/** Struttura che sarà usata per
 * mantenere i dati associati ad
 * ogni peer presente nella rete.
 */
struct peer
{
    uint32_t requestPid; /* pid della richiesta di login */
    uint32_t id; /* id progressivo associato ai peer */
    struct ns_host_addr ns_addr; /* dati per raggiungerlo */
};


/** Le operazioni gestite da qui vanno
 * gestite utilizzando dei mutex per
 * evitare disastri a causa
 * dell'interazione tra il thread
 * principale e quello a gestione del
 * sottosistema UDP.
 */

pthread_mutex_t guard = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
struct rb_tree* tree;
/* serve per assegnare un id progressivo a ciascun
 * peer si connetta alla rete */
uint32_t counterID;

int peers_init(void)
{
    if (tree != NULL)
        return -1;

    tree = rb_tree_init(NULL);
    counterID = 0;
    rb_tree_set_cleanup_f(tree, &free);
    return 0;
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

/** Funzione ausiliaria per generare un oggetto
 * struct peer_data a partire dal corrispondente
 * struct peer.
 */
static struct peer_data*
make_peer_data_form_peer(const struct peer* P)
{
    struct peer_data* ans;
    int16_t orderPort;

    if (ns_host_addr_get_port(&P->ns_addr, &orderPort) == -1)
        return NULL;

    ans = peer_data_init(NULL, P->id, orderPort, &P->ns_tcp);
    if (ans == NULL)
        return NULL;

    return ans;
}

/* si limita a trovare i soli vicini */
int
peers_find_neighbours(
        long int key,
        struct ns_host_addr** neighbours,
        uint16_t* length)
{
    long int peerKey; /* chiave del vicinos */
    struct peer* peerValue; /* vicino */

    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    /* controllo valori ed esistenza elemento */
    if (tree == NULL || rb_tree_get(tree, key, NULL) != 0)
    {
        if (pthread_mutex_unlock(&guard) != 0)
            errExit("*** peers_find_neighbours double fault [pthread_mutex_unlock] ***\n");
        return -1;
    }

    /* ricerca dei vicini */
    switch (rb_tree_size(tree))
    {
    case -1:
        /* errore */
        if (pthread_mutex_unlock(&guard) != 0)
            errExit("*** peers_find_neighbours double fault[pthread_mutex_unlock] ***\n");

        return -1;

    case 0:
        /* qui non dovrebbe mai poter arrivare */
        errExit("*** peers_find_neighbours fault (rb_tree_size(tree)==0) ***\n");
        break; /* non necessario; per sopprimere il warning */

    case 1:
        /* l'albero era prima vuoto */
        //nPeers = 0; /* elementi dellìalbero -1 */
        *length = 0;
        break;

    case 2:
        /* l'albero ha solo un altro elemento */
        /* cerca il vicino */
        /* cerca a sinistra */
        if (rb_tree_prev(tree, key, &peerKey, (void**)&peerValue) == 0);
        /* cerca a sinistra */
        else if (rb_tree_next(tree, key, &peerKey, (void**)&peerValue) == 0);
        /* non ha senso che si raggiunga */
        else
            errExit("*** peers_add_and_find_neighbours fault (size==2 no neighbour) ***\n");

        /* aggiunge il peer */
        neighbours[0] = &peerValue->ns_addr;
        *length = 1; /* e segna il numero a 1 */
        break;

    default:
        /* ci sono due vicini */
        /* cerca quello a sinistra */
        if (rb_tree_prev(tree, key, &peerKey, (void**)&peerValue) == 0);
        /* altrimenti deve prendere il maggiore di tutti */
        else if(rb_tree_max(tree, &peerKey, (void**)&peerValue) == 0);
        /*  altrimenti disastro */
        else
            errExit("*** peers_add_and_find_neighbours fault (size>2 no left neighbour) ***\n");
        /* assegna il primo peer */
        neighbours[0] = &peerValue->ns_addr;

        /* cerca quello a destra */
        if (rb_tree_next(tree, key, &peerKey, (void**)&peerValue) == 0);
        /* altrimenti deve prendere il minore di tutti */
        else if(rb_tree_min(tree, &peerKey, (void**)&peerValue) == 0);
        /*  altrimenti disastro */
        else
            errExit("*** peers_add_and_find_neighbours fault (size>2 no right neighbour) ***\n");
        /* assegna il secondo peer */
        neighbours[1] = &peerValue->ns_addr;

        /* e segna che i peer sono 2 */
        *length = 2;

        break;
    }

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}

int
peers_add_and_find_neighbours(
            uint32_t loginPid,
            const struct ns_host_addr* ns_addr,
            uint32_t* newID,
            struct ns_host_addr** neighbours,
            uint16_t* length)
{
    long int key;
    struct peer* value;
    uint16_t port;

    /* controllo dei parametri */
    if (ns_addr == NULL || newID == NULL
        || neighbours == NULL || length == NULL)
        return -1;

    if (ns_host_addr_get_port(ns_addr, &port) == -1)
        return -1;

    key = (long int)port;

    if (pthread_mutex_lock(&guard) != 0)
    {
        free(value);
        return -1;
    }

    /* controlla di non aver già ricevuto il messaggio */
    if (rb_tree_get(tree, key, (void**)&value) == 0 && value->requestPid == loginPid)
    {
        /* richiesta duplicata, mantiene il vecchio l'ID */
        *newID = value->id;
    }
    else
    {
        /* Questa richiesta non era ancora stata gestita */
        value = (struct peer*)malloc(sizeof(struct peer));
        if (value == NULL)
        {
            if (pthread_mutex_unlock(&guard) != 0)
                errExit("*** peers_add_and_find_neighbours double fault[rb_tree_set:pthread_mutex_unlock] ***\n");

            return -1;
        }
        /* salva l'ID della richiesta */
        value->requestPid = loginPid;
        value->ns_addr = *ns_addr;
        /* nuovo ID */
        *newID = ++counterID;
        value->id = counterID;
    }

    /* inserisce un nuovo valore */
    if (rb_tree_set(tree, key, (void*)value) == -1)
    {
        free(value);
        if (pthread_mutex_unlock(&guard) != 0)
            errExit("*** peers_add_and_find_neighbours double fault[rb_tree_set:pthread_mutex_unlock] ***\n");

        return -1;
    }


    /* ora cerca i vicini */
    if (peers_find_neighbours(key, neighbours, length) == -1)
    {
        free(value);
        if (pthread_mutex_unlock(&guard) != 0)
            errExit("*** peers_add_and_find_neighbours double fault[rb_tree_set:pthread_mutex_unlock] ***\n");
        return -1;
    }

    if (pthread_mutex_unlock(&guard) != 0)
    {
        /* se fallisce anche questo addio */
        if (rb_tree_remove(tree, key, NULL) != 0)
            errExit("*** peers_add_and_find_neighbours double fault[pthread_mutex_unlock:rb_tree_remove] ***\n");
        /* prova a rimuovere il nodo inserito */
        return -1;
    }

    return 0;
}

static void print_elem(long int key, void* value)
{
    char addrStr[32];
    struct peer* pr = (struct peer*)value;
    /* si esclude che l'argomento possa essere NULL */

    if (ns_host_addr_as_string(addrStr, sizeof(addrStr), &pr->ns_addr) == -1)
        errExit("*** peers_add_and_find_neighbours double fault[pthread_mutex_unlock:rb_tree_remove] ***\n");

    printf("\tport[%ld] - addr: %s\n", key, addrStr);
}

/* stampa tutti gli elementi dell'albero */
int peers_showpeers(void)
{
    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    /* il sistema non è ancora stato inizializzato */
    if (tree == NULL)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }

    printf("Totale peers: %ld\n", (long)rb_tree_size(tree));
    if (rb_tree_foreach(tree, &print_elem) == -1)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}

static void print_showneighbour(long int node, void* arg)
{
    char currStr[32];
    char prevStr[32];
    char nextStr[32];
    struct ns_host_addr* neighbours[MAX_NEIGHBOUR_NUMBER];
    struct peer* value;
    uint16_t length;

    value = (struct peer*)arg;
    if (peers_find_neighbours(node, neighbours, &length) == -1)
    {
        errExit("print_showneighbour fault [peers_find_neighbours]\n");
    }
    else
    {
        if (peers_find_neighbours(node, neighbours, &length) == -1)
            errExit("print_showneighbour fault [peers_find_neighbours]\n");

        if (ns_host_addr_as_string(currStr, sizeof(currStr), &value->ns_addr) == -1)
            errExit("print_showneighbour fault [ns_host_addr_as_string]\n");

        switch (length)
        {
        case 2:
            if (ns_host_addr_as_string(nextStr, sizeof(nextStr), neighbours[1]) == -1)
                errExit("print_showneighbour fault [ns_host_addr_as_string]\n");

        case 1:
            if (ns_host_addr_as_string(prevStr, sizeof(prevStr), neighbours[0]) == -1)
                errExit("print_showneighbour fault [ns_host_addr_as_string]\n");
        }

        printf("\t%ld - %s: {%s%s%s}\n", node, currStr, (length>0 ? prevStr : ""),
                (length>1 ? ", " : ""), (length>1 ? nextStr : ""));
    }
}

int peers_showneighbour(long int* peer)
{
    struct peer* value;

    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    if (peer == NULL)
    {
        /* li stampa tutti */
        printf("Totale peers: %ld\n", (long)rb_tree_size(tree));
        if (rb_tree_foreach(tree, &print_showneighbour) == -1)
        {
            pthread_mutex_unlock(&guard);
            return -1;
        }
    }
    else
    {
        if (rb_tree_get(tree, *peer, (void**)&value) == -1)
        {
            fprintf(stderr, "\tpeer (%ld) inesistente\n", *peer);
            pthread_mutex_unlock(&guard);
            return -1;
        }
        else
        {
            print_showneighbour(*peer, value);
        }
    }

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}

static void sendShutdown(long int peerPort, void* data, void* base)
{
    char destAddr[32];
    struct peer* p;
    struct ns_host_addr* dest;
    int sockfd;

    (void)peerPort;
    sockfd = *(int*)base;
    p = (struct peer*)data;
    dest = &p->ns_addr;

    if (ns_host_addr_as_string(destAddr, sizeof(destAddr), dest) == -1)
        errExit("*** peers_send_shutdown:sendShutdown:ns_host_addr_as_string ***\n");

    unified_io_push(UNIFIED_IO_NORMAL, "Sending to %s message MESSAGES_SHUTDOWN_REQ", destAddr);

    if (messages_send_shutdown_req_ns(sockfd, dest, p->id) == -1)
        errExit("*** peers_send_shutdown:sendShutdown:messages_send_shutdown_req_ns ***\n");
}

int peers_send_shutdown(int sockfd)
{
    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    if (tree == NULL)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }

    if (rb_tree_accumulate(tree, &sendShutdown, &sockfd) == -1)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}

int peers_number(void)
{
    int ans;

    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    if (tree == NULL)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }

    ans = (int)rb_tree_size(tree);

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return ans;
}

int peers_get_id(long int key, uint32_t* ID)
{
    struct peer* p;

    if (ID == NULL)
        return -1;

    if (tree == NULL)
        return -1;

    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    if (rb_tree_get(tree, key, (void**)&p) == -1 || p == NULL)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }
    *ID = p->id;

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}

int peers_remove_peer(long int key)
{
    if (tree == NULL)
        return -1;

    if (pthread_mutex_lock(&guard) != 0)
        return -1;

    if (rb_tree_remove(tree, key, NULL) == -1)
    {
        pthread_mutex_unlock(&guard);
        return -1;
    }

    if (pthread_mutex_unlock(&guard) != 0)
        return -1;

    return 0;
}
