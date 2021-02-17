/** Breve programma che imita il lavoro fatto dal server.
 * Si limita a riconoscere i messaggi in arrivo e a
 * stampare a schermo un delle note che 
 */

#include "../commons.h"
#include "../socket_utils.h"
#include "../messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(void)
{
    int socket;
    struct sockaddr_storage ss;
    socklen_t skLen;
    char buffer[1024];
    ssize_t msgLen;
    /* per indicare da chi si ha ricevuto cosa */
    char rcvStr[32];
    uint32_t ID;
    
    socket = initUDPSocket(0);
    if (socket == -1)
        errExit("*** initUDPSocket ***\n");

    printf("socket listening on port [%d]\n", getSocketPort(socket));

    while (1)
    {
        skLen = sizeof(ss);
        if ((msgLen = recvfrom(socket, (void*)&buffer, sizeof(buffer), 0,
                    (struct sockaddr*) &ss, &skLen)) == -1)
        {
            perror("recvfrom");
            errExit("*** recvfrom ***\n");
        }

        if (sockaddr_as_string(rcvStr, sizeof(rcvStr), (struct sockaddr*) &ss, skLen) == -1)
            errExit("*** sockaddr_as_string ***\n");
        /* code */
        printf("Ricevuto messaggio da %s\n", rcvStr);

        switch (recognise_messages_type((void*)buffer))
        {
        case -1:
            errExit("*** recognise_messages_type ***\n");
            break;
        
        case MESSAGES_BOOT_REQ:
            printf("MESSAGES_BOOT_REQ\n");
            if (messages_check_boot_req(buffer, msgLen) == 0)
            {
                struct ns_host_addr* ns_addr;

                /* estrae il contenuto del messaggio */
                if (messages_get_boot_req_body(&ns_addr, (struct boot_req*)buffer) == -1)
                    errExit("*** messages_get_boot_ack_body ***\n");

                /* "stringhizza" */
                if (ns_host_addr_as_string(buffer, sizeof(buffer),ns_addr) == -1)
                    errExit("*** ns_host_addr_as_string ***\n");

                printf("Received: [%s]\n", buffer);

                if (ns_host_addr_any(ns_addr) == 1)
                {
                    printf("\t! Indirizzo 0 presente!\n");
                    if (ns_host_addr_update_addr(ns_addr, (struct sockaddr*)&ss) == -1)
                        errExit("*** ns_host_addr_update_addr ***\n");

                    if (ns_host_addr_as_string(buffer, sizeof(buffer),ns_addr) == -1)
                        errExit("*** ns_host_addr_as_string ***\n");

                    printf("\tSarà memorizzato: %s\n", buffer);
                }
                else
                    printf("\tIndirizzo normale.\n");

            }
            else
            {
                fprintf(stderr, "Malformed request.\n");
            }
            break;

        case MESSAGES_SHUTDOWN_REQ:
            printf("MESSAGES_SHUTDOWN_REQ\n");
            if (messages_check_shutdown_req(buffer, msgLen) == -1
                || messages_get_shutdown_req_body((struct shutdown_req*)buffer, &ID) == -1)
            {
                printf("Malformed request.\n");
                break;
            }
            printf("\tRichiesta valida: ID [%ld]\n", (long)ID);
            break;

        default:
            break;
        }
    }

    return 0;
}
