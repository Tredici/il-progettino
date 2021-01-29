/** Funzioni che servono ad avviare e
 * gestire il sottosistema che gestisce
 * le entry e tutto il resto del lavoro
 * associato.
 */

#ifndef PEER_ENTRIES_MANAGER
#define PEER_ENTRIES_MANAGER

#include "../register.h"

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

/** Enumerazione che elenca tutti i tipi delle
 * aggregazioni il cui calcolo può essere
 * richiesto dall'utente.
 */
enum aggregation_type
{
    AGGREGATION_SUM,
    AGGREGATION_DIFF
};

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

#endif
