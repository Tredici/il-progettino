#define _XOPEN_SOURCE 500  /* per strptime */
#define _POSIX_C_SOURCE 1 /* per localtime_r */

#include "register.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <ctype.h>
#include "list.h"   /* per usare delle list per gestire i registri */
#include "set.h"    /* per tenere facilmente traccia delle firme delle entry presenti */
#include "time_utils.h" /* per facilitare il lavoro con le date */
#include "commons.h" /* per gli errori irrecuperabili */

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
    /* intero che funge da "firma" del
     * peer che creò questa entry, se 0
     * significa che è stato creato dal
     * peer corrente, è necessariamente
     * un valore non negativo */
    int signature;
};

struct e_register
{
    /* memorizza data di crezione
     * di un register */
    struct tm e_time;
    /** indica il giorno cui
     * corrisponde il register,
     * è la data corrente per i
     * register creati entro le
     * 18 e la data del giorno
     * successivo per quelli
     * creati tra le 18:00 e
     * mezzanotte
     */
    struct tm date;
    struct list* l;
    /* indica se il contenuto del
     * register è stato modificato
     * da quando è stato inizializzato,
     * ovvero se al termine del programma
     * il suo contenuto va riportato su file
     * (non 0) oppure non è necessario (vale 0).
     * Di default  */
    int modified;
    /** flag che indica se un registro è stato
     * "chiuso".
     * Un registro si definisce chiuso una volta
     * che tutte le sue entry che dovrebbe contenere
     * sono state recuperate e sono stati calcolati
     * i valori aggregati di interesse.
     */
    int closed;
    /** Firma da considerare come di default per
     * le nuove entry che non ne posseggono una
     * valida.
     */
    int defaultSignature;
    /** Insieme di tutte le firme delle entry
     * presenti nel register.
     */
    struct set* allSignature;
};

