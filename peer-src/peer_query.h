/** Raccolta di funzioni e strutture per contenere
 * i risultati delle query.
 */

#ifndef PEER_QUERY
#define PEER_QUERY

#include "../register.h"
#include "../list.h"
#include "../time_utils.h"

/** Enumerazione che elenca tutti i tipi delle
 * aggregazioni il cui calcolo può essere
 * richiesto dall'utente.
 */
enum aggregation_type
{
    AGGREGATION_SUM,
    AGGREGATION_DIFF
};

/** Struttura che permette di descrivere
 * interamente una query.
 *
 * Non dovrebbe mai essere manipolata
 * direttamente ma solo tramite le
 * apposite funzioni fornite.
 */
struct query;

/** Inizializza l'oggetto puntato da query
 * con i valori forniti.
 *
 * ATTENZIONE: non garantisce alcun controllo
 * sull'integrità dei dati forniti.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int buildQuery(struct query* query,
            enum aggregation_type type,
            enum entry_type category,
            const struct tm* begin,
            const struct tm* end);


#endif
