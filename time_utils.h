/** Funzioni ausiliarie per lavorare con le
 * date e i tempi.
 */

#ifndef TIME_UTILS
#define TIME_UTILS

#include <time.h>

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


#endif
