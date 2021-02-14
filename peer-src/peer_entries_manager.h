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

/** Cerca nella lista di registri controllati
 * dal sottosistema quello corrispondente al
 * registro fornito e vi carica le entry presenti
 * nell'argomento.
 */
int mergeRegisterContent(const struct e_register*);

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

/** Provvede a eseguire il calcolo di una
 * query su un dato intervallo.
 *
 * Restituisce un puntatore a un oggetto
 * rappresentante il risultato della query
 * oppure NULL in caso di errore.
 */
struct answer* calcEntryQuery(const struct query*);

/** Cerca tra i registri posseduti dal peer
 * quello con la data fornita e ne fornisce
 * una rappresentazione in formato network
 * safe pronta ad essere inviata via rete.
 *
 * Il tutto viene eseguito in maniera atomica
 * e si fa ricorso a register_as_ns_array.
 *
 * L'insieme fornito permette di specificare
 * tutte le signature che non è necessario
 * includere nel buffer che sarà generato.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int getNsRegisterData(const struct tm* date,
            struct ns_entry** buffer, size_t* bufLen,
            const struct set* skip);

/** Cerca il registro corrispondente alla data
 * fornita e restituisce un array di uint32_t
 * contenente tutte le firme possedute dal
 * registro trovato in HOST ORDER,
 * pronto ad essere inviato in un messaggio
 * di tipo MESSAGES_FLOOD_FOR_ENTRIES.
 *
 * Internamente fa uso di register_owned_signatures.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int getRegisterSignatures(const struct tm* date,
            uint32_t** buffer, size_t* bufLen);

#endif
