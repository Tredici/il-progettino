#include "unified_io.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

