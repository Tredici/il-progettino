#include "rb_tree.h"
#include <string.h>

/**
 * Macro definita per rendere le funzioni
 * indipendenti dall'eventuale utilizzo
 * futuro di una sentinella diversa da NULL
 */
#define IS_NIL(tree, p) ((p) == NULL || (p) == (tree)->nil)
/**
 * Ottiene i figli destro o sinistro in modo
 * sicuro aggirando l'eventuale sentinella
 */
#define GET_LEFT(tree, p) ((p) != NULL ? (p)->left : NULL )
#define GET_RIGHT(tree, p) ((p) != NULL ? (p)->left : NULL )

/* Ottiene il colore del nodo in questione */
#define GET_COLOR(n)  ((n) != NULL ? (n)->col    : BLACK)
/* Ottiene il padre, eventualmente NULL */
#define GET_PARENT(n) ((n) != NULL ? (n)->parent : NULL )
/* Ottiene il colore del padre, default NERO */
#define GET_PCOLOR(n) ((n)->parent != NULL ? (n)->parent->col    : BLACK)
/* È un figlio destro o sinistro */
#define IS_LEFT_SON(n) ((n)->parent->left == (n))
#define IS_RIGHT_SON(n) ((n)->parent->right == (n))

enum color {
    BLACK,
    RED
};

typedef struct elem
{
    struct elem* parent;
    struct elem* left;
    struct elem* right;

    long int key;
    void* value;

    enum color col;
} elem;

struct rb_tree {
    void (*cleanup_f)(void*);
    elem* root;
    elem* nil;  /* sentinella */
    size_t len;
};

static elem* elem_init(struct rb_tree* tree, long int key, void* value)
{
    elem* ans;

    ans = (elem*)malloc(sizeof(elem));
    if (ans == NULL)
    {
        return NULL;
    }
    memset(ans, 0, sizeof(elem));

    ans->col = RED;
    ans->key = key;
    ans->value = value;
    /* Inizializza con la sentinella */
    ans->right = tree->nil;
    ans->left = tree->nil;
    ans->parent = tree->nil;

    return ans;
}

/** Libera la memoria occupata da p e da tutti i suoi discendenti.
 * Se specificata, invoca la funzione di cleanup fornita sui valori salvati.
 */
static void elem_destroy(struct rb_tree* tree, elem* p, void (*cleanup_f) (void*))
{
    if (IS_NIL(tree, p))
        return;
    if (cleanup_f != NULL)
        cleanup_f(p->value);
    elem_destroy(tree, p->left,  cleanup_f);
    elem_destroy(tree, p->right, cleanup_f);
    free(p);
}

/** Inizializza un albero rosso nero
 *
 */
struct rb_tree* rb_tree_init(struct rb_tree* tree)
{
    struct rb_tree* ans;
    elem* nil;

    if (tree == NULL)
    {
        ans = (struct rb_tree*)malloc(sizeof(struct rb_tree));
        if (ans == NULL)
        {
            return NULL;
        }
    }
    else
    {
        ans = tree;
    }
    memset(ans, 0, sizeof(struct rb_tree));

    /* Inizializza la sentinella */
    nil = elem_init(ans, 0, NULL);
    if (nil == NULL)
    {
        return NULL;
    }
    nil->col = BLACK;
    ans->nil = nil;
    ans->root = nil;

    return ans;
}

struct rb_tree* rb_tree_clear(struct rb_tree* tree)
{
    elem_destroy(tree, tree->root, tree->cleanup_f);
    tree->root = NULL;
    tree->len = 0;
    return tree;
}

void rb_tree_destroy(struct rb_tree* tree)
{
    rb_tree_clear(tree);
    free(tree->nil); /* Elimina la sentinella */
    free(tree);
}

void (*rb_tree_set_cleanup_f(struct rb_tree* tree, void (*cleanup_f)(void*)))(void*)
{
    void (*ans)(void*);

    if (tree == NULL)
    {
        return NULL;
    }
    ans = tree->cleanup_f;
    tree->cleanup_f = cleanup_f;

    return ans;
}

