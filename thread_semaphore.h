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
