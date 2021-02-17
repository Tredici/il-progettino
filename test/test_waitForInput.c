/** Vediamo se ho capito come
 * realizzare system("PAUSE")
 *
 * Mi sono reso conto che system("PAUSE")
 * richiede invio, io invece accetto
 * un qualsiasi carattere
 */

#include "../commons.h"
#include <stdio.h>

int main()
{
    printf("Premere un tasto per continuare:");
    if (waitForInput(0) != 0)
    {
        errExit("*** waitForInput(0) ***");
    }

    printf("\nDONE!\n");    

    return 0;
}
