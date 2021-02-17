#include "../commons.h"
#include "../socket_utils.h"
#include "../messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARGNAME "porta"

void usageHelp(const char* p)
{
    printf("Usage:\n\t%s <" ARGNAME ">\n", p);

    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int socketfd;
    struct sockaddr_storage ss;
    socklen_t ssLen;
    uint32_t ID;

    srand(time(NULL));

    if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usageHelp(argv[0]);

    socketfd = initUDPSocket(0);
    if (socketfd == -1)
        errExit("*** initUDPSocket ***\n");

    if (getSockAddr((struct sockaddr*)&ss, &ssLen, "localhost", argv[1], SOCK_DGRAM, 0) == -1)
        errExit("*** getSockAddr ***\n");

    ID = rand() % 1000000;
    printf("Sending shutdown req with ID [%ld]\n", (long)ID);

    if (messages_send_shutdown_req(socketfd, (struct sockaddr*)&ss, ssLen, ID) == -1)
        errExit("*** messages_send_shutdown_req ***\n");

    printf("Richiesta inviata.\n");

    return 0;
}
