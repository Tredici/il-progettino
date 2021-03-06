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
#include "../commons.h"
#include "peer_udp.h"

static int
parsePeriod(const char* args,
            struct tm* timeStart,
            struct tm* timeEnd)
{
    char buffer[32] = "";
    char* end;
    struct tm a, b;
    struct tm lowerDate = firstRegisterClosed();
    struct tm upperDate = lastRegisterClosed();

    strncpy(buffer, args, sizeof(buffer)-1);
    end = strchr(buffer, '-');
    if (end == NULL)
        return -1;
    /* "spezza" la stringa in due */
    *end = '\0';

    /* La data iniziale è "*"? */
    if (buffer[0] == '*' && buffer[1] == '\0')
        a = lowerDate;
    else if (time_parse_date(buffer, &a) == -1)
        return -1;
    else if (time_date_cmp(&a, &lowerDate) < 0) /* la data è troppo vecchia */
        return -1;
    /* la prima data è segnata */

    ++end; /* punta alla seconda data */
    if (end[0] == '*' && end[1] == '\0')
        b = upperDate;
    else if (time_parse_date(end, &b) == -1)
        return -1;
    else if (time_date_cmp(&b, &upperDate) > 0) /* la data è troppo vecchia */
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
    int ret;
    enum aggregation_type aggregation;
    enum entry_type query_type;
    struct tm date_min, date_max;
    char date1[16], date2[16];
    struct query query;
    struct answer* ans;
    char strQuery[128];
    enum unified_io_mode saved_mode;

    /* controlla che sia connesso al network */
    if (!UDPisConnected())
    {
        printError("ERRORE: il peer non è connesso al network!\n");
        return WRN_CONTINUE;
    }

    ret = sscanf(args, "%15s %15s %31s", aggr, type, period);
    if (ret < 2)
    {
        printError("Parametri invalidi!\n");
        return ERR_PARAMS;
    }
    /* controllo sul parametro aggregazione */
    if (strcasecmp(aggr, "totale") == 0)
        aggregation = AGGREGATION_SUM;
    else if (strcasecmp(aggr, "variazione") == 0)
        aggregation = AGGREGATION_DIFF;
    else
    {
        printError("Le aggregazioni possono "
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
        printError("I tipi possono essere solo {"
            ADD_SWAB "|" ADD_NEW_CASE "}!\n");
        return ERR_PARAMS;
    }

    /* Controllo sull'intervallo delle date */
    if (ret >= 3)
    {
        if (parsePeriod(period, &date_min, &date_max) != 0)
        {
            printError("Errore nel parsing delle date!\n");
            return ERR_PARAMS;
        }
    }
    else
    {
        date_min = firstRegisterClosed();
        date_max = lastRegisterClosed();
    }

    if (time_date_cmp(&date_min, &date_max) > 0)
    {
        printError("La seconda data precede la prima!\n");
        return ERR_PARAMS;
    }

    printf("Ricerca nell'intervallo [%s|%s]\n", time_serialize_date(date1, &date_min),
        time_serialize_date(date2, &date_max));

    if (buildQuery(&query, aggregation, query_type, &date_min, &date_max) != 0)
    {
        printError("Qualcosa non va nella query!\n");
        return ERR_FAIL;
    }

    printf("Calculating: %s\n", stringifyQuery(&query, strQuery, sizeof(strQuery)));
    /* cambia la modalità di funzionamento del sottosistema di IO */
    saved_mode = unified_io_set_mode(UNIFIED_IO_SYNC_MODE);
    ans = calcEntryQuery(&query);
    /* la ripristina */
    unified_io_set_mode(saved_mode);
    if (ans == NULL)
    {
        printError("Impossibile rispondere alla query!\n");
        return ERR_FAIL;
    }
    printf("Result:\n");
    if (printAnswer(ans) != 0)
        errExit("*** FAIL:printAnswer ***\n");

    return OK_CONTINUE;
}
