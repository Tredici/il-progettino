#include "deque.h"
#include <stdlib.h>
#include <string.h>

struct delem
{
    struct delem* prev;
    struct delem* next;
    void* val;
};

typedef struct delem delem;

struct deque
{
    void(*cleanup_f)(void*);
    delem* first;
    delem* last;
    size_t len;
};

struct deque* deque_init(struct deque* d)
{
    struct deque* ans;

    if (d == NULL)
    {
        ans = malloc(sizeof(struct deque));
        if (ans == NULL)
            return NULL;
    }
    else
    {
        ans = d;
    }

    memset(ans, 0, sizeof(struct deque));

    return ans;
}


