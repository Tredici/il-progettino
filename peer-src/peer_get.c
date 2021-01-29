#include "peer_get.h"
#include "../repl.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "peer_add.h"
#include "../register.h"
#include <ctype.h>
#include "peer_entries_manager.h"
#include "../time_utils.h"
#include "../unified_io.h"

static int
parsePeriod(const char* args,
            struct tm* timeStart,
            struct tm* timeEnd)
{
    char buffer[32] = "";
    char* end;
    struct tm a, b;

    strncpy(buffer, args, sizeof(buffer)-1);
    end = strchr(buffer, '-');
    if (end == NULL)
        return -1;
    /* "spezza" la stringa in due */
    *end = '\0';

    /* La data iniziale è "*"? */
    if (buffer[0] == '*' && buffer[1] == '\0')
        memset(&a, 0, sizeof(struct tm));
    else if (time_parse_date(buffer, &a) == -1)
        return -1;
    /* la prima data è segnata */

    ++end; /* punta alla seconda data */
    if (end[0] == '*' && end[1] == '\0')
        memset(&b, 0, sizeof(struct tm));
    else if (time_parse_date(end, &b) == -1)
        return -1;
    /* la seconda data è segnata */

    *timeStart = a;
    *timeEnd = b;

    return 0;
}

int get(const char* args)
{
    char aggr[16] = "";
    char type[16] = "";
    char period[32] = "";
    struct tm timeStart, timeEnd;
    int ret;
    enum aggregation_type query;
    enum entry_type query_type;
    struct tm date_min, date_max;
    char date1[16], date2[16];

    ret = sscanf(args, "%15s %15s %31s", aggr, type, period);
    if (ret < 2)
    {
        fprintf(stderr, "Parametri invalidi!\n");
        return ERR_PARAMS;
    }
    /* controllo sul parametro aggregazione */
    if (strcasecmp(aggr, "totale") == 0)
        query = AGGREGATION_SUM;
    else if (strcasecmp(aggr, "variazione") == 0)
        query = AGGREGATION_DIFF;
    else
    {
        fprintf(stderr, "Le aggregazioni possono "
            "essere solo {totale|variazione}!\n");
        return ERR_PARAMS;
    }
    /* controllo sul type */
    if (strcasecmp(type, ADD_SWAB) == 0)
        query_type = SWAB;
    else if (strcasecmp(type, ADD_NEW_CASE) == 0)
        query_type = NEW_CASE;
    else
    {
        fprintf(stderr, "I tipi possono "
            "essere solo {" ADD_SWAB "|" ADD_NEW_CASE "}!\n");
        return ERR_PARAMS;
    }

    /* Controllo sull'intervallo delle date */
    if (ret >= 3)
    {
        if (parsePeriod(period, &date_min, &date_max) != 0)
            return -1;
    }
    else
    {
        date_min = firstRegisterClosed();
        date_max = lastRegisterClosed();
    }

    printf("Ricerca nell'intervallo [%s|%s]\n", time_serialize_date(date1, &date_min),
        time_serialize_date(date2, &date_max));

    unified_io_push(UNIFIED_IO_ERROR, "UNIMPLEMENTED!");

    return OK_CONTINUE;
}
