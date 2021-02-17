#include "../repl.h"
#include <stdio.h>
#include <stdlib.h>

enum CMD
{
    CMD1,
    CMD11,
    CMD2,
    SOMMA,
    CASE,
    NOTCMD
};

void stampa(const char* type, struct repl_cmd* res)
{
    printf("[%s]\tcmd:\t\"%s\"\n", type, res->command);
    printf("[%s]\targs:\t\"%s\"\n", type, res->args);
}

/* print arguments */
int pr_args(const char* str)
{
    puts(str);
    return 0;
}

int main()
{
    char* tests[] = {
        "cmd1",
        "cmd11",
        "cmd2 a1 a2 a3",
        "somma 2 3",
        "Filssomma 2 3",
        "case",
        "CASE",
        "CaSe",
        "cAsE",
        "cAse",
        "casE e",
        "Case",
        (char*) NULL
    };

    struct repl_cmd_hint hints[] = {
        { "cmd1", CMD1 },
        { "cmd11", CMD11 },
        { "cmd2", CMD2 },
        { "cmd22", NOTCMD },
        { "som", NOTCMD },
        { "somm", NOTCMD },
        { "somma", SOMMA },
        { "case", CASE }
    };

    struct repl_cmd_hint* todo_hints;
    struct repl_cmd_todo todo[] = {
        { "cmd1", pr_args },
        { "cmd11", pr_args },
        { "cmd2", pr_args },
        { "cmd22", pr_args },
        { "som", pr_args },
        { "somm", pr_args },
        { "somma", pr_args },
        { "case", pr_args }
    };

    const char* test;
    struct repl_cmd res;
    int i;

    todo_hints = repl_make_hint_from_todo(todo, sizeof(todo)/sizeof(todo[0]), NULL);

    for (i = 0, test = tests[i]; test != NULL; test = tests[++i])
    {
        res = repl_recognise_cmd(test, hints, sizeof(hints)/sizeof(hints[0]));
        printf("%s:\n", test);
        switch (res.flag)
        {
        case CMD1:
            /* code */
            stampa("CMD1", &res);
            break;
        case CMD2:
            /* code */
            stampa("CMD2", &res);
            break;
        case CMD11:
            /* code */
            stampa("CMD11", &res);
            break;
        case SOMMA:
            /* code */
            stampa("SOMMA", &res);
            break;
        case NOTCMD:
            /* code */
            stampa("NOTCMD", &res);
            break;
        case CASE:
            /* code */
            stampa("CASE", &res);
            break;
        
        default:
            /* NOT FOUND */
            printf("\tNO!\n");
            break;
        }
        puts("");
    }

    return 0;
}