void (*rb_tree_get_cleanup_f(struct rb_tree* tree))(void*)
{
    if (tree == NULL)
    {
        return NULL;
    }

    return tree->cleanup_f;
}


/** Ruota un nodo con il suo figlio sinistro
 *  LEFT-ROTATE pag. 259
 */
void right_rotate(struct rb_tree* tree, elem* x)
{
    elem* y;

    y = x->left;
    x->left = y->right;
    if (!IS_NIL(tree, y->right))
        y->right->parent = x;

    y->parent = x->parent;
    if (IS_NIL(tree, y->parent))
    {
        /* Avviene un cambio di radice */
        tree->root = y;
    }
    else if (x == x->parent->right)
    {
        x->parent->right = y;
    }
    else
    {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}

/** Ruota un nodo con il suo figlio destro
 *  RIGHT-ROTATE pag. 259
 */
void left_rotate(struct rb_tree* tree, elem* x)
{
    elem* y;

    y = x->right;
    x->right = y->left;
    if (!IS_NIL(tree, y->left))
        y->left->parent = x;

    y->parent = x->parent;
    if (IS_NIL(tree, y->parent))
    {
        /* Avviene un cambio di radice */
        tree->root = y;
    }
    else if (x == x->parent->left)
    {
        x->parent->left = y;
    }
    else
    {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

/** Sposta un nodo da un posto all'altro
 *  in un albero rosso nero
 * Cormen pag. 267
 */
static void rb_transplant(struct rb_tree* tree, elem* u, elem* v)
{
    if (IS_NIL(tree, u->parent))
    {
        tree->root = v;
    }
    else if (u == u->parent->left)
    {
        u->parent->left = v;
    }
    else
    {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

/** Funzioni ausiliarie da utilizzare altrove
 * Cormen pag. 241
 */
static elem* tree_minimum(struct rb_tree* tree, elem* p)
{
    while (!IS_NIL(tree, p->left))
        p = p->left;
    return p;
}
static elem* tree_maximum(struct rb_tree* tree, elem* p)
{
    while (!IS_NIL(tree, p->right))
        p = p->right;
    return p;
}

/** Ripristina le proprietà di un albero rosso nero
 *  in seguito a un inserimento
 * Cormen pag. 261
 */
static void rb_tree_insert_fixup(struct rb_tree* tree, elem* new_item)
{
    elem* parent;
    elem* uncle;
    elem* grandpa;

    /* ripeti fino a che non ripristini la situazione */
    while (GET_PCOLOR(new_item) == RED)
    {
        parent  = GET_PARENT(new_item);
        grandpa = GET_PARENT(parent);

        if (IS_LEFT_SON(parent))
        {
            /* il padre è un figlio sinistro */
            uncle = grandpa->right; /* quindi lo zio è un figlio destro */

            if (GET_COLOR(uncle) == RED) /* Padre e zio rossi */
            {
                /* "Sposta" il problema verso la radice */
                parent->col = BLACK;
                uncle->col = BLACK;
                grandpa->col = RED;
                /*A questo punto il n. di nodi neri in ciascun percorso è invariato*/
                new_item = grandpa;
                /* A questo punto padre e nonno cambiano ovviamente */
            }
            else /* Lo zio è nero */
            {
                if (IS_RIGHT_SON(new_item)) /* Caso 2: figlio destro di un sinistro */
                {
                    new_item = parent;
                    /* padre e nonno CAMBIANO! */
                    left_rotate(tree, new_item);
                    /* ma cambiano ANCHE in seguito
                        a una ROTAZIONE!!! */
                    parent = GET_PARENT(new_item);
                    grandpa = GET_PARENT(parent);
                }
                /* Caso 3, eseguito SEMPRE dopo il 2 */
                parent->col  = BLACK;
                grandpa->col = RED;
                right_rotate(tree, grandpa);
            }
        }
        else
        {
            /* Speculare a sopra, invertendo destra e sinistra */
            /* il padre è un figlio destro */
            uncle = grandpa->left;

            if (GET_COLOR(uncle) == RED)
            {
                parent->col = BLACK;
                uncle->col = BLACK;
                grandpa->col = RED;
                new_item = grandpa;
            }
            else
            {
                if (IS_LEFT_SON(new_item))
                {
                    new_item = parent;
                    right_rotate(tree, new_item);
                    parent = GET_PARENT(new_item);
                    grandpa = GET_PARENT(parent);
                }
                parent->col  = BLACK;
                grandpa->col = RED;
                left_rotate(tree, grandpa);
            }
        }
    }
    /* Colora la radice di nero */
    tree->root->col = BLACK;
}


/** Inserisce un nuovo nodo nell'albero o aggiorna il valore corrente
 *
 * Ritorna 0 in caso di successo, un valore non nullo in caso di errore.
 */
int rb_tree_set(struct rb_tree* tree, long int key, void* val)
{
    elem* new_item;
    elem* curr, *next;

    curr = NULL;
    next = tree->root;

    while (!IS_NIL(tree, next))
    {
        curr = next;

        if (curr->key < key)
        {
            /* i maggiori vanno a destra */
            next = curr->right;
        }
        else if (curr->key > key)
        {
            /* i minori a sinistra */
            next = curr->left;
        }
        else
        {
            /* CASO SPECIALE! Aggiorna vecchio valore! Non fa altro */
            curr->value = val;
            return 0;
        }
    }
    /* Ha trovato il punto dell'albero dove finirà */
    new_item = elem_init(tree, key, val);
    if (new_item == NULL)
    {
        return -1;
    }
    if (IS_NIL(tree, curr))
    {
        /* l'albero è vuoto, aggiungiamo la radice */
        tree->root = new_item;
        /* parent è di default tree->nil */
    }
    else
    {
        /* il parent va impostato adeguatamente */
        new_item->parent = curr;

        if (curr->key < key)
        {
            /* maggiore: posizionato a destra */
            curr->right = new_item;
        }
        else
        {
            /* minore: posizionato a sinistra */
            curr->left = new_item;
        }
    }
    /* Potrebbe aver infranto la regola dei colori */
    rb_tree_insert_fixup(tree, new_item);

    tree->len++;

    return 0;
}

/** Cerca un valore nell'albero e lo restituisce tramite il terzo argomento, oppure si limita a verificare che questo esista
 *
 *  Ritorna 0 in caso di successo, un valore non nullo in caso di errore.
 */
int rb_tree_get(struct rb_tree* tree, long int key, void** val)
{
    elem* curr, *next;

    curr = NULL;
    next = tree->root;

    while (!IS_NIL(tree, next))
    {
        curr = next;

        if (curr->key < key)
        {
            next = curr->right;
        }
        else if (curr->key > key)
        {
            next = curr->left;
        }
        else
        {
            /* Found! */
            if (val != NULL)
                *val = curr->value;
            return 0;
        }
    }
    /* Not found! */
    return -1;
}

/** Ripristina l'integrità di un albero in seguito alla
 *  rimozione di un elemento
 * Cormen Pag. 270
 */
static void rb_delete_fixup(struct rb_tree* tree, elem* x)
{
    elem* w;

    while (x != tree->root && x->col == BLACK)
    {
        if (x == x->parent->left)
        {
            w =  x->parent->right;
            if (w->col == RED)
            {
                w->col = BLACK;
                x->parent->col = RED;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->col == BLACK && w->right->col == BLACK)
            {
                w->col = RED;
                x = x->parent;
            }
            else
            {
                if (w->right->col == BLACK)
                {
                    w->left->col = BLACK;
                    w->col = RED;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->col = x->parent->col;
                x->parent->col = BLACK;
                w->right->col = BLACK;
                left_rotate(tree, x->parent);
                x = tree->root;
            }
        }
        else
        {
            /* speculare a sopra con left e right
             * invertiti
             */
            w = x->parent->left;
            if (w->col == RED)
            {
                w->col = BLACK;
                x->parent->col = RED;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if (w->left->col == BLACK && w->right->col == BLACK)
            {
                w->col = RED;
                x = x->parent;
            }
            else
            {
                if (w->left->col == BLACK)
                {
                    w->right->col = BLACK;
                    w->col = RED;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->col = x->parent->col;
                x->parent->col = BLACK;
                w->left->col = BLACK;
                right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }

    x->col = BLACK;
}

/**
 * Cormen Pag. 268
 */
static void rb_delete(struct rb_tree* tree, elem* z)
{
    elem* y, *x;
    enum color y_original_color;

    y = z;
    y_original_color = y->col;
    if (IS_NIL(tree, z->left))
    {
        x = z->right;
        rb_transplant(tree, z, z->right);
    }
    else if (IS_NIL(tree, z->right))
    {
        x = z->left;
        rb_transplant(tree, z, z->left);
    }
    else
    {
        y = tree_minimum(tree, z->right);
        y_original_color = y->col;
        x = y->right;
        if (y->parent == z)
            x->parent = y;  /* Questo serve nel caso della sentinella */
        else
        {
            rb_transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        rb_transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->col = z->col;
    }

    /* Eventuale ripristino delle
     *  proprietà dell'albero
     */
    if (y_original_color == BLACK)
        rb_delete_fixup(tree, x);
}

/**
 * Metodo effettivo, quelle sopra sono funzioni ausiliarie
 */
int rb_tree_remove(struct rb_tree* tree, long int key, void** val)
{
    elem* curr, *next;
    void* res;

    curr = NULL;
    next = tree->root;

    while (!IS_NIL(tree, next))
    {
        curr = next;

        if (curr->key < key)
        {
            next = curr->right;
        }
        else if (curr->key > key)
        {
            next = curr->left;
        }
        else
        {
            /* Found! */
            res = curr->value;
            break;
        }
    }

    if (IS_NIL(tree, next))
    {
        /* Not found! */
        return -1;
    }

    /* Elimina il nodo */
    rb_delete(tree, curr);
    /* La dimensione è diminuita di uno */
    tree->len--;

    /* Valuta se restituire il valore oppure pulirlo */
    if (val != NULL)
    {
        *val = res;
    }
    else if (tree->cleanup_f != NULL)
    {
        tree->cleanup_f(res);
    }

    return 0;
}

ssize_t rb_tree_size(const struct rb_tree* tree)
{
    if (tree == NULL)
        return -1;

    return tree->len;
}

int rb_tree_min(struct rb_tree* tree, long int* key, void** value)
{
    elem* e;

    if (tree == NULL)
        return -1;

    e = tree_minimum(tree, tree->root);
    if (IS_NIL(tree, e))
        return -1;

    if (key != NULL)
        *key = e->key;

    if (value != NULL)
        *value = e->value;

    return 0;
}

int rb_tree_max(struct rb_tree* tree, long int* key, void** value)
{
    elem* e;

    if (tree == NULL)
        return -1;

    e = tree_maximum(tree, tree->root);
    if (IS_NIL(tree, e))
        return -1;

    if (key != NULL)
        *key = e->key;

    if (value != NULL)
        *value = e->value;

    return 0;
}

/** Trova e restituisce il nodo associato
 * alla chiave fornita o NULL se questo non
 * viene trovato
 */
static elem* elem_find(struct rb_tree* tree, long int key)
{
    elem* curr, *next;

    if (tree == NULL)
        return NULL;

    curr = NULL;
    next = tree->root;

    while (!IS_NIL(tree, next))
    {
        curr = next;

        if (curr->key < key)
        {
            next = curr->right;
        }
        else if (curr->key > key)
        {
            next = curr->left;
        }
        else
        {
            /* trovato! */
            return curr;
        }
    }
    /* Non trovato */
    return NULL;
}

/** Ricava l'elemento precedente a quello dato
 * Perfettamente speculare alla precedente
 * left|right e maximum|minimum scambiati
 */
static elem* elem_prev(struct rb_tree* tree, elem* e)
{
    elem* curr;
    elem* next;

    if (tree == NULL || IS_NIL(tree, e))
        return NULL;

    /** caso semplice, il nodo ha dei discendenti
     * a destra
     */
    if (!IS_NIL(tree, e->left))
    {
        next = tree_maximum(tree, e->left);
    }
    else
    {
        curr = e;
        next = curr->parent;
        /* bisogna tornare indietro */
        while (curr != tree->root && IS_LEFT_SON(curr))
        {
            curr = next;
            next = next->parent;
        }
        if (curr == tree->root)
        {
            /* non c'è niente da fare */
            return NULL;
        }
        /* arrivati a questo punto il successore è
         * l'antenato dato che sta alla sinistra
         * del nodo di partenza*/
    }

    return next;
}

/** Dato un elemento ottiene il suo successore,
 * il primo immediatamente a destra
 */
static elem* elem_next(struct rb_tree* tree, elem* e)
{
    elem* curr;
    elem* next;

    if (tree == NULL || IS_NIL(tree, e))
        return NULL;

    /** caso semplice, il nodo ha dei discendenti
     * a destra
     */
    if (!IS_NIL(tree, e->right))
    {
        next = tree_minimum(tree, e->right);
    }
    else
    {
        curr = e;
        next = curr->parent;
        /* bisogna tornare indietro */
        while (curr != tree->root && IS_RIGHT_SON(curr))
        {
            curr = next;
            next = next->parent;
        }
        if (curr == tree->root)
        {
            /* non c'è niente da fare */
            return NULL;
        }
        /* deve essere vero allora IS_LEFT_SON(curr) */
    }

    return next;
}

int rb_tree_next(struct rb_tree* tree, long int base, long int* key, void** value)
{
    elem* node;

    if (tree == NULL)
        return -1;

    node = elem_find(tree, base);
    if (node == NULL)
        return -1;

    node = elem_next(tree, node);
    if (node == NULL)
        return -1;
    
    if (key != NULL)
        *key = node->key;
    if (value != NULL)
        *value = node->value;

    return 0;
}

int rb_tree_prev(struct rb_tree* tree, long int base, long int* key, void** value)
{
    elem* node;

    if (tree == NULL)
        return -1;

    node = elem_find(tree, base);
    if (node == NULL)
        return -1;

    node = elem_prev(tree, node);
    if (node == NULL)
        return -1;
    
    if (key != NULL)
        *key = node->key;
    if (value != NULL)
        *value = node->value;

    return 0;
}

/* il valore restituito va nel nuovo albero; per ora ignora possibili disastri */
elem* clone_HELPER(
            struct rb_tree* NEW,
            const struct rb_tree* OLD,
            const elem* curr,
            void*(*copy_fun)(void*))
{
    elem* ans;
    elem* l_son;
    elem* r_son;

    /* se si è arrivati a una sentinella ne restituisce un'altra */
    if (IS_NIL(OLD, curr))
        return NEW->nil;

    ans = elem_init(NEW, curr->key, copy_fun(curr->value));
    if (ans == NULL)
        return NULL;
    /* stesso colore */
    ans->col = curr->col;
    /* figlio sinistro */
    l_son = clone_HELPER(NEW, OLD, curr->left, copy_fun);
    if (l_son == NULL)
    {
        if (NEW->cleanup_f != NULL)
            NEW->cleanup_f(ans->value);
        free((void*)ans);
        return NULL;
    }
    else
    {
        ans->left = l_son;
        l_son->parent = ans;
    }
    /* ora tocca al figlio destro */
    r_son = clone_HELPER(NEW, OLD, curr->right, copy_fun);
    if (r_son == NULL)
    {
        /* distrugge il sottoalbero sinistro già creato */
        elem_destroy(NEW, l_son, NEW->cleanup_f);
        /* come sopra */
        if (NEW->cleanup_f != NULL)
            NEW->cleanup_f(ans->value);
        free((void*)ans);
        return NULL;
    }
    else
    {
        ans->right = r_son;
        r_son->parent = ans;
    }

    return ans;
}

struct rb_tree* rb_tree_clone(const struct rb_tree* tree, void*(*fun)(void*))
{
    struct rb_tree* ans;
    elem* root;

    if (tree == NULL || fun == NULL)
        return NULL;

    ans = rb_tree_init(NULL);
    if (ans == NULL)
        return NULL;

    ans->cleanup_f = tree->cleanup_f; /* stesso cleanup */

    if (tree->len != 0)
    {
        root = clone_HELPER(ans, tree, tree->root, fun);
        if (root == NULL)
        {
            free(ans);
            return NULL;
        }
        ans->root = root;
        ans->len = tree->len; /* dimensione totale */
    }

    return ans;
}

/** Funzione ausiliaria che applica su tutti
 * i nodi dell'albero la funzione fun in ordine
 */
static void inorder(struct rb_tree* tree, elem* node, void(*fun)(long int, void*))
{
    if (IS_NIL(tree, node))
        return;

    inorder(tree, node->left, fun);
    fun(node->key, node->value);
    inorder(tree, node->right, fun);
}

int rb_tree_foreach(struct rb_tree* tree, void(*fun)(long int, void*))
{
    if (tree == NULL || fun == NULL)
        return -1;

    inorder(tree, tree->root, fun);

    return 0;
}

/** Come inorder ma per accumulate
 */
static void inorder_acc(struct rb_tree* tree, elem* node, void(*fun)(long int, void*, void*), void* base)
{
    if (IS_NIL(tree, node))
        return;

    inorder_acc(tree, node->left, fun, base);
    fun(node->key, node->value, base);
    inorder_acc(tree, node->right, fun, base);
}

int rb_tree_accumulate(struct rb_tree* tree, void(*fun)(long int, void*, void*), void* base)
{
    if (tree == NULL || fun == NULL)
        return -1;

    inorder_acc(tree, tree->root, fun, base);

    return 0;
}

#ifdef _RB_TREE_DEBUG
#include <stdio.h>
static void p_elem(const struct rb_tree* T, elem* p, int h)
{
    int i;

    for(i=0; i<h; ++i)
    {
        printf("  ");
    }
    printf("<%s - %s", (IS_NIL(T, p) ? "NIL" : "NODE" ),
        (GET_COLOR(p) == BLACK? "BLACK" : "RED" )
    );
    if (!IS_NIL(T, p))
    {
        printf(" : [ %ld : %ld ]", p->key, (long int)p->value);
    }
    printf(">\n");

    if (!IS_NIL(T, p))
    {
        p_elem(T, p->left, h+1);
        p_elem(T, p->right, h+1);
        /*for(int i=0; i<h; ++i)
        {
            printf("  ");
        }
        printf("<\\NODE>\n");*/
    }
}
void rb_tree_debug(const struct rb_tree* T)
{
    p_elem(T, T->root, 0);
}
/** Controlla l'altezza nera del nodo dato
 * In caso di problemi restituisce -key
 * del nodo "problematico"
 */
long int black_h(const struct rb_tree* T,
                const elem* node)
{
    long int left, right, ans;

    if (IS_NIL(T, node))
        return 0L;

    left = black_h(T, node->left);
    if (left < 0)
        return left;

    right = black_h(T, node->right);
    if (right < 0)
        return right;

    if (left != right)
    {
        ans = -node->key;
        fprintf(stderr, "*** DISASTRO nodo [%ld] ***\n", -ans);
    }
    else
    {
        ans = left + (node->col == BLACK? 1 : 0);
    }

    return ans;
}
/** Verifica che l'altezza nera di tutte le foglie
 *  sia la stessa
 */
long int rb_tree_check_integrity(const struct rb_tree* T)
{
    long int ans;

    if (!IS_NIL(T, T->root) && T->root->col == RED)
    {
        ans =  -T->root->key;
        fprintf(stderr, "*** DISASTRO root [%ld] ***\n", -ans);
    }
    else
    {
        ans = black_h(T, T->root);
    }

    return ans;
}
#endif
