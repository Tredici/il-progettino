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

/** Struttura in grado di rappresentare
 * il contenuto di un oggetto di tipo
 * struct query in maniera network safe.
 */
struct ns_query
{
    struct ns_tm begin, end;
    uint16_t type, category;
};

/** Oggetto che rappresenta una risposta
 * a una data query.
 */
struct answer;

/** Oggetto che permette di memorizzare una
 * query in maniera network safe.
 */
struct ns_answer
{
    struct ns_query query;
    uint32_t lenght;
    uint32_t data[0];
};

/** Inizializza un oggetto di tipo struct ns_query
 * con il contenuto di un oggetto struct query.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int initNsQuery(struct ns_query*, const struct query*);

/** Trasferisce il contenuto di un oggetto di tipo
 * struct ns_query in un oggetto struct query.
 *
 * Svolge un ruolo complementare a quello di
 * initNsQuery.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int readNsQuery(struct query*, const struct ns_query*);

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

/** Calcola un hash di una query e lo restituisce.
 * Permette di identificare in maniera univoca
 * tutte le query le cui date
 *
 * Restituisce 0 in caso di errore.
 */
long int hashQuery(const struct query* query);

#endif
