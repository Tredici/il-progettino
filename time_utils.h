/** Funzioni ausiliarie per lavorare con le
 * date e i tempi.
 */

#ifndef TIME_UTILS
#define TIME_UTILS

#include <time.h>
#include <arpa/inet.h>

/** Struttura che permette di rappresentare
 * un oggetto di tipo struct tm in maniera
 * "network safe"
 */
struct ns_tm
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
};

/** Inizializza un oggetto di tipo struct ns_tm
 * con il contenuto di un oggetto struct tm.
 *
 * Restituisce 0 in caso di
 * successo e -1 in caso di
 * errore.
 */
int time_init_ns_tm(struct ns_tm*, const struct tm*);

/** Inizializza un oggetto di tipo struct tm
 * con il contenuto di un oggetto struct ns_tm.
 *
 * Restituisce 0 in caso di
 * successo e -1 in caso di
 * errore.
 */
int time_read_ns_tm(struct tm*, const struct ns_tm*);

/** Inizalizza e fornisce una struct tm
 * con data e orario correnti in accordo
 * con la locale (timezone) dell'utente.
 */
struct tm time_date_now(void);

/** Inizalizza e fornisce una struct tm
 * con la data di oggi.
 */
struct tm time_date_today(void);

/** Inizializza e restituisce un oggetto
 * di tipo struct tm con l'anno il mese
 * e il giorno forniti.
 *
 * Per il mese 1 indica gennaio e 12
 * indica dicembre.
 * Valori "sballati" saranno normalizzati
 * in automatico.
 */
struct tm time_date_init(int, int, int);

/** Preleva rispettivamente anno, mese e giorno
 * dalla struttura fornita se i puntatori
 * dati non sono NULL.
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 */
int time_date_read(const struct tm*, int*, int*, int*);

/** Copia solo la data dalla prima
 * struttura alla seconda.
 *
 * Restituisce 0 in caso di successo
 * e -1 in caso di errore.
 */
int time_copy_date(struct tm*, const struct tm*);

/** Confronta due oggetti di tipo
 * struct tm facendo un confronto
 * esclusivamente sulla data.
 *
 * Restituisce 0 se le due date
 * corrispondono, un valore negativo
 * se la prima è antecedente, un
 * valore positivo se la seconda è
 * antecedente.
 * 
 * In caso di errore restituisce 0
 * e imposta errno a EINVAL
 */
int time_date_cmp(const struct tm*, const struct tm*);

/** Restituisce il numero di giorni di differenza
 * tra la prima data e la seconda data - guarda
 * solo i campi giorno-mese-anno.
 *
 * Restituisce 0 in caso di errore
 * e imposta errno a EINVAL
 */
int time_date_diff(const struct tm*, const struct tm*);

/** Svolge un ruolo complementare a quello di
 * time_date_diff sommando (in senso algebrico, +-)
 * tanti giorni quanti quelli specificati all'oggetto
 * struct tm fornito.
 *
 * In caso di errore (argomento NULL) fornisce una
 * struttura tutti i cui campi sono stati azzerati.
 */
struct tm time_date_add(const struct tm*, int);

/** Incrementa, dopo aver isolato solo i
 * campi relativi a una data, la data
 * fornita di tanti giorni quanti sono
 * quelli specificati.
 *
 * In caso di successo fornisce il
 * puntatore all'oggetto struct tm
 * fornito, in caso di errore restituisce
 * NULL.
 */
struct tm* time_date_inc(struct tm*, int);

/** Decrementa, dopo aver isolato solo i
 * campi relativi a una data, la data
 * fornita di tanti giorni quanti sono
 * quelli specificati.
 *
 * In caso di successo fornisce il
 * puntatore all'oggetto struct tm
 * fornito, in caso di errore restituisce
 * NULL.
 */
struct tm* time_date_dec(struct tm*, int);

/** Estrae un oggetto data da una stringa.
 * Formato atteso:
 *  dd:mm:yyyy
 *
 * Restituisce 0 in caso di successo e -1
 * in caso di errore.
 * In caso di errore non modifica
 * l'oggetto puntato da date.
 */
int time_parse_date(const char* str, struct tm* date);

/** Converte oggetto data in una stringa
 * nel formato:
 *  dd:mm:yyyy
 *
 * Il buffer fornito deve avere dimensione
 * almeno pari a 11 (al più dieci caratteri
 * per la data più uno per il carattere '\0').
 *
 * Restituisce il puntatore al buffer in
 * caso di successo e NULL in caso di errore.
 */
char* time_serialize_date(char* str, const struct tm* date);

#endif
