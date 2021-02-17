#include "../repl.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* print arguments */
int pr_args(const char* str)
{
    printf("\tpr_args: strlen(\"%s\")=(%d)\n", str, (int)strlen(str));
    //puts(str);
    return 0;
}
int cmd1(const char* str)
{
    printf("\tcmd1:\n", str);
    return 0;
}
int cmd2(const char* str)
{
    printf("\tcmd2:\n", str);
    return 0;
}
int cmd22(const char* str)
{
    printf("\tcmd22:\n", str);
    return 0;
}
int cmd11(const char* str)
{
    printf("\tcmd11:\n", str);
    return 0;
}
int somma(const char* str)
{
    int a,b;
    sscanf(str, "%d %d", &a, &b);
    printf("\tsomma [\"%s\"]: %d + %d = %d\n", str, a, b, a+b);
    return 0;
}

int main()
{
    int i, res;
    char* test;
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
    struct repl_cmd_todo commands[] = {
        { "cmd1", &cmd1 },
        { "cmd11", &cmd11 },
        { "cmd2", &cmd2 },
        { "cmd22", &cmd22 },
        { "som", &pr_args },
        { "somm", &pr_args },
        { "somma", &somma },
        { "case", &pr_args },
        { "", NULL}
    };

    for (i = 0, test = tests[i]; test != NULL; test = tests[++i])
    {
        printf("[%s]:\n", test);
        res = repl_apply_cmd(test, commands, repl_todos_lenght(commands));
        if (res == -1)
        {
            puts("\t404 - NOT FOUND!");
        }
        //puts("");
    }

    return 0;
}
