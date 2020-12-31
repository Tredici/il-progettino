#define _XOPEN_SOURCE 500  /* per strptime */
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
/* carattere tampone */
#define C_SWAB 'T'
/* carattere nuovo caso */
#define C_NEW_CASE 'N'

/** Una entry necessiterà di:
 *  tempo:
 *      +data
 *      ? +ora
 *  tipo:
 *      >tampone
 *
 */


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
    case C_NEW_CASE:
        /* code */
        ans->type = NEW_CASE;
        break;

    case C_SWAB:
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


char* register_serialize_entry(const struct entry* E, char* buf, size_t len)
{
    char str[ENTRY_TEXT_MAXLEN] = {};
    char* ans;
    int strLen;
    char symbol;

    if (E == NULL)
    {
        return NULL;
    }
    /* Controllo integrità tipo */
    switch (E->type)
    {
    case SWAB:
        symbol = C_SWAB;
        break;

    case NEW_CASE:
        symbol = C_NEW_CASE;
        break;

    default:
        /* struttura malformata */
        return NULL;
    }
    /* Controllo validità counter */
    if (E->counter <= 0)
    {
        return NULL;
    }

    strLen = strftime(str, ENTRY_TEXT_MAXLEN, "%Y-%m-%d", (const struct tm*)&E->e_time);
    if (strLen != DATE_LENGHT)
    {
        return NULL;
    }

    strLen = sprintf(&str[DATE_LENGHT], SEPARATOR "%c" SEPARATOR, symbol);
    if (strLen != 2 + TYPE_LENGHT)
    {
        return NULL;
    }

    strLen = sprintf(&str[DATE_LENGHT+2+TYPE_LENGHT], "%d", E->counter);
    if (strLen < 0)
    {
        return 0;
    }

    if (buf != NULL)
    {
        if ((int)len < DATE_LENGHT+2+TYPE_LENGHT+strLen+1)
        {
            /** Se non basta lo spazio per contenere
             * il risultato genera un errore
             */
            return NULL;
        }
        ans = strcpy(buf, str);
    }
    else
    {
        ans = strdup(str);
    }

    return ans;
}
