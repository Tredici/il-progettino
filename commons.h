/**
 * Raccolta di funzioni generiche utili in vari progetti
 *
 * - parsing di numeri dagli argomenti,
 * - terminazione "allegra" in caso di errore irreparabile
 */


#ifndef COMMONS
#define COMMONS

/* sempre utili */
int min(int x, int y);
int max(int x, int y);

/* Mostra un messaggio di errore e il termina il programma  */
void errExit(const char *format, ...);

/** Come fprintf(stderr, const char *format, ...)
 * ma stampa il risultato in verde.
 * Fa uso delle opportune sequenze di escape ASII
 */
void printSuccess(const char *format, ...);


/** Come fprintf(stderr, const char *format, ...)
 * ma stampa il risultato i rosso.
 * Fa uso delle opportune sequenze di escape ASII
 */
void printError(const char *format, ...);

/* Qualcosa di gravissimo che mina la consistenza dello
 * stato del programma è appena avvenuto - stampa varie
 * informazioni utili al debugging dell'applicazione e
 * termina forzatamente l'esecuzione invocando abort() */
void fatal(const char *format, ...);

/* Se l'argomento è 0 si blocca fino a che l'utente
 * non preme un tasto qualsiasi, se l'argomento non
 * è zero (si consiglia di usare 1) esegue un test
 * della possibilità di leggere dell'input e ritorna
 * immediatamente */
int waitForInput(int);

/** Funzione deputata al parsing di un argomento di tipo
 * intero ottenuto da linea di comando.
 *
 * Il primo argomento deve essere la stringa di cui fare
 * il parsing, il secondo è il nome dell'argomento che
 * l'utente avrebbe dovuto fornire, utilizzato per
 * comporre il messaggio da mostrare qualora il parsing
 * fallisca.
 *
 * In caso di errore invoca errExit e termina
 * l'esecuzione.
 */
int argParseInt(const char*, const char*);

/** Come argParseInt ma permette di specificare un range
 * [min;max] entro cui deve ricadere l'argomento.
 */
int argParseIntRange(const char*, const char*, int, int);

#endif

