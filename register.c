#define _XOPEN_SOURCE   /* per strptime */
#include "register.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* 4 ANNO 2 MESE 2 GIORNI 2 SEPARATORI (-) */
#define DATE_LENGHT 10
/* Il separatore nella rappresentazione
    testuale delle entry */
#define SEPARATOR "|"
/* 1 CARATTERE */
#define TYPE_LENGHT 1


enum entry_type
{
    SWAB,       /* tampone */
    NEW_CASE    /* nuovo caso */
};

/** Una entry necessiterà di:
 *  tempo:
 *      +data
 *      ? +ora
 *  tipo:
 *      >tampone
 *      >nuovo caso
 *  numero:
 *      -numero di persone coinvolte
 */

struct entry
{
    struct tm e_time;
    enum entry_type type;
    int counter;
};


struct entry* register_parse_entry(const char* s, struct entry* e)
{
    struct entry* ans;
    char type;

    if (e == NULL)
    {
        ans = malloc(sizeof(struct entry));
        if (ans == NULL)
        {
            return NULL;
        }
    }
    else
    {
        ans = e;
    }
    memset(ans, 0, sizeof(struct entry));
    /**
     * Struttura di una entry
     * \d{4}-\d{2}-\d{2}|[NT]|\d+
     */
    if (strptime(s, "%Y-%m-%d" SEPARATOR, &ans->e_time) == NULL)
    {
        if (e == NULL)
        {
            free(ans);
        }

        return NULL;
    }
    /* Ora prova a leggere il tipo */
    if (sscanf(&s[DATE_LENGHT+1], "%c" SEPARATOR, &type) == EOF)
    {
        if (e == NULL)
        {
            free(ans);
        }

        return NULL;
    }
    /* Case-insensitive */
    switch (toupper(type))
    {
    case 'N':
        /* code */
        ans->type = NEW_CASE;
        break;

    case 'T':
        /* code */
        ans->type = SWAB;
        break;

    default:
        if (e == NULL)
        {
            free(ans);
        }

        return NULL;
    }

    /* Ora manca solo il numero finale */
    if (sscanf(&s[DATE_LENGHT+1+TYPE_LENGHT+1], "%d", &ans->counter) == EOF)
    {
        if (e == NULL)
        {
            free(ans);
        }

        return NULL;
    }

    return ans;
}

void register_free_entry(struct entry* E)
{
    free(E);
}


