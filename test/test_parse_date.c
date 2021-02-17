#include "../time_utils.h"
#include <stdio.h>
#include <signal.h>
#include <time.h>

volatile sig_atomic_t stop;

int main(/*int argc, char const *argv[]*/)
{
    struct tm date;
    char line[65];

    while (!stop)
    {
        printf("Inserisci una data nel formato dd-mm-yyyy:>");
        scanf("%30s", line);
        if (time_parse_date(line, &date) == 0)
            printf("Data corretta!\n");
        else
            fprintf(stderr, "Malformed date!\n");
        strftime(line, sizeof(line), "%d-%m-%Y", &date);
        printf("Data (%s)\n", line);
    }

    return 0;
}
