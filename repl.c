/* macro per rendere disponibile getline */
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "repl.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

struct repl_cmd repl_recognise_cmd(const char* text,
    const struct repl_cmd_hint cmds[], int len)
{
    size_t cmdLen;
    struct repl_cmd ans;
    const struct repl_cmd_hint* cmd;
    const char* args;
    int i;

    ans.command = text;
    ans.flag = -1;

    for (cmd = &cmds[0], i = 0; (len == -1 || i < len) && cmd->command[0] != '\0'; ++cmd, ++i)
    {
        cmdLen = strlen(cmd->command);

        /* Test del comando */
        if (strncasecmp(text, cmd->command, cmdLen) == 0 && !isgraph(text[cmdLen]))
        {
            ans.flag = cmd->flag;
            
            for (args = &text[cmdLen]; *args != '\0' && 0 == isgraph(*args); ++args)
                ;
            
            ans.args = args;
            break;
        }

        ++cmds;
    }

    return ans;
}

struct repl_cmd_hint*
repl_make_hint_from_todo(const struct repl_cmd_todo* todos,
                        int len,
                        struct repl_cmd_hint* hints)
{
    struct repl_cmd_hint* ans;
    int i;

    if (hints == NULL)
    {
        if (len == -1) /* parametri inconsistenti */
        {
            return NULL;
        }
        ans = calloc(len, sizeof(struct repl_cmd_hint));
    }
    else
    {
        ans = hints;
    }

    for (i = 0; (i == -1 || i < len) && todos[i].command[0] != '\0'; ++i)
    {
        /* inizializza l'i-esimo "hint" */
        strncpy(ans[i].command, todos[i].command, REPL_CMD_MAX_L);
        /* vedi la descrizione della funzione */
        ans[i].flag = i;
    }

    return ans;
}

int repl_hints_lenght(struct repl_cmd_hint* hints)
{
    int counter;

    if (hints == NULL)
        return -1;

    counter = 0;
    while (hints[counter].command[0] != '\0')
        ++counter;

    return counter;
}

int repl_todos_lenght(struct repl_cmd_todo* todos)
{
    int counter;

    if (todos == NULL)
        return -1;

    counter = 0;
    while (todos[counter].command[0] != '\0')
        ++counter;

    return counter;
}


int repl_apply_cmd(const char* cmd, struct repl_cmd_todo* cmds, int len)
{
    struct repl_cmd_hint* hints;
    struct repl_cmd command;
    int index;
    int result;

    if (cmds == NULL)
        return -1;

    if (len == -1) /* per ora non si accettano queste cose */
        return -1;

    hints = repl_make_hint_from_todo(cmds, len, NULL);
    if (hints == NULL)
        return -1;

    command = repl_recognise_cmd(cmd, hints, len);
    free(hints);
    hints = NULL;

    /* indice del comando individuato */
    index = command.flag;
    if (index == -1)
    {
        errno = ENOSYS;
        return -1;
    }

    result = cmds[index].fun(command.args);

    if (result < 0)
        return -1;

    return result;
}

int repl_start(const char* msg, struct repl_cmd_todo* cmds, int len)
{
    int res;
    int repeat;
    char* line;
    size_t lineLen;
    size_t lineBytes;

    if (cmds == NULL)
    {
        return -1;
    }

    if (len == -1)
    {
        len = repl_todos_lenght(cmds);
        if (len == -1)
        {
            return -1;
        }
    }

    repeat = 1;
    while (repeat)
    {
        printf("%s>", msg != NULL ? msg : "");
        line = NULL;
        lineLen = 0;
        lineBytes = getline(&line, &lineLen, stdout);
        if (lineBytes == -1)
        {
            free(&line);
            return -1;
        }
        /* rimuove il carattere '\n' finale */
        line[lineBytes-1] = '\0';

        errno = 0;
        res = repl_apply_cmd(line, cmds, len);
        free(&line);

        if (res == -1 && errno == ENOSYS)
            res = WRN_CMDNF;

        switch (res)
        {
        case OK_CONTINUE:
            break;
        case OK_TERMINATE:
            repeat = 0;
            res = 0;
            break;
        case WRN_CONTINUE:
            break;
        case ERR_PARAMS:
            break;
        case WRN_CMDNF:
            break;

        default:
            if (res < 0)
            {
                repeat = 0;
            }
            else
            {
                fprintf(stderr, "repl_start: unexpected result.\n");
            }

            break;
        }
    }

    return res;
}
