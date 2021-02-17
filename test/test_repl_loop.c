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
    printf("\tcmd1: %s\n", str);
    return 0;
}
int cmd2(const char* str)
{
    printf("\tcmd2: %s\n", str);
    return 0;
}
int cmd22(const char* str)
{
    printf("\tcmd22: %s\n", str);
    return 0;
}
int cmd11(const char* str)
{
    printf("\tcmd11: %s\n", str);
    return 0;
}
int somma(const char* str)
{
    int a,b;
    sscanf(str, "%d %d", &a, &b);
    printf("\tsomma [\"%s\"]: %d + %d = %d\n", str, a, b, a+b);
    return 0;
}

static int stop(const char* str)
{
    (void) str;
    printf(" *** FINE *** \n");
    return OK_TERMINATE;
}

static void repl_404(const char* args)
{
    (void)args;
    fprintf(stderr, "NOOOOOOOO\n");
}

static void repeat(void)
{
    static int i=0;
    printf("LOOP - (%d)\n", i++);
}

int main()
{
    extern const char* repl_sign;
    extern void (*repl_not_found_f)
          (const char*);
    extern void (*repl_repeat)(void);
    repl_repeat  = &repeat;

    repl_not_found_f = &repl_404;

    repl_sign = ">>>";
    //repl_sign = NULL;
    struct repl_cmd_todo commands[] = {
        { "cmd1", &cmd1 },
        { "cmd11", &cmd11 },
        { "cmd2", &cmd2 },
        { "cmd22", &cmd22 },
        { "som", &pr_args },
        { "somm", &pr_args },
        { "somma", &somma },
        { "case", &pr_args },
        { "stop", &stop },
        { "", NULL}
    };

    repl_start("INPUT", commands, -1);

    return 0;
}