struct entry*
register_new_entry(struct entry* E,
                enum entry_type type,
                int counter,
                int signature)
{
    struct entry* ans;
    time_t today;

    if (counter <= 0 || signature < 0)
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
    ans->signature = signature;

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

int register_retrieve_entry_signature(const struct entry* E)
{
    if (E == NULL || E->signature < 0)
        return -1;

    return E->signature;
}

struct entry* register_parse_entry(const char* s, struct entry* e, enum ENTRY_SERIALIZE_RULE flag)
{
    int lastPos; /* per trovare la firma */
    struct entry* ans;
    char type;

    /* controllo del flag */
    switch (flag)
    {
    case ENTRY_SIGNATURE_OMITTED:
    case ENTRY_SIGNATURE_REQUIRED:
    case ENTRY_SIGNATURE_OPTIONAL:
        break;
    default:    /* parametro invalido */
        return NULL;
    }

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
     * \d{4}-\d{2}-\d{2}|[NT]|\d+(\[\d+\])?
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

    /* bisogna omettere la firma? */
    if (flag != ENTRY_SIGNATURE_OMITTED)
    {
        /* ENTRY_SIGNATURE_REQUIRED | ENTRY_SIGNATURE_OPTIONAL */

        /* scorre per trovare la firma */
        for (lastPos = DATE_LENGHT+1+TYPE_LENGHT+1; isdigit(s[lastPos]); ++lastPos)
            ;

        /* o la va o errore */
        if (s[lastPos] == '[')
        {
            lastPos = lastPos+1;
            /* la firma è azzerata all'inizio da memset */
            if (sscanf(&s[lastPos], "%d", &ans->signature) == EOF)
            {
                if (e == NULL)
                {
                    free(ans);
                }

                return NULL;
            }
            /* cerca se alla fine c'è ciò che vogliamo */
            for (; isdigit(s[lastPos]); ++lastPos)
                ;
            /* check finale */
            if (s[lastPos] != ']')
            {
                if (e == NULL)
                {
                    free(ans);
                }

                return NULL;
            }
        }
        else if (flag == ENTRY_SIGNATURE_REQUIRED)
        {
            /* se la firma era richiesta si ha un errore */
            if (e == NULL)
            {
                free(ans);
            }

            return NULL;
        }
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


char* register_serialize_entry(const struct entry* E, char* buf, size_t len, enum ENTRY_SERIALIZE_RULE flag)
{
    char str[ENTRY_TEXT_MAXLEN] = {};
    char* ans;
    int strLen, appoggio;
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
        return NULL;
    }

    /* controllo del flag */
    switch (flag)
    {
    case ENTRY_SIGNATURE_OMITTED:
        break;
    case ENTRY_SIGNATURE_OPTIONAL:
        /* vede se saltare */
        if (E->signature == 0)
            break;
    case ENTRY_SIGNATURE_REQUIRED:
        appoggio =  DATE_LENGHT+2+TYPE_LENGHT+strLen;
        strLen = sprintf(&str[appoggio], "[%d]", E->signature);
        if (strLen == EOF)
            return NULL;
        break;
    default:    /* parametro invalido */
        return NULL;
    }

    if (buf != NULL)
    {
        if ((int)len < (int)strlen(str)+1)
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

struct e_register* register_create(struct e_register* r, int defaultSignature)
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
    ans->defaultSignature = defaultSignature;
    t = time(NULL);
    if (localtime_r(&t, &ans->e_time) == NULL)
    {
        if (r == NULL)
            free(ans);

        return NULL;
    }
    /* dopo aver impostato la data di creazione
     * deve assegnare anche la data cui il
     * registro è associato */
    if (ans->e_time.tm_hour < 18) /* creato prima delle 18:00 */
    {
        /* anno,mese,giorno di creazione */
        ans->date.tm_mday = ans->e_time.tm_mday;
        ans->date.tm_mon = ans->e_time.tm_mon;
        ans->date.tm_year = ans->e_time.tm_year;
    }
    else /* creato dopo le 18:00 */
    {
        /* il giorno è domani */
        ans->date.tm_mday = ans->e_time.tm_mday+1;
        ans->date.tm_mon = ans->e_time.tm_mon;
        ans->date.tm_year = ans->e_time.tm_year;
        /* mktime normalizza le date se necessario */
        (void)mktime(&ans->date);
    }
    /* inizializza l'insieme di firme */
    ans->allSignature = set_init(NULL);
    if (ans->allSignature == NULL)
    {
        if (r == NULL)
            free(ans);

        return NULL;
    }
    /* un elemento già ci deve finire */
    if (defaultSignature)
        set_add(ans->allSignature, (long)defaultSignature);

    /* inizializza la lista di entri */
    ans->l = list_init(NULL);
    if (ans->l == NULL)
    {
        set_destroy(ans->allSignature);
        if (r == NULL)
            free(ans);

        return NULL;
    }
    list_set_cleanup(ans->l, &rfe_AUX);

    return ans;
}

struct e_register* register_create_date(
            struct e_register* r,
            int defaultSignature,
            const struct tm* date)
{
    struct e_register* ans;

    ans = register_create(r, defaultSignature);
    if (ans == NULL)
        return NULL;

    /* imposta la data se specificata */
    if (date != NULL)
        ans->date = *date;

    return ans;
}

ssize_t register_size(const struct e_register* R)
{
    if (R == NULL)
        return -1;

    return list_size(R->l);
}

const struct tm* register_date(const struct e_register* r)
{
    return r == NULL ? NULL : &r->date;
}

void register_destroy(struct e_register* r)
{
    if (r == NULL)
        return;

    list_destroy(r->l);
    r->l = NULL;
    free(r);
}

int register_is_changed(const struct e_register* r)
{
    return (r != NULL && r->modified != 0);
}

int register_clear_dirty_flag(struct e_register* r)
{
    if (r == NULL)
        return -1;

    r->modified = 0;
    return 0;
}

int register_add_entry(struct e_register* R, const struct entry* E)
{
    struct entry* E2;

    if (R == NULL || E == NULL)
        return -1;

    E2 = register_clone_entry(E, NULL);
    if (E2 == NULL)
        return -1;

    /* imposta la firma se necessario */
    if (E2->signature == 0 && R->defaultSignature != 0)
        E2->signature = R->defaultSignature;

    if (list_prepend(R->l, E2) != 0)
    {
        free((void*)E2);
        return -1;
    }

    /* bisogna inserire un nuova entry? */
    if (E2->signature != 0)
        set_add(R->allSignature, (long)E2->signature);

    /* segna l'aggiunta */
    R->modified = 1;

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

/** Struttura dati ausiliaria per passare i
 * dati in maniera sicura alla callback
 */
struct RS_data
{
    FILE* fp; /* puntatore per l'output */
    int defaultSignature; /* signature di default */
    enum ENTRY_SERIALIZE_RULE rule; /* politica da seguire */
};
/** Funzione ausiliaria per serializzare ogni
 * elemento.
 */
void register_serialize_HELPER(void* elem, void* data)
{
    struct entry* E;
    struct RS_data* D;
    char buffer[32];

    /* se va male qui è un disastro */
    if (elem == NULL || data == NULL)
        errExit("*** fatal [register_serialize] ***\n");

    /* cast di qualità */
    E = (struct entry*)elem;
    D = (struct RS_data*)data;

    /* se è richiesta una firma di default */
    if (D->rule == ENTRY_SIGNATURE_REQUIRED)
    {
        if (D->defaultSignature == 0 && E->signature == 0)
        {
            /* INCONSISTENZA! */
            errExit("*** fatal [signature] ***\n");
        }
        else if (E->signature == 0) /* imposta una firma di default */
            E->signature = D->defaultSignature;
    }
    /* gestisce l'omissione - ENTRY_SIGNATURE_OPTIONAL */
    if (D->rule == ENTRY_SIGNATURE_OPTIONAL && D->defaultSignature == E->signature)
        E->signature = 0;
    /* stringhizza */
    if (register_serialize_entry(E, buffer, sizeof(buffer), D->rule) == NULL)
        errExit("*** fatal [signature] ***\n");
    /* ripristina lo status quo */
    if (D->rule == ENTRY_SIGNATURE_OPTIONAL && 0 == E->signature)
        E->signature = D->defaultSignature;

    /* ora mette il tutto in output */
    fprintf(D->fp, "%s\n", buffer);
}

int register_serialize(
            FILE* fp,
            const struct e_register* R,
            enum ENTRY_SERIALIZE_RULE flag)
{
    struct RS_data base;

    if (fp == NULL || R == NULL)
        return -1;
    /* controllo del flag */
    switch (flag)
    {
    case ENTRY_SIGNATURE_REQUIRED:
        /* controllo: signature di default */
        if (R->defaultSignature == 0)
            return -1;
        break;
    case ENTRY_SIGNATURE_OMITTED:
    case ENTRY_SIGNATURE_OPTIONAL:
        break;
    default:
        return -1;
    }

    /* inizializza la base */
    base.fp = fp; /* output */
    base.defaultSignature = R->defaultSignature; /* firma di default */
    base.rule = flag; /* politica sulla firma */
    /* iteratori, iteratori ovunque */
    list_accumulate(R->l, &register_serialize_HELPER, (void*)&base);

    /* forza l'output */
    fflush(fp);

    return 0;
}

int register_serialize_fd(
            int fd,
            const struct e_register* R,
            enum ENTRY_SERIALIZE_RULE flag)
{
    int fdCp; /* copia */
    FILE* fp; /* puntatore alla copia */
    int ans;

    /* controlla solo il primo parametro,
     * rimanda il controllo degli altri */
    if (fd < 0)
        return -1;

    fdCp = dup(fd);
    if (fdCp == -1)
        return -1;

    /* apre in scrittura in modalità append */
    fp = fdopen(fdCp, "a");
    if (fp == NULL)
        return -1;

    /* invoca la funzione ausiliaria */
    ans = register_serialize(fp, R, flag);

    /* chiude il puntatore e il f.d. sotto (fdCp) */
    if (fclose(fp) != 0)
        return -1;

    return ans;
}

int register_flush(struct e_register* R, int force)
{
    int signature, year, month, day;
    char filename[32];
    FILE* fout;
    int ans;

    if (R == NULL || R->defaultSignature == 0)
        return -1;

    /* funzionamento normale */
    if (!force)
    {
        /* controlla che non sia vuoto */
        if (register_size(R) == 0)
            return 0;
        /* controlla il dirty flag */
        if (!register_is_changed(R))
            return 0;
    }

    signature = R->defaultSignature;
    if (time_date_read(&R->date, &year, &month, &day) == -1)
        return -1;

    /* genera il nome del file */
    if (sprintf(filename, "%d.%d-%d-%d.txt",
            signature, year, month, day) < 0)
        return -1;

    /* cerca di aprire il file */
    fout = fopen(filename, "w");

    /* si appoggia a un'altra funzione */
    ans = register_serialize(fout, R, ENTRY_SIGNATURE_OPTIONAL);

    /* lo chiude */
    if (fclose(fout) != 0)
        return -1;

    /* azzera il flag */
    register_clear_dirty_flag(R);

    return ans == 0 ? 1 : ans;
}

char* register_filename(
            const struct e_register* R,
            char* buffer,
            size_t bufLen)
{
    int signature, year, month, day;
    char filename[32];
    int len;
    char* ans;

    if (R == NULL || R->defaultSignature == 0)
        return NULL;

    signature = R->defaultSignature;
    time_date_read(&R->date, &year, &month, &day);

    len = sprintf(filename, "%d.%d-%d-%d.txt",
        signature, year, month, day);
    if (len == -1)
        return NULL;

    if (buffer != NULL)
    {
        if (bufLen < (size_t)(1+len))
        {
            /* non abbstanza spazio */
            return NULL;
        }
        ans = strcpy(buffer, filename);
    }
    else
    {
        ans = strdup(filename);
    }

    return ans;
}

/** Funzione ausiliaria che prova a prendere
 * una riga di input dal file descriptor fornito.
 * Serve come funzione ausiliaria per
 * register_parse e può lavorare in modo "normale"
 * (flag strict == 0) o strict (flag strict != 0).
 *
 * NOTA: "\w" va a includere anche altri segni grafici
 * come "[]-|" perché non mi ricordo altrimenti come
 * andrebbero indicati (forse come "\S"?).
 *
 * In modalità strict si aspetta un input di questo tipo:
 * \w+\n.
 * È un errore se non viene trovato il carattere di newline.
 * La sequenza di caratteri alfanumerici deve essere di al
 * più bufLen-1 caratteri, l'ultimo carattere è sempre posto
 * a '\0' per sicurezza.
 *
 * In modalità non strict possono essere presenti, e sono
 * scartati, un numero arbitrario di caratteri blank
 * (newline INCLUSO) prima e dopo l'inizio di ogni
 * sequenza \w+.
 *
 * Restituisce il numero di caratteri letti e messi dentro
 * buff se ha successo oppure -1 in caso di errore.
 * In caso di EOF precoce restituisce 0 se non sta lavorando
 * in "strict mode".
 */
static int getLimitedLine(FILE* fin, char* buff, size_t bufLen, int strict)
{
    int i, c, limit;

    /* check */
    if (fin == NULL || buff == NULL || bufLen < ENTRY_TEXT_MAXLEN)
        return -1;

    c = fgetc(fin);
    /* strict mode? */
    if (strict)
    {
        /* una entry inizia con un carattere alfanumerico */
        if (!isalnum(c))
            return -1;
    }
    else
    {
        if (c == EOF)
            return 0;
        /* scarta tutti i blank */
        if (!isalnum(c))
        {
            while (isspace(c))
                c = fgetc(fin);
            /* fine file? */
            if (c == EOF)
                return 0;
        }
    }
    /* ripristina lo status quo */
    ungetc(c, fin);
    /* prova a leggere caratteri e a metterli
     * nel buffer */
    limit = (int)bufLen-1; /* operazioni più efficienti */
    buff[limit] = '\0'; /* sicurezza */
    for (i = 0; i < limit; ++i)
    {
        /* prende un carattere, lo testa */
        c = fgetc(fin);
        /* nelle entry possono esserci anche caratteri
         * non alfanumerici ma "[]-|", testa quindi che
         * il carattere sia stampabile */
        if (!isgraph(c))
        {
            ungetc(c, fin);
            break;
        }
        /* controlla che sia un carattere ascii */
        if (!isascii(c))
        {
            /* check EOF */
            if (!strict && c == EOF)
            {
                buff[i] = '\0';
                return i;
            }
            return -1;
        }
        /* inserisce nel buffer */
        buff[i] = (char)c;
    }
    /* può ignorare altri blank? */
    if (!strict)
    {
        do
        {
            c = fgetc(fin);
        }
        while (isblank(c) && c != '\n');
    }
    else
        c = fgetc(fin);

    /* siamo alla fine? - controlla anche EOF */
    if (c != '\n' && !(!strict && c == EOF))
        return -1;

    buff[i] = '\0';
    return i;
}

struct e_register* register_parse(
            FILE* fp,
            enum ENTRY_SERIALIZE_RULE flag,
            int limit)
{
    char line[ENTRY_TEXT_MAXLEN]; /* per prendere una riga alla volta */
    struct e_register* R;
    int i; /* per stare entro il limite */
    struct entry E; /* si lavora per valore stavolta */

    /* controllo dei parametri */
    if (fp == NULL || limit < -1)
        return NULL;

    /* prova a creare il registro */
    R = register_create(NULL, 0);
    if (R == NULL)
        return NULL;
    if (limit == -1)
    {
        /* controlli blandi, è  */
        while (1)
        {
            switch (getLimitedLine(fp, line, sizeof(line), 0))
            {
            /* EOF */
            case 0:
                goto esci;
            /* fail */
            case -1:
                register_destroy(R);
                return NULL;
            /* qualcosa ha letto */
            default:
                /* tenta il parsing della entry */
                if (register_parse_entry(line, &E, flag) == NULL)
                {
                    register_destroy(R);
                    return NULL;
                }
                /* prova a inserirla nel registro */
                if (register_add_entry(R, &E) != 0)
                {
                    register_destroy(R);
                    return NULL;
                }
                break;
            }
        }
        esci:
            ;
    }
    else
    {
        /* controlli "seri": una entry per riga,
         * terminata da newline, no spazi bianchi */
        for (i = 0; i != limit; ++i)
        {
            switch (getLimitedLine(fp, line, sizeof(line), 1))
            {   /* in strict mode non restituisce mai 0 */
            /* fail */
            case -1:
                register_destroy(R);
                return NULL;
            /* qualcosa ha letto */
            default:
                /* tenta il parsing della entry - DEVE avere signature */
                if (register_parse_entry(line, &E, ENTRY_SIGNATURE_REQUIRED) == NULL)
                {
                    register_destroy(R);
                    return NULL;
                }
                /* prova a inserirla nel registro */
                if (register_add_entry(R, &E) != 0)
                {
                    register_destroy(R);
                    return NULL;
                }
                break;
            }
        }
    }

    return R;
}

struct e_register* register_parse_fd(
            int fd,
            enum ENTRY_SERIALIZE_RULE flag,
            int limit)
{
    struct e_register* ans;
    FILE* fin;

    /* demanda gli altri controlli alla */
    fin = fdopen(dup(fd), "r");
    if (fin == NULL)
        return NULL;

    ans = register_parse(fin, flag, limit);

    if (fclose(fin) != 0)
    {
        register_destroy(ans);
        return NULL;
    }

    return ans;
}

struct e_register* register_read(
            int defaultSignature,
            const struct tm* date,
            int strict)
{
    int year, month, day;
    char filename[32];
    FILE* fin;
    struct tm tmpDate;
    struct e_register* ans;

    if (defaultSignature <= 0)
        return NULL;

    if (date == NULL)
    {
        tmpDate = time_date_now();
        if (tmpDate.tm_hour >= 18)
            time_date_inc(&tmpDate, 1);
    }
    else
        time_copy_date(&tmpDate, date);
    /* estrae la data */
    if (time_date_read(&tmpDate, &year, &month, &day) == -1)
        return NULL;

    /* genera il nome del file da cercare */
    if (sprintf(filename, "%d.%d-%d-%d.txt",
        defaultSignature, year, month, day) < 0)
        return NULL;

    fin = fopen(filename, "r");
    if (fin == NULL)
    {
        /* non è riuscito ad aprire */
        if (strict) /* strict mode */
            return NULL;

        ans = register_create_date(NULL, defaultSignature, &tmpDate);
    }
    else
    {
        ans = register_parse(fin, ENTRY_SIGNATURE_OPTIONAL, -1);
        /* chiude il FILE* */
        if (fclose(fin) != 0)
        {
            register_destroy(ans);
            return NULL;
        }
        if (ans != NULL) /* imposta la firma di default */
        {
            ans->modified = 0; /* è pari al suo file */
            ans->defaultSignature = defaultSignature;
            ans->date = tmpDate;/* imposta la data */
        }
    }

    return ans;
}

/** Struttura dati ausiliaria per implementare
 * register_merge.
 */
struct mergeData
{
    /* registro destinazione */
    struct e_register* Rdst;
    const struct e_register* Rsrc;
    /*  */
    struct set* diff;
};

static void merge_helper(void* elem, void* base)
{
    struct entry* E;
    struct mergeData* D;
    int signature;

    D = (struct mergeData*)base;
    E = (struct entry*)elem;

    signature = E->signature == 0 ?
                        D->Rsrc->defaultSignature :
                        E->signature;
    /* bisogna aggiungerlo? */
    /* si assume che due registri con firma 0 siano
     * sempre distinti */
    if (set_has(D->diff, signature) || signature == 0)
    {
        register_add_entry(D->Rdst, E);
    }
}

int register_merge(struct e_register* R1, const struct e_register* R2)
{
    struct mergeData base; /* da passare alla funzione ausiliaria */
    struct set* news; /* insieme dei nuovi tipi */

    if (R1 == NULL || R2 == NULL)
        return -1;

    /* controllo sulla data */
    if (time_date_cmp(&R1->date, &R2->date) != 0)
        return -1;

    /** Fa la differenza tra le firme nota a un registro
     * e all'altro
     */
    news = set_diff(R2->allSignature, R1->allSignature);
    if (news == NULL)
        return -1;

    /* se è vuoto non fa nulla */
    if (set_size(news) == 0)
    {
        set_destroy(news);
        return 0; /* OK! */
    }
    /* altrimenti si lavora */
    base.diff = news;
    base.Rdst = R1;
    base.Rsrc = R1;
    list_accumulate(R2->l, &merge_helper, (void*)&base);
    set_destroy(news);

    return 0;
}

struct e_register* register_clone(const struct e_register* R)
{
    struct e_register* ans;

    if (R == NULL)
        return NULL;

    /* approccio semplice ma efficace */
    ans = register_create_date(NULL, 0, &R->date);
    if (ans == NULL)
        return NULL;

    if (register_merge(ans, R) == -1)
    {
        /* fail */
        register_destroy(ans);
        return NULL;
    }
    /* è una copia */
    ans->defaultSignature = R->defaultSignature;
    /* come nuovo */
    ans->modified = 0;
    /* o entrambi o nessuno */
    ans->closed = R->closed;

    return ans;
}

int register_close(struct e_register* R)
{
    if (R == NULL)
        return -1;

    R->closed = 1;
    return 0;
}

int register_is_closed(const struct e_register* R)
{
    if (R == NULL)
        return -1;

    return R->closed == 1;
}
