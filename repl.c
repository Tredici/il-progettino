#include "repl.h"
#include <string.h>
#include <ctype.h>

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

    for (cmd = &cmds[0], i = 0; (len == -1 || i < len) && cmd != NULL; ++cmd)
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
