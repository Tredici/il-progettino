/** Ispirata (quello che mi ricordo di) std::set
 * questa struttura dati permette di gestire
 * facilmente alcune operazioni basilari su
 * insiemi di numeri interi
 */


#ifndef SET
#define SET


struct set;

struct set* set_init(struct set*);
struct set* set_clear(struct set*);
/* da chiamare SEMPRE al termine dell'utilizzo */
void set_destroy(struct set*);

#endif
