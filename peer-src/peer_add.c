#include "peer_add.h"
#include "../repl.h"
#include <stdio.h>
#include "../register.h"
#include <ctype.h>
#include "peer_entries_manager.h"

#define C_SWAP 'T'
#define C_CASE '+'


int add(const char* args)
{
    int quantity;
    char ctype;
    enum entry_type type;
    struct entry* E;
    char enStr[32];
    
    if (sscanf(args, "%c %d", &ctype, &quantity) != 2)
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
    E = register_new_entry(NULL, type, quantity, 0);
    if (E == NULL)
        return ERR_FAIL;

    printf("Tentativo di registrare l'entry %s\n", register_serialize_entry(E, enStr, sizeof(enStr), ENTRY_SIGNATURE_OPTIONAL));

    /* Qui andrà il codice per aggiungere 
     * la nuova entry al registro del peer */
    if (addEntryToCurrent(E) != 0)
    {
        fprintf(stderr, "Tentativo fallito!\n");
        register_free_entry(E);
        return ERR_FAIL;
    }

    printf("Entry inserita con successo!\n");
    register_free_entry(E);

    return OK_CONTINUE;
}
