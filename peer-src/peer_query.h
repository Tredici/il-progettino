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
struct query
{
    /* tipo della query */
    enum aggregation_type aggregation;
    /* "categoria" degli oggetti
     * da considerare */
    enum entry_type category;
    /* date di inizio e di fine
     * del periodo di interesse */
    struct tm begin, end;
};

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

/** Controlla l'integrità di una query.
 *
 * Restituisce 0 in caso di successo (la
 * query è valida) oppure -1 altrimenti.
 */
int checkQuery(const struct query*);

/** Fornisce una stringa rappresentante il
 * contenuto di una query.
 *
 * Se il puntatore al buffer è NULL alloca
 * lo spazio richiesto in memoria dinamica
 * usando strdup.
 *
 * Restituisce il puntatore al buffer
 * in caso di successo e NULL in caso di
 * errore.
 */
char* stringifyQuery(const struct query*, char*, size_t);

/** Calcola la risposta alla query fornita
 * usando i dati presenti della lista fornita,
 * la quale risponde a questo "prototipo":
 *      list<register>
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 */
int calcAnswer(struct answer**, const struct query*, const struct list*);

/** Libera correttamente lo spazio allocato da
 * un oggetto di tipo struct answer.
 */
void freeAnswer(struct answer*);

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

/** Inizializza e fornisce un oggetto di tipo
 * struct ns_answer a partire da un oggetto di
 * tipo struct answer.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int initNsAnswer(struct ns_answer**, size_t*, const struct answer*);

/** Inizializza un oggetto di tipo struct answer
 * con il contenuto di un oggetto struct ns_answer.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int readNsAnswer(struct answer**, const struct ns_answer*, size_t);

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

/** Estrae e fornisce i dati richiesti
 * da un oggetto di tipo struct query.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int readQuery(const struct query* query,
            enum aggregation_type* type,
            enum entry_type* category,
            struct tm* begin,
            struct tm* end);

/** Calcola un hash di una query e lo restituisce.
 * Permette di identificare in maniera univoca
 * tutte le query le cui date
 *
 * Restituisce 0 in caso di errore.
 */
long int hashQuery(const struct query* query);

/** Stampa su stdout il contenuto di una
 * risposta a una query, ovvero il contenuto
 * di un oggetto di tipo struct answer.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int printAnswer(const struct answer*);

#endif
