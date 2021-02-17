#include "../commons.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    int status;

    printf("main(): (%ld)\n", (long) getpid());

    switch (fork())
    {
    case -1:
        errExit("Andata male\n");
        return 1;
        break;
    
    case 0:
        printf("Figlio (%ld)\n", (long) getpid());
        errExit("!TEST errExit (%s) [pid][%ld] !\n", "son", (long)getpid());
        return 2;
        break;

    default:
        printf("main(): waiting for child\n");
        wait(&status);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_FAILURE)
        {
            fprintf(stderr, "*** DISASTRO errExit() ***\n");
            return EXIT_FAILURE;
        }
        break;
    }

    printf("main() terminato con successo\n");

    return 0;
}
