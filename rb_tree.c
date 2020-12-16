#include "rb_tree.h"

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
 * Ritorna 0 in caso di successo, -1 in caso di errore.
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

    return 0;
}


