/** Funzioni ausiliarie per lavorare con le
 * date e i tempi.
 */

#ifndef TIME_UTILS
#define TIME_UTILS

#include <time.h>

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

#endif
