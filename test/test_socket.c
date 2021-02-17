#include "../socket_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

volatile sig_atomic_t flag = 0;

void handler(int sig)
{
    flag = 1;
}

#define BUFF_LEN 128

int main()
{
    int ss, port;
    char buffer[BUFF_LEN+1];
    ssize_t len;

    //signal(SIGINT, handler);

    ss = initUDPSocket(0);
    if (ss == -1)
    {
    }

    port = getSocketPort(ss);
    if (port == -1)
    {
        fprintf(stderr, "getSocketPort()\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Opened socket (%d) on port (%d)\n", ss, port);

    while (flag == 0)
    {
        fprintf(stdout, "Waiting for data:\n");
        len = recv(ss, (void*) buffer, BUFF_LEN, 0);
        if (len == -1)
        {
            fprintf(stderr, "recv()\n");
            break;
        }
        buffer[len] = 0;
        fprintf(stdout, "received [%d]: %s\n", (int)len, buffer);
    }

    fprintf(stdout, "STOP\n");

    if (close(ss) != 0)
    {
        fprintf(stderr, "close()\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

