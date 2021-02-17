/** Vediamo se sono in grado di creare un socket
 * passivo tcp.
 */

#include "../socket_utils.h"
#include "../commons.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int main()
{
    int sk, port;
    int pair;
    char buffer[32];
    ssize_t len;

    printf("Test socket TCP\n");

    sk = initTCPSocket(0, 10);
    if (sk == -1)
        errExit("*** initTCPSocket ***\n");

    port = getSocketPort(sk);
    if (port == -1)
        errExit("*** getSocketPort ***\n");

    printf("TCP socket listening on port (%d)\n", port);

    printf("Waiting for accept.\n");
    pair = accept(sk, NULL, NULL);
    if (pair == -1)
        errExit("*** accept ***\n");

    printf("Connection accepted!\n");
    /* chiude il socket principale */
    close(sk);

    while ((len = read(pair, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, (size_t)len);
    }

    return 0;
}
