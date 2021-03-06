/** Mi sono reso conto mi servisse un modo
 * semplice ed efficiente per dare vita a
 * long-life thread e assicurarmi che questi
 * si avviassero correttamente.
 *
 * La struttura che viene qui sviluppata
 * serve appunto a bloccare il thread
 * creatore fino a che il figlio non si
 * assicura di essere partito correttamente
 * (o meno) e lo notifica al padre.
 */

#ifndef THREAD_SEMAPHORE
#define THREAD_SEMAPHORE

#include <pthread.h>

/** Semaforo sul quale si blocca il padre
 * in attesa che il figlio faccia qualcosa
 */
struct thread_semaphore;

/** Crea e inzializza il semaforo.
 * Solo il padre deve invocare questa
 * funzione.
 */
struct thread_semaphore* thread_semaphore_init();

/** Attende che il semaforo sia configurato
 * - "acceso" - dal figlio.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 *
 * SOLO il thread padre dovrebbe chiamarla.
 */
int thread_semaphore_wait(struct thread_semaphore*);

/** Fornisce lo stato del semaforo.
 * NON deve essere mai chiamato prima di
 * thread_semaphore_wait.
 * Restituisce lo stato in cui il figlio ha
 * lasciato il semaforo.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 *
 * SOLO il thread padre dovrebbe chiamarla.
 */
int thread_semaphore_get_status(struct thread_semaphore*, int*, void**);

/** Libera tutte le risorse allocate per il
 * semaforo.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 *
 * SOLO il thread padre dovrebbe chiamarla.
 */
int thread_semaphore_destroy(struct thread_semaphore*);

/** Assegna facoltativamente uno stato e dei
 * valori al semaforo e lo imposta come "acceso"
 * cosicché il padre sia notificato dello
 * stato del figlio.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 *
 * SOLO il figlio deve chiamarla.
 */
int thread_semaphore_signal(struct thread_semaphore*, int, void*);

/** Avvia un thread destinato a durare a tempo
 * indefinito.
 *
 * L'avvio viene effettuato utilizzando le funzioni
 * thread_semaphore_* quindi la funzione del thread
 * deve segnalare al chiamante il suo corretto avvio.
 *
 * Se il primo parametro non è NULL fornisce l'id
 * del thread creato senza ulteriori operazioni
 * dietro ALTRIMENTI il thread viene distaccato
 * ("detatched").
 *
 * Il thread creato deve lasciare 0 nella variabile
 * di stato del semaforo per segnalare un corretto
 * avvio, qualsiasi altro valore sarà interpretato
 * come indice di un'avvio fallimentare e indice
 * della terminazione precoce del nuovo figlio.
 *
 * Il penultimo argomento permette di passare alla
 * funzione un parametro come void*.
 * La funzione argomento riceverà come argomento
 * un puntatore a un void*[2] che però è locale
 * a start_long_life_thread e conterrà il valore
 * argomento nel suo secondo elemento, che andrà
 * copiato prima di segnalare il semaforo se lo
 * si vorrà utilizzare in seguito.
 *
 * Utilizza l'ultimo argomento per fornire al
 * chiamante i dati passati dal thread creato,
 * se questo non è NULL.
 *
 * Restituisce 0 in caso di successo e -1 in
 * caso di errore.
 */
int start_long_life_thread(pthread_t*, void*(*)(void *), void*, void**);

/** Da invocare da un thread creato con
 * start_long_life_thread fornendoci
 * l'argomento (void* args):
 *  restituisce al chiamanete il
 *  punatatore al semaforo
 */
struct thread_semaphore* thread_semaphore_form_args(void*);

/** Ricava gli argomenti passati a un
 * thread creato tramite
 * start_long_life_thread
 * dall'parametro (void*) della funzione
 * che costituisce di questo
 */
void* thread_semaphore_get_args(void*);

#endif
