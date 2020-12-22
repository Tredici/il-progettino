#include "rb_tree.h"

/**
 * Macro definita per rendere le funzioni
 * indipendenti dall'eventuale utilizzo
 * futuro di una sentinella diversa da NULL
 */
#define IS_NIL(tree, p) ((p) == NULL)
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
    size_t len;
};

static elem* elem_init(long int key, void* value)
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

    return ans;
}

/** Libera la memoria occupata da p e da tutti i suoi discendenti.
 * Se specificata, invoca la funzione di cleanup fornita sui valori salvati.
 */
static void elem_destroy(elem* p, void (*cleanup_f) (void*))
{
    if (p == NULL) 
        return;
    if (cleanup_f != NULL)
        cleanup_f(p->value);
    elem_destroy(p->left,  cleanup_f);
    elem_destroy(p->right, cleanup_f);
    free(p);
}

/** Inizializza un albero rosso nero
 *
 */
struct rb_tree* rb_tree_init(struct rb_tree* tree)
{
    struct rb_tree* ans;

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

    return ans;
}

struct rb_tree* rb_tree_clear(struct rb_tree* tree)
{
    elem_destroy(tree->root, tree->cleanup_f);
    tree->root = NULL;
    tree->len = 0;
    return tree;
}

void rb_tree_destroy(struct rb_tree* tree)
{
    rb_tree_clear(tree);
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
static elem* right_rotate(struct rb_tree* tree, elem* node)
{
    elem* ans;
    elem *to_move;

    if (node == NULL)
    {
        return NULL;
    }
    ans = node->left;
    if (ans == NULL)
    {
        return NULL;
    }
    to_move = ans->right;
    node->left = to_move;
    if (to_move != NULL)
        to_move->parent = node;

    ans->parent = node->parent;
    if (ans->parent == NULL)
    {
        /* Avviene un cambio di radice */
        tree->root = ans;
    }
    else if (node == node->parent->left)
    {
        node->parent->left = ans;
    }
    else
    {
        node->parent->right = ans;
    }

    ans->right = node;
    node->parent = ans;

    return ans;
}

/** Ruota un nodo con il suo figlio destro
 *  RIGHT-ROTATE pag. 259
 */
static elem* left_rotate(struct rb_tree* tree, elem* node)
{
    elem* ans;
    elem *to_move;

    if (node == NULL)
    {
        return NULL;
    }

    ans = node->right;
    if (ans == NULL)
    {
        return NULL;
    }
    to_move = node->left;
    node->right = to_move;
    if (to_move != NULL)
        to_move->parent = node;

    ans->parent = node->parent;
    if (ans->parent == NULL)
    {
        /* Avviene un cambio di radice */
        tree->root = ans;
    }
    else if (node == node->parent->left)
    {
        node->parent->left = ans;
    }
    else
    {
        node->parent->right = ans;
    }

    ans->left = node;
    node->parent = ans;

    return ans;
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

    while (next != NULL)
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
    new_item = elem_init(key, val);
    if (new_item == NULL)
    {
        return -1;
    }
    new_item->parent = curr;
    if (curr == NULL)
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

