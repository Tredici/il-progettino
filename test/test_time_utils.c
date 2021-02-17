#include <stdio.h>
#include "../time_utils.h"
#include "../commons.h"

#define TEST_NUM 5

int main()
{
    /* per accedere ai dati */
    int y,m,d;
    struct tm date, inc, dec;
    int i;

    printf("Test time_utils:\n");
    
    printf("Test data di oggi:\n");
    date = time_date_today();
    time_date_read(&date, &y, &m, &d);
    printf("Oggi è il: %d-%d-%d\n", y,m,d);

    /* test uguaglianza */
    if (time_date_cmp(&date, &date))
        errExit("*** time_date_cmp ***\n");
    printf("OK: time_date_cmp.\n");

    printf("Test incremento:\n");
    inc = date;
    for(i = 0; i != TEST_NUM; ++i)
    {
        time_date_inc(&inc, 1);
        time_date_read(&inc, &y, &m, &d);
        printf("Tra %d sarà il: %d-%d-%d\n", i, y,m,d);
        /* confronto maggiore minore */
        if (time_date_cmp(&date, &inc) >= 0
            || time_date_cmp(&inc, &date) <= 0)
            errExit("*** time_date_cmp ***\n");
    }

    printf("Test decremento:\n");
    dec = date;
    for(i = 0; i != TEST_NUM; ++i)
    {
        time_date_dec(&dec, 1);
        time_date_read(&dec, &y, &m, &d);
        printf("Tra %d sarà il: %d-%d-%d\n", i, y,m,d);
    }

    printf("Test time_date_init:\n");
    date = time_date_init(2000, 10, 4);
    time_date_read(&date, &y, &m, &d);
    printf("Oggi è il: %d-%d-%d\n", y,m,d);

    return 0;
}
