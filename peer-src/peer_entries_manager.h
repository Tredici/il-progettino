/** Funzioni che servono ad avviare e
 * gestire il sottosistema che gestisce
 * le entry e tutto il resto del lavoro
 * associato.
 */

#ifndef PEER_ENTRIES_MANAGER
#define PEER_ENTRIES_MANAGER

#include "peer_query.h"

/** Indica l'anno più remoto per cui il sistema
 * garantirà di tenere un registro per ogni suo
 * giorno.
 *
 * In pratica vi saranno registri per tutti i
 * giorni a partire dal primo gennaio dell'anno
 * indicato da questa macro fino alla data
 * corrente.
 */
#define INFERIOR_YEAR 2020

/** Fornisce la data del più vecchio
 * registro posseduto dal peer corrente.
 *
 * In caso di errore restituisce
 * una struttura completamente
 * azzerata.
 */
struct tm firstRegisterClosed(void);

/** Fornisce la data dell'ultimo
 * registro chiuso - ovvero non
 * più modificabile dal peer corrente.
 *
 * In caso di errore restituisce
 * una struttura completamente
 * azzerata.
 */
struct tm lastRegisterClosed(void);

/** Avvia il sottosistema che gestisce le
 * entry.
 *
 * Il parametro intero fornito indica la
 * porta su cui lavora il peer per
 * recuperare eventualmente i vecchi dati
 * da file.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int startEntriesSubsystem(int);

/** Termina il sottosistema a gestione delle
 * entry.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int closeEntriesSubsystem(void);

/** Aggiunge una copia dell'entry
 * fornita al registro con la data
 * corrente.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int addEntryToCurrent(const struct entry*);

/** Cerca nella cache la risposta alla query
 * fornita.
 *
 * ATTENZIONE: non bisogna mai invocare free
 * o qualsiasi altra funzione con side effect
 * sul puntatore eventualmente restituito.
 *
 * Restituisce il puntatore all'oggetto
 * struct answer in caso di successo e
 * NULL in caso di fallimento.
 */
const struct answer* findCachedAnswer(const struct query*);

/** Aggiunge il risultato di una query alla
 * cache per poterlo recuperare più avanti
 * mediante findCachedAnswer.
 *
 * ATTENZIONE: non bisogna mai invocare free
 * o qualsiasi altra funzione con side effect,
 * in caso di successo, sul puntatore all'oggetto
 * struct answer fornito.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int addAnswerToCache(const struct query*, const struct answer*);

#endif
