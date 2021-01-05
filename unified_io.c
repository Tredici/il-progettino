#include "unified_io.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** Struttura che sarà usata per conservare
 * i messaggi generati dai vari thread.
 */
struct io_message
{
    enum unified_io_type type;
    char* msg;
};

struct io_message* io_message_init(const char* msg, enum unified_io_type type)
{
    struct io_message* ans;

    ans = malloc(sizeof(struct io_message));
    if (ans == NULL)
        return NULL;

    /* più per tradizione che per necessità */
    memset(ans, 0, sizeof(struct io_message));
    ans->type = type;
    ans->msg = strdup(msg);

    return ans;
}

/** Coda per i messaggi che genereranno
 * i vari thread.
 */
struct queue* message_queue = NULL; /* per leggibilità */

int unified_io_init()
{
    struct queue* Q;

    if (message_queue != NULL)
        return -1;

    Q = queue_init(NULL, Q_CONCURRENT);
    if (Q == NULL)
        return -1;

    queue_set_cleanup_f(Q, &free);
    message_queue = Q;

    return 0;
}

int unified_io_close()
{
    if (message_queue == NULL)
        return -1;

    queue_destroy(message_queue);

    return 0;
}

