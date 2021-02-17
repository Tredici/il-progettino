/** Vediamo se il polling funziona
 */

#include "../commons.h"
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

int main()
{
    int i=0;
    struct termios td, save;

    printf("Premere un tasto per continuare:");
    errno = 0;

    tcgetattr(STDIN_FILENO, &td);
    save = td;
    td.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &td);

    while (waitForInput(1) != 0)
    {
        if (errno != EWOULDBLOCK)
            errExit("*** waitForInput(0) ***");

        //printf("\nNO input [%d] - Rilascia la CPU", i++);
        fflush(stdout);
        ++i;
        sched_yield();
        errno = 0;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &save);

    printf("\nDONE! [%d]\n", i);    

    return 0;
}
