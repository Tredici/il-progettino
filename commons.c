#define _GNU_SOURCE

#include "commons.h"
#include <stdio.h>
#include <stdarg.h> /* per un numero variabile di argomenti */
#include <stdlib.h>
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/syscall.h>
#include <signal.h>

#ifndef NDEBUG
#include <execinfo.h> /* per backtrace */
#endif

#define MAX_ERR_L 256
#define MAX_CALL_DEPTH 256

int min(int x, int y) { return x<y ? x : y; }
int max(int x, int y) { return x>y ? x : y; }

void errExit(const char *format, ...)
{
    va_list args;

    fflush(stdout);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

void fatal(const char *format, ...)
{
#ifndef NDEBUG
    void* btCalls[MAX_CALL_DEPTH] = {};
    int btLen;
    char errStr[64];
#endif
    va_list args;
    int errCpy = errno;
    pid_t tid;

    /* man syscall(2) - EXAMPLE - Linux specific */
    tid = syscall(SYS_gettid);

    fflush(stdout);
    fprintf(stderr, "fatal:\nmessage:\t");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    (void)strerror_r(errCpy, errStr, sizeof(errStr));
    fprintf(stderr, "errno = (%d): %s\n", errCpy, errStr);
    fprintf(stderr, "tid = (%ld);\npid = (%ld);\nppid = (%ld)\n",
        (long)tid, (long)getpid(), (long)getppid());

    fprintf(stderr, "gid = (%ld);\nsid = (%ld);\n",
        (long)getgid(), (long)getsid(0));


#ifndef NDEBUG
    fprintf(stderr, "Backtrace:\n");
    btLen = backtrace(btCalls, MAX_CALL_DEPTH);
    fprintf(stderr, "backtrace() returned %d adddresses\n", btLen);
    fflush(stderr);
    /* mette in output */
    backtrace_symbols_fd(btCalls, (size_t)btLen, STDERR_FILENO);
#endif

    fprintf(stderr, "Aborting...\n");
    abort();
}

/** Vagamente ispirata a ttySetCbreak
 * a pagina 1310 di The Linux
 * Programming Interface
 */
int waitForInput(int flag)
{
    struct termios tp, save;
    char buf;   /* basta 1 carattere per il test */
    int ans;
    int optional_actions;
    struct pollfd pfd;

    ans = 0;
    fflush(stdout);

    if (tcgetattr(STDIN_FILENO, &tp) == -1)
        return -1;

    save = tp;
    /* no echo */
    /* siabilità la moalità canonica "una riga alla volta" */
    tp.c_lflag &= ~(ECHO|ICANON);
    /* i segnali devono funzionare sempre */
    tp.c_lflag |= ISIG;

    /* nessun timeout */
    tp.c_cc[VTIME] = 0;
    /* come convertire in 1 o 0 un intero */
    /* imposta il minimo numero di caratteri
    * da leggere:
    * 0  - POLLING
    * !0 - wait */
    tp.c_cc[VMIN] = !flag;

    /* prepara il terminale per il test */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tp) == -1)
        return -1;

    if (flag)
    {
        /* poll per l'input */
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;

        /* vediamo se funziona */
        switch (poll(&pfd, 1, 0))
        {
        case 0:
            /* si bloccherebbe */
            ans = 1;
            break;
        case 1:
            /* input disponibile */
            ans = 0;
            break;
        default:
            /* errore grave */
            ans = -1;
            break;
        }
    }
    else
    {
        /* qualora ci si voglia bloccare */
        /* prova a eseguire la lettura */
        switch (read(STDIN_FILENO, (void*)&buf, 1))
        {
        case 0:
            /* POLLING - NODATA */
            ans = 1;
            break;
        case 1:
            /* tutto ok */
            ans = 0;
            break;

        default:
            ans = -1;
            break;
        }
    }

    switch (ans)
    {
    case 0:
        /* Tasto premuto */
        optional_actions = TCSAFLUSH;
        break;
    case 1:
        /* nessun input */
        errno = EWOULDBLOCK;
        /* per impedire che si perda
         * dell'input tra il momento
         * del test e il ripristino
         * delle condizioni iniziali */
        optional_actions = TCSANOW;
        ans = -1;
        break;

    default:
        /* errore */
        optional_actions = TCSANOW;
        ans = -1;
        break;
    }

    /* ripristina lo status quo */
    if (tcsetattr(STDIN_FILENO, optional_actions, &save) == -1)
        return -1;

    return ans;
}

int argParseInt(const char* arg, const char* onErrorName)
{
    long int lnum;
    char* end;

    errno = 0;
    lnum = strtol(arg, &end, 10);

    /* esegue tutti i possibili controlli di
     * correttezza del caso */
    if (errno != 0 || (long)(int)lnum != lnum
        || arg == end || *end != '\0')
    {
        /* controlla che il numero sia rappresentabile
         * nella dimensione di un int */
        errExit("Error parsing <%s>\n", onErrorName);
    }

    return (int)lnum;
}

int argParseIntRange(const char* arg, const char* onErrorName, int min, int max)
{
    int val;

    val = argParseInt(arg, onErrorName);
    if (!(min <= val && val <= max))
        errExit("Error <%s> out of range [%d;%d]\n", onErrorName, min, max);

    return val;
}