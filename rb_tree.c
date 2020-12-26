#include "rb_tree.h"

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

    ans = malloc(sizeof(elem));
    if (ans == NULL)
    {
        return ans;
    }
    memset(ans, 0, sizeof(elem));

    ans->col = RED;
    ans->key = key;
    ans->value = value;
    /* Inizializza con la sentinella */
    ans->right = tree->nil;
    ans->left = tree->nil;

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
        ans = malloc(sizeof(struct rb_tree));
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

void (*rb_set_cleanup_f(struct rb_tree* tree, void (*cleanup_f)(void*)))(void*)
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

void (*rb_get_cleanup_f(struct rb_tree* tree))(void*)
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
    elem* ans;
    elem *to_move;

    ans = x->left;
    to_move = ans->right;
    x->left = to_move;
    if (!IS_NIL(tree, to_move))
        to_move->parent = x;

    ans->parent = x->parent;
    if (ans->parent == NULL)
    {
        /* Avviene un cambio di radice */
        tree->root = ans;
    }
    else if (x == x->parent->left)
    {
        x->parent->left = ans;
    }
    else
    {
        x->parent->right = ans;
    }

    ans->right = x;
    x->parent = ans;
}

/** Ruota un nodo con il suo figlio destro
 *  RIGHT-ROTATE pag. 259
 */
void left_rotate(struct rb_tree* tree, elem* x)
{
    elem* y;
    elem *to_move;

    y = x->right;
    to_move = x->left;
    x->right = to_move;
    if (!IS_NIL(tree, to_move))
        to_move->parent = x;

    y->parent = x->parent;
    if (y->parent == NULL)
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
/* Ottiene il colore del nodo in questione */
#define GET_COLOR(n)  ((n) != NULL ? (n)->col    : BLACK)
/* Ottiene il padre, eventualmente NULL */
#define GET_PARENT(n) ((n) != NULL ? (n)->parent : NULL )
/* Ottiene il colore del padre, default NERO */
#define GET_PCOLOR(n) ((n)->parent != NULL ? (n)->parent->col    : BLACK)
/* È un figlio destro o sinistro */
#define IS_LEFT_SON(n) ((n)->parent->left == (n))
#define IS_RIGHT_SON(n) ((n)->parent->right == (n))

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
            }
            else /* Lo zio è nero */
            {
                if (IS_RIGHT_SON(parent)) /* Caso 2: figlio destro di un sinistro */
                {
                    new_item = parent;
                    left_rotate(tree, new_item);
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
                if (IS_LEFT_SON(parent))
                {
                    new_item = parent;
                    right_rotate(tree, new_item);
                }
                parent->col  = BLACK;
                grandpa->col = RED;
                left_rotate(tree, grandpa);
            }
        }
    }
    /* Colora la radice di nero */
    tree->root->col = BLACK;

/* Elimina la macro create apposta */
#undef IS_RIGHT_SON
#undef IS_LEFT_SON
#undef GET_PCOLOR
#undef GET_PARENT
#undef GET_COLOR
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
    new_item->parent = curr;
    if (IS_NIL(tree, curr))
    {
        /* l'albero è vuoto, aggiungiamo la radice */
        tree->root = new_item;
    }
    else if (curr->key < key)
    {
        /* maggiore: posizionato a destra */
        curr->right = new_item;
    }
    else
    {
        /* minore: posizionato a sinistra */
        curr->left = new_item;
    }
    /* Potrebbe aver infranto la regola dei colori */
    rb_tree_insert_fixup(tree, new_item);

    tree->len++;

    return 0;
}

/** Cerca un valore nell'albero e lo restituisce tramite il terzo argomento
 *
 *  Ritorna 0 in caso di successo, un valore non nullo in caso di errore.
 */
int rb_tree_get(struct rb_tree* tree, long int key, void** val)
{
    elem* curr, *next;

    curr = NULL;
    next = tree->root;

    while (next != NULL)
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

    while (x != tree->root || x->col == BLACK)
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
                if (w->right->parent == BLACK)
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
    else if (tree->cleanup_f)
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
