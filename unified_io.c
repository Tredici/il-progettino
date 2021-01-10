#include "unified_io.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/** Struttura che sarà usata per conservare
 * i messaggi generati dai vari thread.
 */
struct io_message
{
    enum unified_io_type type;
    char* msg;
};

static struct io_message* io_message_init(enum unified_io_type type, const char* msg)
{
    struct io_message* ans;

    if (msg == NULL || (type < 0 || type >= UNIFIED_IO_LIMIT))
        return NULL;

    ans = malloc(sizeof(struct io_message));
    if (ans == NULL)
        return NULL;

    /* più per tradizione che per necessità */
    memset(ans, 0, sizeof(struct io_message));
    ans->type = type;
    ans->msg = strdup(msg);

    return ans;
}

static void io_message_destroy(struct io_message* iom)
{
    if (iom == NULL)
        return;

    free(iom->msg);
    free(iom);
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

int unified_io_push(enum unified_io_type type, const char* msg)
{
    struct io_message* iom;

    iom = io_message_init(type, msg);
    if (iom == NULL)
        return -1;

    if (queue_push(message_queue, (void*)iom) < 0)
    {
        io_message_destroy(iom);
        return -1;
    }

    return 0;
}

int unified_io_print(int flag)
{
    struct io_message* iom;

    if (message_queue == NULL)
        return -1;

    if (queue_pop(message_queue, (void*)&iom, !!flag) < 0)
    {
        if (errno == ENODATA)
            errno = EWOULDBLOCK;
        return -1;
    }

    switch (iom->type)
    {
    case UNIFIED_IO_NORMAL:
        fprintf(stdout, "%s\n", iom->msg);
        break;

    case UNIFIED_IO_ERROR:
        fprintf(stderr, "%s\n", iom->msg);
        break;

    case UNIFIED_IO_LIMIT:
        /* mai raggiunto - serve solo ad
         * eliminare un warning durante
         * la compilazione */
        break;
    }

    return 0;
}
