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




