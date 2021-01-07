/*
 * Poter gestire delle liste è molto utile.
 * A tale fine creo l'equivalente di una
 * classe list<T>, sebbene sia molto minimale.
 */

#ifndef LIST
#define LIST

#include <stdlib.h> /* Definisce NULL */
#include <errno.h>

struct elem;
struct list;

/**Equivalente a un costruttore.
 * @params:
 *  +puntatore alla lista da inizializzare, NULL
 * @return:
 *  +puntatore alla lista inizializzata
 *
 * Inizializza una lista:
 *  -se l'argomento è NULL alloca internamente un
 *      oggetto list e lo restituisce
 *  -se l'argomento è valido inizializza 
 *      correttamente la struttura
 */
struct list* list_init(struct list*);

/**(Quasi) equivalente a un distruttore
 * elimina tutti gli elementi della lista
 * invocando sul contenuto l'apposita funzione di
 * cleanup, se specificata.
 * Riporta la lista allo stato iniziale
 */
struct list* list_clear(struct list*);

/** Come sopra ma in più libera la memoria
 *
 */
void list_destroy(struct list*);

/** Fornisce il numero di elementi nella lista
 *
 */
ssize_t list_size(const struct list*);

/** Per specificare l'eventuale funzione di cleanup
 *
 * Rermette di impostare una funzione di cleanup da
 * invocare su ogni elemento della lista in caso di
 * distruzione (clear) della stessa. Restituisce il
 * precedente handler.
 *
 */
void (*list_set_cleanup(struct list*, void(*)(void*)))(void*);

/**  Restituisce la funzione di cleanup corrente
 *
 */
void (*list_get_cleanup(const struct list*))(void*);


/** Accede all'elemento i esimo della lista e
 * restituisce il puntatore a esso
 */
void* list_get_item(struct list*, size_t);

/** Imposta un nuovo valore per l'elemento
 * n-esimo della lista e ritorna il valore
 * precedente
 */
void* list_set_item(struct list*, size_t, void*);

/** Itera lungo la lista ed applica ad ogni
 * elemento una data funzione.
 */
void list_foreach(struct list*, void (*)(void*));

/** Itera lungo la lista ed elimina tutti gli elementi
 * sui quali la funzione fornita restituisce un valore
 * non nullo, l'elemento viene semplicemente elimitato,
 * un eventuale cleanup non è eseguito di default e va
 * eseguito nella funzione fornita.
 *
 * Restituisce il numero di elementi eliminati o -1 in
 * caso di errore
 */
int list_eliminate(struct list*, int (*)(void*));


/** Itera lungo una lista ed esegue quello che ci si
 * aspetta da una funzione chiamata accumulate se si
 * ha un minimo di esperienza.
 * Il primo parametro della funzione argomento è il
 * puntatore al valore nella liste mentre il secondo
 * coincide con il terzo argomento fornito alla
 * funzione.
 */
void list_accumulate(struct list*, void (*)(void*, void*), void*);

/** Genera una lista con tanti elementi quanti ne ha
 * la lista fornita generandoli mediante la funzione
 * passata come secondo argomento.
 * Il terzo parametro, eventualmente NULL, è la
 * funzione da impostare come di cleanup per la nuova
 * lista che sta venendo creata.
 *
 * Restituisce NULL in caso di errore.
 *
 * Il nome è ispirato alla funzione (o metodo) map
 * presente in numerose librerie di altri linguaggi
 * che permette di otterenere un iterabile mappando
 * gli elementi di un altro.
 */
struct list* list_map(const struct list*, void* (*)(void*), void (*)(void*));

/** Genera una lista a partire da un'altra in cui
 * ogni elemento della nuova lista è ottenuto
 * elaborando due elementi consecutivi della lista
 * originale.
 * La lista risultato ha lunghezza minore di 1 di
 * quella originale, ragion per cui se la lista ha
 * meno di due elementi quella risultate è vuota.
 *
 * Restituisce NULL in caso di errore.
 */
struct list* list_reduce(const struct list*, void* (*)(void*,void*), void (*)(void*));

/** Copia una lista in un'altra.
 * La copia è fatta "per valore" (duplicando i
 * riferimenti) oppure tramite la funzione secondo
 * argomento se questa non è NULL.
 */
struct list* list_copy(const struct list*, void* (*)(void*));

/** Aggiunge un elemento in testa alla lista
 */
int list_prepend(struct list*, void*);

/** Aggiunge un elemento in coda alla lista
 */
int list_append(struct list*, void*);

#ifdef _LIST_DEBUG
/* Stampa il contenuto di una lista considerando
 * i puntatori come dei long
 */
void list_debug(struct list*);
#endif

#endif
