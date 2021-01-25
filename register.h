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
 *  \d{4}-\d{2}-\d{2}|[NT]|\d+(\[\d+\])?
 *
 * Una pipe '|' farà da separatore
 * e i tre campi saranno rispettivamente:
 *  +data
 *  +tipologia
 *  +numero di persone coinvolte
 * opzionale:
 *  +tra parentesi quadre un numero che
 *      identifica il peer che ha generato
 *      la entry
 */

/** Enumerazione che permette di specificare quali
 * criteri seguire nella serializzazione e nel parsing
 * delle entry per quanto riguarda la firma associata
 * a ciascuna entry.
 */
enum ENTRY_SERIALIZE_RULE
{
    ENTRY_SIGNATURE_OMITTED,
    ENTRY_SIGNATURE_REQUIRED,
    ENTRY_SIGNATURE_OPTIONAL
};

/** Crea un entry associata al giorno corrente
 * del tipo e con la quantità specificata.
 *
 * Il quarto parametro permette di specificare
 * la firma da associare al nuovo elemento, o
 * 0 se non se ne vuole specificare alcuna.
 */
struct entry* register_new_entry(struct entry*, enum entry_type, int, int);

/** Copia una entry in un'altra, eventualmente allocando lo spazio
 * necessario in memoria dinamica.
 *
 * Restituisce NULL in caso di errore
 */
struct entry* register_clone_entry(const struct entry*, struct entry*);

/** Fornisce l'identificativo (firma) del peer che ha prodotto
 * l'entry o 0 nel caso questo non sia stato specificato.
 *
 * Restituisce un valore non negtivo in caso di successo
 * e -1 in caso di errore.
 */
int register_retrieve_entry_signature(const struct entry*);

/** Esegue il parsing di una entry convertendola
 *  da formato testuale a struttura di riferimento
 *
 * Se il secondo parametro è NULL alloca la entry
 * in memoria dinamica.
 *
 * Il terzo parametro specifica la politica da
 * applicare nei confronti della signature presente
 * nella stringa da analizzare.
 *  +ENTRY_SIGNATURE_OMITTED  -> salta per intero il
 *                          controllo della signature
 *  +ENTRY_SIGNATURE_REQUIRED -> fallisce se non trova
 *                          la firma tra parentesi [].
 *  +ENTRY_SIGNATURE_OPTIONAL -> se la tova bene altrimenti
 *                          pazienza.
 *
 * In caso di successo ritorna un puntatore alla
 * entry data (che può coincidere con l'argomento),
 * altrimenti NULL
 */
struct entry* register_parse_entry(const char*, struct entry*, enum ENTRY_SERIALIZE_RULE);

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
 * L'ultimo argomento è un flag che se non nullo
 * permette di specificare se si vuole serializzare
 * anche la firma associata all'entry.
 * In particolare se:
 *  ENTRY_SIGNATURE_OMITTED  -> questa viene esclusa
 *  ENTRY_SIGNATURE_REQUIRED ->  questa viene inserita sempre
 *  ENTRY_SIGNATURE_OPTIONAL -> questa viene inserita solo se non 0
 *
 * Se il vettore allocato ha una dimensione di
 * almeno
 */
char* register_serialize_entry(const struct entry*, char*, size_t, enum ENTRY_SERIALIZE_RULE);

/** Crea un nuovo registro in memoria, eventualmente
 * allocando lo spazio necessario in memoria dinamica.
 *
 * Il secondo parametro permette di speficare la signature
 * da assegnare di default alle entry che non ne posseggono
 * una loro di default.
 * 0 se non si vuole associare nessuna firma di default.
 */
struct e_register* register_create(struct e_register*, int);

/** Fornisce il puntatore alla struttura
 * date dell'oggetto struct e_register fornito.
 * L'oggetto date indica il giorno cui è associato
 * il registro (che potrebbe essere il giorno
 * corrente oppure il successivo).
 *
 * Restituisce NULL in caso di errore e
 * il puntatore alla struttura date del
 * register.
 */
const struct tm* register_date(const struct e_register*);

/** Libera tutta la memoria associata a un registro
 * senza preoccuparsi di eseguire alcuna operazione
 * per garantire la persistenza dei dati o altro.
 */
void register_destroy(struct e_register*);

/** Restituisce un va valore non nullo se il
 * register è stato modificato dalla sua
 * inizializzazione o dall'ultima chiamata a
 * register_clear_dirty_flag.
 *
 * Coerentemente restituisce 0 nel caso l'argomento
 * non sia valido.
 */
int register_is_changed(const struct e_register*);

/** Azzera il flag che indica che il registro è
 * stato modificato dalla sua inizializzazione o
 * dall'ultima chiamata a questa funzione
 * register_clear_dirty_flag.
 *
 * Restituisce 0 in caso di successo e
 * -1 in caso di errore.
 */
int register_clear_dirty_flag(struct e_register*);

/** Aggiunge una entry a un registro.
 * L'aggiunta è fatta in maniera "safe", viene inserita
 * una copia cosicché non ci siano rischi se l'entry
 * orginale viene distrutta.
 */
int register_add_entry(struct e_register*, const struct entry*);

/** Dato un registro calcola il numero di
 * elementi del tipo specificato.
 *
 * Restituisce il totale calcolato oppure
 * -1 in caso di errore.
 */
int register_calc_type(const struct e_register*, enum entry_type);

#endif
