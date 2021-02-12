#define _GNU_SOURCE /* per PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP */
#define _XOPEN_SOURCE 500

#include "unified_io.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

/** ID del thread che attiva il sottosistema
 * di IO.
 * Dovrebbe essere poi egli stesso a chiuderlo.
 */
static pthread_t io_controller;

/** Struttura che sarà usata per conservare
 * i messaggi generati dai vari thread.
 */
struct io_message
{
    /* id del thread che genera il messaggio */
    pthread_t author;
    char authorName[20];
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
    ans->author = pthread_self(); /* ID del thread corrente */
    if (pthread_getname_np(ans->author, ans->authorName, sizeof(ans->authorName)) != 0)
        ans->authorName[0] = '\0';
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


static void print_message(enum unified_io_type type, const char* msg)
{
    switch (type)
    {
    case UNIFIED_IO_NORMAL:
        fprintf(stdout, "%s\n", msg);
        break;

    case UNIFIED_IO_ERROR:
        fprintf(stderr, "\033[31m%s\n\033[0m", msg);
        break;

    case UNIFIED_IO_LIMIT:
        /* mai raggiunto - serve solo ad
         * eliminare un warning durante
         * la compilazione */
        break;
    }
}

/** Wrapper per la funzione print_message
 * che si occupa di gestire la stampa del
 * messaggio passato.
 */
static void print_wrapper(const struct io_message* iom)
{
    /* verifica se stampare l'identificativo del thread
     * ma lo fa solo se il thread che ha generato il
     * messaggio non è quello che ha atttivato il sottosistema */
    if (pthread_equal(io_controller, iom->author) == 0)
    {
        if (strlen(iom->authorName) > 0)
        {
            /* errore */
            if (iom->type == UNIFIED_IO_ERROR)
                fprintf(stderr, "[%s]", iom->authorName);
            else /* "normale" */
                fprintf(stdout, "[%s]", iom->authorName);
        }
    }
    print_message(iom->type, iom->msg);
}

/** Coda per i messaggi che genereranno
 * i vari thread.
 */
static struct queue* message_queue = NULL; /* per leggibilità */

/** Per gestire la mutua esclusione e permettere di
 * utilizzare un sistema diverso dalla coda, quando
 * possibile.
 */
static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static enum unified_io_mode mode = UNIFIED_IO_ASYNC_MODE;

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

    io_controller = pthread_self();

    return 0;
}

int unified_io_set_thread_name(const char* name)
{
    pthread_t me = pthread_self();
    int res;

    /* controlla che non sia il chiamante */
    if (pthread_equal(io_controller, me))
        return -1;

    res = pthread_setname_np(me, name);
    if (res != 0)
        return -1;

    return 0;
}

int unified_io_set_mode(enum unified_io_mode new_mode)
{
    enum unified_io_mode old_mode;

    switch (new_mode)
    {
    case UNIFIED_IO_ASYNC_MODE:
    case UNIFIED_IO_SYNC_MODE:
        if (pthread_mutex_lock(&mutex) != 0)
            return -1;
        old_mode = mode;
        mode = new_mode;
        /* si occupa di svuotare la coda */
        if (new_mode == UNIFIED_IO_SYNC_MODE)
        {
            errno = 0;
            while (unified_io_print(1) == 0)
                ;
        }
        /* gestisce anche l'eventuale errore con unified_io_print */
        if (pthread_mutex_unlock(&mutex) != 0 || errno != EWOULDBLOCK)
            return -1;
        break;

    default:
        return -1;
    }

    return old_mode;
}

enum unified_io_mode unified_io_get_mode(void)
{
    return mode;
}

int unified_io_close()
{
    if (message_queue == NULL)
        return -1;

    if (unified_io_flush() == -1)
        return -1;

    queue_destroy(message_queue);

    return 0;
}

int unified_io_push(enum unified_io_type type, const char* format, ...)
{
    struct io_message* iom;
    char msg[UNIFIED_IO_MAX_MSGLEN+1];
    va_list al;
    int err;

    /* assicura che funzioni tutto correttamente */
    msg[UNIFIED_IO_MAX_MSGLEN] = '\0';

    va_start(al, format);
    err = vsnprintf(msg, UNIFIED_IO_MAX_MSGLEN, format, al);
    va_end(al);
    if (err < 0)
        return -1;

    iom = io_message_init(type, msg);
    if (iom == NULL)
        return -1;
    if (pthread_mutex_lock(&mutex) != 0)
    {
        io_message_destroy(iom);
        return -1;
    }

    /* se bisogna accodare */
    if (mode == UNIFIED_IO_ASYNC_MODE)
    {
        if (queue_push(message_queue, (void*)iom) < 0)
        {
            io_message_destroy(iom);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    else /* altrimenti stampa direttamente */
    {
        print_wrapper(iom);
        io_message_destroy(iom); /* libera la memoria */
    }

    if (pthread_mutex_unlock(&mutex) != 0)
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

    print_wrapper(iom);
    io_message_destroy(iom);

    return 0;
}

int unified_io_flush(void)
{
    int status;

    if (pthread_mutex_lock(&mutex) != 0)
        return -1;

    /* verifica che il sottosistema sia stato attivato */
    if (message_queue == NULL || (status = unified_io_get_mode()) == -1)
    {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    if (status == UNIFIED_IO_ASYNC_MODE)
    {
        if (unified_io_set_mode(UNIFIED_IO_SYNC_MODE) == -1)
        {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        if (unified_io_set_mode(UNIFIED_IO_ASYNC_MODE) == -1)
        {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    if (pthread_mutex_unlock(&mutex) != 0)
        return -1;

    return 0;
}
