/**
 * Raccolta di funzioni generiche utili in vari progetti
 * 
 * - parsing di numeri dagli argomenti,
 * - terminazione "allegra" in caso di errore irreparabile
 */


#ifndef COMMONS
#define COMMONS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> /* per un numero variabile di argomenti */

/* Mostra un messaggio di errore e il termina il programma  */
void errExit(const char *format, ...);

#endif

