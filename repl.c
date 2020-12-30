#include "repl.h"
#include <string.h>

struct repl_cmd repl_recognise_cmd(const char* text,
    const struct repl_cmd_hint cmds[])
{
    size_t cmdLen;
    struct repl_cmd ans;
    const struct repl_cmd_hint* cmd;
    const char* args;

    ans.command = text;
    ans.flag = -1;

    for (cmd = &cmds[0]; cmd != NULL; ++cmd)
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
