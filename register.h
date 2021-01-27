/** Funzioni ausiliarie per gestire i registri
 * nella prima versione sarà gestito tutto "in memory"
 */


#ifndef REGISTER
#define REGISTER

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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
 *  ENTRY_SIGNATURE_REQUIRED -> questa viene inserita sempre
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

/** Crea un nuovo registro associato alla data specificata.
 * È del tutto equivalente a register_create se l'argomento
 * struct tm* è NULL, altrimenti usa l'argomento per
 * impostare il giorno a cui il fa riferimento.
 */
struct e_register* register_create_date(struct e_register*, int, const struct tm*);

/** Restituisce il numero di entry contenute
 * nel registro.
 *
 * Restituisce -1 in caso di errore.
 */
ssize_t register_size(const struct e_register*);

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

/** Stampa tutti le entry contenute nel registro,
 * una per riga.
 *
 * Il primo elemento è il file pointer da utilizzare
 * per stampare le entry, il secondo il registro
 * di riferimento, il terzo è un flag che indica la
 * politica da seguire per quanto riguarda le
 * signature delle entry:
 *  +ENTRY_SIGNATURE_OMITTED  -> omette tutte le signature
 *  +ENTRY_SIGNATURE_REQUIRED -> stampa tutte le signature
 *      (fallisce se il register non ha una signature
 *      specificata come di default)
 *  +ENTRY_SIGNATURE_OPTIONAL -> stampa solo le signature
 *      diverse da quella come di default, funziona anche
 *      se il registro non ne ha alcuna specificata.
 *
 * Restituisce 0 in caso di successo e
 * -1 in caso di errore.
 */
int register_serialize(FILE*, const struct e_register*, enum ENTRY_SERIALIZE_RULE);

/** Come register_serialize ma utilizza un file descriptor
 * invece di un file pointer.
 *
 * Restituisce 0 in caso di successo e
 * -1 in caso di errore.
 */
int register_serialize_fd(int, const struct e_register*, enum ENTRY_SERIALIZE_RULE);

/** Scrive tutto il contenuto del registro su file,
 * il comportamento varia a seconda sel valore del
 * secondo argomento.
 *
 * Il registro deve avere specificato un valore
 * non nullo come signature di default.
 *
 * Il nome del file è così generato:
 *  "defaultSignature"."year"-"month"-"day".txt
 * Dove year,month,day indicano la data cui il
 * registro corrisponde.
 *
 * Se il secondo argomento è 0:
 *  +se il registro è vuoto ritorna immediatamente;
 *  +se il "dirty flag" è 0 ritorna immediatamente;
 *  +apre il file in scrittura troncandone tutto
 *      il contenuto e inizia a scriverci il contenuto
 *      del registro.
 *  +azzera il "dirty flag"
 *
 * Se il secondo argomento NON è 0:
 *  +apre il file in scrittura troncandone tutto
 *      il contenuto e inizia a scriverci il contenuto
 *      del registro.
 *  +azzera il "dirty flag"
 *
 * Restituisce 0 se non stampa nulla, 1 se scrive su
 * file il contenuto del registro e
 * -1 in caso di errore.
 */
int register_flush(struct e_register*, int);

/** Permette di ottenere il nome del file
 * associato a un dato register (se ne esiste uno).
 * Se l'utente fornisce un buffer il risultato è
 * salvato in esso, altrimenti è allocato in memoria
 * dinamica e lo spazio usato può essere liberato
 * con free.
 *
 * Se il secondo parametro è NULL il terzo argomento
 * è ignorato, altrimenti specifica la dimensione
 * del buffer.
 *
 * Restituisce NULL in caso di errore e l'indirizzo
 * del buffer utilizzato in caso di successo.
 */
char* register_filename(const struct e_register*, char*, size_t);

/** Svolge un lavoro complementare a quello
 * di register_serialize_fd leggendo al più
 * tante entry quante ne sono specificate
 * dall'ultimo parametro in un inserendole
 * nuovo registro.
 *
 * Affinché il tutto funzioni bene si
 * si aspetta che ci sia una entry da
 * riconoscere per riga.
 *
 * Il secondo parametro si applica solo se
 * l'ultimo è -1 ed è pensato specificatamente
 * per la lettura da file.
 *
 * Se l'ultimo argomento è -1 prosegue
 * fino a quando non incontra un EOF.
 *
 * Le entry duplicate sono seplicemente
 * scartate.
 *
 * Restituisce un puntatore al nuovo
 * registro in caso di successo e NULL
 * in caso di errore.
 */
struct e_register* register_parse(FILE*, enum ENTRY_SERIALIZE_RULE, int);

/** Come register_parse ma utilizza un fd
 * invece di un file pointer.
 */
struct e_register* register_parse_fd(int, enum ENTRY_SERIALIZE_RULE, int);

/** Svolge un lavoro complementare a quello
 * register_flush.
 *
 * Cerca di aprire un file il cui nome è nella forma
 *  "defaultSignature"."year"-"month"-"day".txt
 * (dove year,month,day indicano la data cui il
 * registro corrisponde) dove defaultSignature
 * (che corrisponde al primo parametro fornito)
 * deve essere un intero positivo e gli altri
 * tre sono ricavati dal puntatore all'oggetto struct
 * tm fornito, a meno che questo non sia NULL, nel qual
 * caso vengono utilizzati i dati del giorno corrente.
 *
 * Il terzo parametro è un flag che se non nullo
 * specifica che la funzione deve fallire se il
 * file riusulta inesistente.
 *
 * Restituisce un puntatore al nuovo
 * registro in caso di successo o NULL
 * in caso di errore.
 */
struct e_register* register_read(int, const struct tm*, int);

/** Dopo aver controllato che ambo i registri
 * si riferiscono allo stesso giorno, altrimenti
 * si ha un errore, aggiunge al primo registro
 * tutte le entry del secondo le cui firme non
 * sono inizialmente note al primo.
 *
 * Si assume che se un registro possiede
 * una entry con una data signature allora
 * possiede tutte le entry con la medesima
 * e permettere l'inserimento di quelle con
 * la medesima signature causerebbe la
 * creazione di duplicati non desiderati.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int register_merge(struct e_register*, const struct e_register*);

/** Genera una copia del registro dato.
 * Utile per implementare altre funzioni.
 *
 * Restituisce la copia in caso di successo
 * e NULL in caso di errore.
 */
struct e_register* register_clone(const struct e_register*);

/** "Chiude" un registro, ovvero pone su di esso
 * l'assunzione che contenga tutte le entry del
 * giorno cui si riferisce.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int register_close(struct e_register*);

/** Verifica se il registro sia stato chiuso.
 * Restituisce 1 in caso affermativo e 0 in
 * caso negativo.
 *
 * Restituisce -1 in caso di errore.
 */
int register_is_closed(const struct e_register*);

#endif
