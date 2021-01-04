/** Ispirata (quello che mi ricordo di) std::set
 * questa struttura dati permette di gestire
 * facilmente alcune operazioni basilari su
 * insiemi di numeri interi
 */


#ifndef SET
#define SET

#include <stdlib.h>

struct set;

struct set* set_init(struct set*);
struct set* set_clear(struct set*);
/* da chiamare SEMPRE al termine dell'utilizzo */
void set_destroy(struct set*);


/* inserimento test e rimozione */
int set_add(struct set*, long int);
int set_check(struct set*, long int);
int set_remove(struct set*, long int);

/* size */
ssize_t set_size(struct set*);


#endif
