/**
 * Raccolta di funzioni generiche utili in vari progetti
 *
 * - parsing di numeri dagli argomenti,
 * - terminazione "allegra" in caso di errore irreparabile
 */


#ifndef COMMONS
#define COMMONS

/* Mostra un messaggio di errore e il termina il programma  */
void errExit(const char *format, ...);

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
 */
int argParseInt(const char*, const char*);

#endif

