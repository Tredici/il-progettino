#define _XOPEN_SOURCE 500  /* per strptime */
#define _POSIX_C_SOURCE /* per localtime_r */

#include "register.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "list.h"   /* per usare delle list per gestire i registri */

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

struct e_register
{
    /* memorizza data di crezione
     * di un register */
    struct tm e_time;
    struct list* l;
};

struct entry*
register_new_entry(struct entry* E,
                enum entry_type type,
                int counter)
{
    struct entry* ans;
    time_t today;

    if (counter <= 0)
        return NULL;

    switch (type)
    {
    case SWAB:
    case NEW_CASE:
        break;

    default:
        return NULL;
    }

    if (E == NULL)
    {
        ans = malloc(sizeof(struct entry));
        if (ans == NULL)
            return NULL;
    }
    else
    {
        ans = E;
    }

    today = time(NULL);
    if (localtime_r(&today, &ans->e_time) == NULL)
        return NULL;

    ans->type = type;
    ans->counter = counter;

    return ans;
}

struct entry* register_clone_entry(const struct entry* E, struct entry* E2)
{
    struct entry* ans;

    if (E == NULL)
        return NULL;

    if (E2 == NULL)
    {
        ans = malloc(sizeof(struct entry));
        if (ans == NULL)
            return NULL;
    }
    else
    {
        ans = E2;
    }
    memcpy(ans, E, sizeof(struct entry));

    return ans;
}

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

/** Funzione ausiliaria per aggirare
 * il problema del cast trai tipi
 * puntatore a funzione
 */
static void rfe_AUX(void* E)
{
    register_free_entry((struct entry*)E);
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

struct e_register* register_create(struct e_register* r, int flag)
{
    struct e_register* ans;
    time_t t;

    if (r == NULL)
    {
        ans = malloc(sizeof(struct e_register));
        if (ans == NULL)
            return NULL;
    }
    else
    {
        ans = r;
    }
    memset(ans, 0, sizeof(struct e_register));
    t = time(NULL);
    if (localtime_r(&t, &ans->e_time) == NULL)
    {
        if (r == NULL)
            free(ans);

        return NULL;
    }
    ans->l = list_init(NULL);
    if (ans->l == NULL)
    {
        if (r == NULL)
            free(ans);

        return NULL;
    }
    list_set_cleanup(ans->l, &rfe_AUX);

    return ans;
}

void register_destroy(struct e_register* r)
{
    if (r == NULL)
        return;

    list_destroy(r->l);
    r->l = NULL;
    free(r);
}

int register_add_entry(struct e_register* R, const struct entry* E)
{
    struct entry* E2;

    if (R == NULL || E == NULL)
        return -1;

    E2 = register_clone_entry(E, NULL);
    if (E2 == NULL)
        return -1;

    if (list_prepend(R->l, E2) != 0)
        return -1;

    return 0;
}

static void sumType(void* elem, void* tot)
{
    struct entry* E;

    if (elem == NULL)
        return;

    E = (struct entry*)elem;
    if ((int)E->type == ((int*)tot)[1])
        *(int*)tot += E->counter;
}

int register_calc_type(const struct e_register* R, enum entry_type type)
{
    int ans[2];

    if (R == NULL)
        return -1;

    switch (type)
    {
    case SWAB:
    case NEW_CASE:
        ans[1] = type;
        break;
    default:
        return -1;
    }

    ans[0] = 0;
    list_accumulate(R->l, &sumType, &ans);

    return ans[0];
}
