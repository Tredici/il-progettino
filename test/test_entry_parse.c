#include "../register.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test(char* group[], int success)
{
    char* p, *p2;
    struct entry* T;

    while (*group != NULL)
    {
        p = *group++;
        T = register_parse_entry(p, NULL, ENTRY_SIGNATURE_OPTIONAL);
        
        if ((T == NULL && success == 1) ||
            (T != NULL && success == 0))
        {
            fprintf(stderr, "*** DISASTRO (%s) ***\n", p);
            return 0;
        }

        if (success)
        {
            p2 = register_serialize_entry(T, NULL, 0, ENTRY_SIGNATURE_OPTIONAL);
            if (strcasecmp(p, p2))
            {
                fprintf(stderr, "*** DISASTRO register_serialize_entry (%s) ***\n", p);
                return 0;
            }
            printf("%s\n", p2);
        }
    }

    return 1;
}

int main()
{
    char* test_success[] = {
            "2020-12-29|n|5",
            "2020-12-29|N|5[1]",
            "2020-12-30|T|5[123]",
            "2020-12-30|t|5[12357]",

            (char*)NULL
        };
    char* test_fail[] = {
            "2020-13-29|n|5",
            "2020-02-32|n|5",

            (char*)NULL
        };

    if (!test(test_success, 1))
    {
        fprintf(stderr, "*** DISASTRO TEST SUCCESS ***\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST SUCCESS SUPERATI!\n");
    
    if (!test(test_fail, 0))
    {
        fprintf(stderr, "*** DISASTRO TEST FAIL ***\n");
        exit(EXIT_FAILURE);
    }
    printf("TEST FAIL SUPERATI!\n");

    return 0;
}
