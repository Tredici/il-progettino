/** File con i test finali sui register.
 * Serializzazione e deserializzazione.
 * Cose così insomma.
 */

#include "../register.h"
#include "../commons.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/** ID del register che viene sottoposto
 * ai test di scrittura
 */
#define TEST_ID 33

/** Numero di entry da aggiungere
 * per i test
 */
#define ENTRY_NUM 5

static void printEntry(const struct entry* E)
{
    char buffer[32];
    register_serialize_entry(E, buffer,
                sizeof(buffer), ENTRY_SIGNATURE_OMITTED);
    printf("NEW: %s\n", buffer);
}

static void testFILEstdout(struct e_register* testR)
{
    printf("Test [ENTRY_SIGNATURE_OMITTED] su stdout:\n");
    if (register_serialize(stdout, testR, ENTRY_SIGNATURE_OMITTED) != 0)
        errExit("*** register_serialize ***\n");
    printf("Test [ENTRY_SIGNATURE_REQUIRED] su stdout:\n");
    if (register_serialize(stdout, testR, ENTRY_SIGNATURE_REQUIRED) != 0)
        errExit("*** register_serialize ***\n");
    printf("Test [ENTRY_SIGNATURE_OPTIONAL] su stdout:\n");
    if (register_serialize(stdout, testR, ENTRY_SIGNATURE_OPTIONAL) != 0)
        errExit("*** register_serialize ***\n");
}

static void testFILENO(struct e_register* testR)
{
    printf("Test [ENTRY_SIGNATURE_OMITTED] su STDOUT_FILENO:\n");
    if (register_serialize_fd(STDOUT_FILENO, testR, ENTRY_SIGNATURE_OMITTED) != 0)
        errExit("*** register_serialize_fd ***\n");
    printf("Test [ENTRY_SIGNATURE_REQUIRED] su STDOUT_FILENO:\n");
    if (register_serialize_fd(STDOUT_FILENO, testR, ENTRY_SIGNATURE_REQUIRED) != 0)
        errExit("*** register_serialize_fd ***\n");
    printf("Test [ENTRY_SIGNATURE_OPTIONAL] su STDOUT_FILENO:\n");
    if (register_serialize_fd(STDOUT_FILENO, testR, ENTRY_SIGNATURE_OPTIONAL) != 0)
        errExit("*** register_serialize_fd ***\n");
}

static void testSAVE(struct e_register* testR)
{
    char buffer[32];

    if (register_filename(testR, buffer, sizeof(buffer)) == NULL)
        errExit("*** register_filename ***\n");

    printf("Test register_flush:\n");
    switch (register_flush(testR, 0))
    {
    case -1:
        errExit("*** register_flush ***\n");
        break;
    case 0:
        printf("No file written!\n");
        break;
    default:
        printf("\tWritten file [%s]\n", buffer);
        break;
    }
    printf("OK: register_flush\n");
    /* controlla che il dirty flag sia a 0 */
    if (register_is_changed(testR))
        errExit("*** register_is_changed ***\n");
}

static void testClone(struct e_register* testR)
{
    struct e_register* copyR;

    printf("*********************************\n");
    printf("Test register_clone:\n");
    copyR = register_clone(testR);

    if (copyR == NULL)
        errExit("*** register_clone ***\n");

    /* copyR */
    printf("Prova a salvare su file il clone:\n");
    testSAVE(copyR);
    printf("Stampa il contenuto del clone:\n");
    testFILENO(copyR);

    printf("OK: register_clone\n");
    printf("*********************************\n");
    register_destroy(copyR);
}


static void testREAD(void)
{
    struct e_register* testR;

    printf("Test register_read.\n");

    /* deve fallire perché l'argomento è inesistente */
    printf("\tTest strict mode:\n");
    testR = register_read(ENTRY_NUM, NULL, 1);
    if (testR != NULL)
        errExit("*** register_read ***\n");
    printf("\tOK strict mode!\n");

    /* il file non esiste ma funziona perché non è
     * in strict mode */
    printf("\tTest non-strict mode:\n");
    testR = register_read(ENTRY_NUM, NULL, 0);
    if (testR == NULL)
        errExit("*** register_read ***\n");
    printf("\tOK non-strict mode!\n");
    register_destroy(testR); testR = NULL;

    /* stavolta il file esiste */
    printf("\tTest success:\n");
    testR = register_read(TEST_ID, NULL, 1);
    if (testR == NULL)
        errExit("*** register_read ***\n");
    printf("\tOK test success!\n");

    if (register_is_changed(testR))
        errExit("*** register_is_changed ***\n");
    printf("OK: register_read!\n");

    printf("Stampa copia:\n");    
    printf("Test [ENTRY_SIGNATURE_REQUIRED] su stdout:\n");
    if (register_serialize(stdout, testR, ENTRY_SIGNATURE_REQUIRED) != 0)
        errExit("*** register_serialize ***\n");

    printf("Distruzione copia\n");
    register_destroy(testR);
}

int main()
{
    struct e_register* testR;
    struct entry* E;
    int i;

    srand(time(NULL));

    printf("Test creazione registro:\n");
    testR = register_create(NULL, TEST_ID);
    if (testR == NULL)
        errExit("*** register_create ***\n");

    /* controlla che il dirty flag sia a 0 */
    if (register_is_changed(testR))
        errExit("*** register_is_changed ***\n");

    printf("Test aggiunta [%d] entry:\n", ENTRY_NUM);
    for (i = 0; i != ENTRY_NUM; ++i)
    {
        E = register_new_entry(NULL, (rand()&1 ?SWAB:NEW_CASE), 1+(rand()%50), 0);
        if (E == NULL)
            errExit("*** register_new_entry ***\n");

        printEntry(E);
        /* aggiunge  */
        if (register_add_entry(testR, E) != 0)
            errExit("*** register_add_entry ***\n");
        /* ora deve essere cambiato */
        if (!register_is_changed(testR))
            errExit("*** register_is_changed ***\n");
    }

    /* test con FILE* */
    testFILEstdout(testR);
    /* test con FILE* */
    testFILENO(testR);
    /* test salvataggio su file */
    testSAVE(testR);
    /* test copia */
    testClone(testR);

    printf("Distruzione registro\n");
    register_destroy(testR);
    testR = NULL;

    testREAD();

    return 0;
}
