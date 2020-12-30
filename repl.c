#include "repl.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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

