#include "../messages.h"
#include "../socket_utils.h"
#include "../commons.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

static void usageHelp(const char* prog)
{
    printf("Usage:\n\t%s <porta>\n", prog);
    exit(0);
}

int main(int argc, char const *argv[])
{
    int sk;
    int peer;
    struct sockaddr_storage ss;
    socklen_t ssLen;
    char buffer[128];
    ssize_t bufLen;

    if (argc != 2)
        usageHelp(argv[0]);

    if (getSockAddr((struct sockaddr*)&ss, &ssLen, "localhost", argv[1], 0, 0) != 0)
        errExit("*** getSockAddr ***\n");

    sk = initUDPSocket(0);
    if (sk == -1)
        errExit("*** initUDPSocket ***\n");

    do
    {
        printf("port> ");
        if (scanf("%d", &peer) <= 0)
        {
            perror("Errore nell'input!");
            fflush(stdin);
            continue;
        }
        
        if (!peer)
            break;
        printf("Sending message [MESSAGES_CHECK_REQ]\n");
        if (messages_send_check_req(sk, (struct sockaddr*)&ss, ssLen, peer) != 0)
            errExit("*** messages_send_check_req ***\n");

        bufLen = recv(sk, buffer, sizeof(buffer), 0);
        if (messages_check_check_ack(buffer, (size_t)bufLen) != 0)
        {
            printf("Malformed message!\n");
            continue;
        }
        printf("Received message [MESSAGES_CHECK_ACK]\n");
    } while (1);

    if (close(sk) != 0)
        errExit("*** close ***\n");

    /* code */
    return 0;
}

