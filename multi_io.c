#include "multi_io.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

static int active = 0;

struct winsize ws;
struct termios termios_quo;

int multiio_start()
{
    char* tty_name;
    struct termios t, prev_t;

    /**
     * Controlla di non aver già inizializzato il sistema
     */
    if (active != 0)
    {
        return -1;
    }

    /**
     * Deve verificare di possedere un terminale
     */
    if (!( isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO) ))
    {
        return -1;
    }

    /**
     * Bisognerebbe verificare che siano connessi
     * allo stesso ma questo per ora lo trascuriamo
     *
     * usare ttyname per farlo più avanti
     */
    tty_name = ttyname(STDIN_FILENO);
    if (tty_name == NULL)
        return -1;
    tty_name = strdupa(tty_name);

    if (strcmp(ttyname, ttyname(STDOUT_FILENO))
        || strcmp(ttyname, ttyname(STDERR_FILENO)))
    {
        return -1;
    }

    /**
     * Cambia la configurazione del terminale
     */
    /* ottiene il vecchio stato */
    if (tcgetattr(STDIN_FILENO, &t) == -1)
        return -1;
    prev_t = t;

    /**
     * Salva il nuovo stato del sistema
     */
    active = 1;
    return 0;
}
