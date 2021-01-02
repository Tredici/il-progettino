#include "peer_add.h"
#include "../repl.h"
#include <stdio.h>
#include "../register.h"
#include <ctype.h>

#define C_SWAP 'T'
#define C_CASE '+'


int add(const char* args)
{
    int quantity;
    char ctype;
    enum entry_type type;
    struct entry* E;
    
    if (sscanf(args, "%c %d", &quantity, &ctype) != 2)
    {
        /* è andata male */
        return ERR_PARAMS;
    }

    if (quantity <= 0)
    {
        return ERR_PARAMS;
    }

    switch (toupper(ctype))
    {
    case C_CASE:
        type = NEW_CASE;
        break;
    case C_SWAP:
        type = SWAB;
        break;
    
    default:
        return ERR_PARAMS;
    }

    /* qui andrà il codice per generare */
    E = register_new_entry(NULL, type, quantity);
    if (E == NULL)
        return WRN_CONTINUE;

    /* Qui andrà il codice per aggiungere 
     * la nuova entry al registro del peer */


    register_free_entry(E);

    return OK_CONTINUE;
}
