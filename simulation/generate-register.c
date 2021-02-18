/* Genera casualmente un file corrispondente a un registro
 * per il peer e la data specificate */

#include "../commons.h" /* per argParseIntRange */
#include "../register.h"
#include "../time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ARGNAME "porta"
#define ENTRY_PER_REGISTER 5

void usageHelp(const char* p)
{
    printf("Usage:\n\t%s <" ARGNAME ">\n", p);

    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int port, i, j;
    struct tm day;
    struct e_register* R;
    struct entry* E;
    char filename[32], date[16];

    if (argc != 2)
    {
        usageHelp(argv[0]);
        return 1;
    }

    srand(time(NULL));

    port = argParseIntRange(argv[1], "port", 1024, (1<<16)-1);

    /* crea registri per lo scorso mese */
    day = time_date_today();
    for (i = 0; i != 30; ++i)
    {
        /* decrementa la data */
        time_date_dec(&day, 1);
        time_serialize_date(date, &day);
        R = register_create_date(NULL, port, &day);
        if (R == NULL)
            fatal("register_create_date");

        if (register_filename(R, filename, sizeof(filename)) == NULL)
            fatal("register_filename");

        /* controlla se il file esiste già */
        if (access(filename, F_OK) == 0)
        {
            printError("Il file %s esiste già!\n", filename);
        }
        else
        {
            /* carica il file */
            for (j = 0; j != ENTRY_PER_REGISTER; ++j)
            {
                E = register_new_entry_date(NULL, rand()&1? SWAB:NEW_CASE,
                    1+rand()%10, 0, &day);
                if (register_add_entry(R, E) != 0)
                    fatal("register_add_entry");

                register_free_entry(E);
            }

            /* scrive il file */
            printf("Writing file %s\n", filename);
            if (register_flush(R, 0) == -1)
                fatal("register_flush: file %s", filename);
            printf("SUCCESS!\n");
        }

        register_destroy(R);
    }

    return 0;
}
