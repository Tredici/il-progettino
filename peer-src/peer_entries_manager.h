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
