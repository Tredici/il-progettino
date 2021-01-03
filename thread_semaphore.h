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
