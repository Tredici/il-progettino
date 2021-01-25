#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include "peer_entries_manager.h"
#include "../thread_semaphore.h"
#include "../unified_io.h"
#include "../register.h"
#include "../list.h"
#include "../commons.h"
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
/* syscall specifica di linux, vediamo come va */
#include <sys/timerfd.h>

/** Segnale che si userà per richiedere al
 * sottosistema a gestione delle entries di
 * terminare.
 */
#define TERM_SUBSYS_SIGNAL SIGQUIT

/** MUTEX a guardia delle operazioni
 * sui registri.
 */
static pthread_mutex_t REGISTERguard = PTHREAD_MUTEX_INITIALIZER;
/** Lista dei registri a disposizione
 * del peer corrente.
 * In testa v'è quello corrispondente
 * al giorno indicato dalla variabile
 * da HEADdate
 */
static struct list* REGISTERlist;
/** data corrispondente all'oggetto
 * in testa alla lista di registri
 */
static struct tm HEADdate;

/** Identifica il peer quando si tratta
 * di salvare il contenuto dei registri
 * in dei file.
 *
 * È il numero fornito all'avvio del
 * processo da riga di comando.
 */
static int peerIDentifier;

/* handler farlocco */
static void fakeHandler(int x) {(void)x;}

static void newListHead(void)
{
    /* test della data */
    struct tm newDate;
    /* nuovo candidato testa */
    struct e_register* newHEAD;

    newHEAD = register_create(NULL, 0);
    if (newHEAD == NULL)
        errExit("*** ENTRIES:register_create ***\n");

    /* ottiene la data */
    newDate = *register_date(newHEAD);

    /* INIZIO SEZIONE CRITICA */
    if (pthread_mutex_lock(&REGISTERguard) != 0)
        errExit("*** ENTRIES:pthread_mutex_lock ***\n");

    unified_io_push(UNIFIED_IO_NORMAL,
        "Adding new register in date [%d-%d-%d]",
        newDate.tm_year+1900, newDate.tm_mon+1, newDate.tm_mday);

    /* prova a inserire il nuovo valore */
    if (list_prepend(REGISTERlist, (void*)newHEAD) != 0)
        errExit("*** ENTRIES:list_prepend ***\n");

    /* aggiorna la data di riferimeto */
    HEADdate = newDate;

    /* TERMINE SEZIONE CRITICA */
    if (pthread_mutex_unlock(&REGISTERguard) != 0)
        errExit("*** ENTRIES:pthread_mutex_lock ***\n");
}

/** Funzione a gestione del thread
 * che fa da guardia a register
 */
static void* entriesSubsystem(void* args)
{
    struct thread_semaphore* ts;
    /* descrittore di file da cui controllare
     * l'orario */
    int timerFd;
    /* per bloccare il segnale da ascoltare */
    sigset_t toBlock;
    /* per quando serve tenere il segnale sbloccato */
    sigset_t toSBlock;
    /* variabili per impostare lo scattare del
     * timer ogni giorno alle 18 */
    struct itimerspec start;
    /* struttura ausiliaria */
    struct tm startTM;
    /* per il polling */
    struct pollfd pollFd;
    /* test valore di ritorno */
    int err;

    ts = thread_semaphore_form_args(args);
    if (ts == NULL)
        errExit("*** ENTRIES ***\n");

    /* fa ciò che serve per bloccare il segnale */
    if (sigemptyset(&toBlock) != 0
        || signal(TERM_SUBSYS_SIGNAL, &fakeHandler) == SIG_ERR
        || sigaddset(&toBlock, TERM_SUBSYS_SIGNAL) != 0
        || pthread_sigmask(SIG_BLOCK, &toBlock, &toSBlock) != 0)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** ENTRIES ***\n");

    /* crea il descrittore per leggere timer */
    timerFd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerFd == -1)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** ENTRIES ***\n");

    /* dalle 18 di oggi ogni 24 ore */
    memset(&start, 0, sizeof(start));
    /* primo avvio */
    /* si appoggia all'oggetto HEADdate per il primo avvio */
    startTM = HEADdate; startTM.tm_hour = 18;
    start.it_value.tv_sec = mktime(&startTM); /* secondi da Epoch */

    /* ogni 24h - semplificazione che trascura il cambio di orario */
    start.it_interval.tv_sec = 24*60*60;

    if (timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &start, NULL) == -1)
        if (thread_semaphore_signal(ts, -1, NULL) == -1)
            errExit("*** ENTRIES ***\n");

    /* avvia sblocca il thread principale */
    if (thread_semaphore_signal(ts, 0, NULL) == -1)
        errExit("*** ENTRIES ***\n");

    unified_io_push(UNIFIED_IO_NORMAL, "Started entries subsystem!");

    /* loop continuo, fino allo spegnimento del sottosistema
     * che è comandato dal  */
    while (1)
    {
        memset(&pollFd, 0, sizeof(pollFd));
        pollFd.fd = timerFd;
        pollFd.events = POLLIN;
        errno = 0;
        err = ppoll(&pollFd, 1, NULL, &toSBlock);
        if (err != 1)
        {
            if (errno == EINTR)
                break;
            errExit("*** ENTRIES:ppoll ***\n");
        }
        /* nuova testa per l'elemento */
        newListHead(); /* se fallisce termina */
    }

    unified_io_push(UNIFIED_IO_NORMAL, "Terminating entries subsystem!");

    return NULL;
}

