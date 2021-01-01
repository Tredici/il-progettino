/** Funzioni ausiliarie per gestire i registri
 * nella prima versione sarà gestito tutto "in memory"
 */


#ifndef REGISTER
#define REGISTER

#include <stdlib.h>

/** Spazio massimo necessario per
 * serializzare un'entry
 */
#define ENTRY_TEXT_MAXLEN 22

enum entry_type
{
    SWAB,       /* tampone */
    NEW_CASE    /* nuovo caso */
};

/** Un register è una raccolta di entry
 * e permette di eseguire una serie di
 * operazioni su queste
 */
struct e_register;

struct entry;

/** La struttura testuale di un entry
 * sarà:
 *  \d{4}-\d{2}-\d{2}|[NT]|\d+
 *
 * Una pipe '|' farà da separatore
 * e i tre campi saranno rispettivamente:
 *  +data
 *  +tipologia
 *  +numero di persone coinvolte
 *
 */


/** Crea un entry associata al giorno corrente
 * del tipo e con la quantità specificata.
 */
struct entry* register_new_entry(struct entry*, enum entry_type, int);

/** Copia una entry in un'altra, eventualmente allocando lo spazio
 * necessario in memoria dinamica.
 *
 * Restituisce NULL in caso di errore
 */
struct entry* register_clone_entry(const struct entry*, struct entry*);

/** Esegue il parsing di una entry convertendola
 *  da formato testuale a struttura di riferimento
 *
 * Se il secondo parametro è NULL alloca la entry
 * in memoria dinamica
 *
 * In caso di successo ritorna un puntatore alla
 * entry data (che può coincidere con l'argomento),
 * altrimenti NULL
 */
struct entry* register_parse_entry(const char*, struct entry*);

/** Distrugge correttamente un'entry allocata
 * in memoria dinamica, ad esempio tramine
 * register_parse_entry
 */
void register_free_entry(struct entry*);

/** Svolge un lavoro complementare rispetto a
 * register_parse_entry
 *
 * Se il secondo argomento è NULL il terzo viene
 * ignorato e lo spazio necessario viene riservato
 * in memoria dinamica e può essere tranquillamente
 * riservato in memoria dinamica
 *
 * Se il vettore allocato ha una dimensione di
 * almeno
 */
char* register_serialize_entry(const struct entry*, char*, size_t);

/** Crea un nuovo registro in memoria, eventualmente
 * allocando lo spazio necessario in memoria dinamica
 */
struct e_register* register_create(struct e_register*, int);

/** Libera tutta la memoria associata a un registro
 * senza preoccuparsi di eseguire alcuna operazione
 * per garantire la persistenza dei dati o altro.
 */
void register_destroy(struct e_register*);

#endif
