/**
 * Raccolta di funzioni necessarie per permettere 
 * a più thread di eseguire operazioni di io insieme
 * risolvendo tutti i problemi di condivisione del 
 * canale di io
 *
 * Queste funzioni fanno un grosso utilizzo dei
 * segnali standard quindi vanno usate con
 * consapevolezza delle possibili interferenze.
 */


#ifndef MULTI_IO
#define MULTI_IO

/**
 * Inizializza le cose per gestire l'io insieme a più processi
 *
 * 0 on success
 * nonzero on error
 *
 * ! ATTENZIONE a chiamarla da thread secondari
 */
int multiio_start();

/**
 * Ripristina lo status quo, da chiamare alla fine dell'utilizzo
 */
int multiio_end();


#endif