/* aggira il problema dei cast tra
 * puntatori a funzione */
static void healp_cleaner(void* ptr)
{
    /* non serve fare nulla */
    if (ptr == NULL)
        return;
    /* codice da aggiungere poi per serializzare
     * il contenuto del registro */

    register_destroy((struct e_register*)ptr);
}

/** Inizializza opportunamente l'oggetto
 * REGISTERlist.
 */
static int init_REGISTERlist(int port)
{
    /* candidata lista di registri */
    struct list* test_list;
    /* candidato registro di oggi */
    struct e_register* head;
    /* data di creazione del registro */
    struct tm test_date;

    /* creiamo la lista */
    test_list = list_init(NULL);
    if (test_list == NULL)
        return -1;

    (void)list_set_cleanup(test_list, &healp_cleaner);

    /* crea il registro */
    head = register_create(NULL, 0);
    /* controllo e prova a messa in lista */
    if (head == NULL || list_prepend(test_list, (void*)head) != 0)
    {
        free((void*)test_list);
        return -1;
    }

    /* prende la data */
    test_date = *register_date(head);

    /* svolge altre operazioni di configurazione */

    /* salva globalmente le informazioni */
    HEADdate = test_date;
    REGISTERlist = test_list;

    return 0;
}

/** Identificativo del thread che gestisce
 * i registri
 */
static pthread_t REGISTER_tid;

/* flag che indica se il sottosistema è stato avviato */
static sig_atomic_t started;

int startEntriesSubsystem(int port)
{
    /* crea i registri */
    if (init_REGISTERlist(port) != 0)
        return -1;

    /* avvia il subsystem */
    if (start_long_life_thread(&REGISTER_tid, &entriesSubsystem, NULL, NULL) == -1)
        return -1;

    /* accende il flag */
    started = 1;

    return 0;
}

int closeEntriesSubsystem(void)
{
    /* controlla il flag */
    if (!started)
        return -1;

    if (pthread_kill(REGISTER_tid, TERM_SUBSYS_SIGNAL) != 0)
        errExit("*** pthread_kill ***\n");

    if (pthread_join(REGISTER_tid, NULL) != 0)
        errExit("*** pthread_join ***\n");

    started = 0;

    return 0;
}


int addEntryToCurrent(const struct entry* E)
{
    struct e_register* currentRegister;

    /* controlla che il sottosistema sia stato avviato */
    if (!started)
        return -1;

    if (E == NULL)
        return -1;

    /* INIZIO SEZIONE CRITICA */
    if (pthread_mutex_lock(&REGISTERguard) != 0)
        return -1;

    /* ottiene il riferimento al registro di oggi */
    if (list_first(REGISTERlist, (void**)&currentRegister) != 0
        /* ora inserisce la entry nel registro */
        || register_add_entry(currentRegister, E) != 0)
    {
        pthread_mutex_unlock(&REGISTERguard);
        return -1;
    }

    /* FINE SEZIONE CRITICA */
    if (pthread_mutex_unlock(&REGISTERguard) != 0)
        return -1;

    return 0;
}
