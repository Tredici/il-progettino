#include "../ns_host_addr.h"
#include "../socket_utils.h"
#include "../commons.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>


int main(/*int argc, char* argv[]*/)
{
    /* fd del socket */
    int socket;
    int UDPort;
    uint16_t port;
    /* per i dati sul socket */
    struct sockaddr_storage ss;
    socklen_t size = sizeof(struct sockaddr_storage);

    struct ns_host_addr ns_addr;

    char buffer[64];

    socket = initUDPSocket(0);
    UDPort = getSocketPort(socket);
    printf("Opened socket on port (%d)\n", UDPort);

    /* GRUPPO 1 DI TEST */
    printf("TEST SERIALIZZAZIONE INDIRIZZO IP:\n");

    /* ottiene le informazioni sul socket */
    memset(&ss, 0, sizeof(ss));
    if (getsockname(socket, (struct sockaddr*) &ss, &size) != 0)
        errExit("*** getsockname() ***\n");

    memset(buffer, 0, sizeof(buffer));
    if (sockaddr_as_string(buffer, sizeof(buffer), (const struct sockaddr*)&ss, size) == -1)
        errExit("*** sockaddr_as_string() ***\n");
    printf("Inizio:\t%s\n", buffer);

    memset(&ns_addr, 0, sizeof(ns_addr));
    if (ns_host_addr_from_sockaddr(&ns_addr, (struct sockaddr*) &ss) == -1)
        errExit("*** ns_host_addr_from_sockaddr() ***\n");

    printf("[OK] ns_host_addr_from_sockaddr()\n");

    memset(buffer, 0, sizeof(buffer));
    if (ns_host_addr_as_string(buffer, sizeof(buffer), &ns_addr) == -1)
        errExit("*** ns_host_addr_as_string() ***\n");

    printf("ns_host_addr:\t%s\n", buffer);


    memset(&ss, 0, sizeof(ss));
    if (sockaddr_from_ns_host_addr((struct sockaddr*)&ss, &size, &ns_addr) == -1)
        errExit("*** sockaddr_from_ns_host_addr() ***\n");

    memset(buffer, 0, sizeof(buffer));
    if (sockaddr_as_string(buffer, sizeof(buffer), (const struct sockaddr*)&ss, size) == -1)
        errExit("*** sockaddr_as_string() ***\n");
    printf("Rewind:\t%s\n", buffer);
    printf("SUCCESSO!\n");

    /* GRUPPO 2 DI TEST */
    printf("TEST MODIFICA struct ns_host_addr:\n");
    if (ns_host_addr_get_port(&ns_addr, &port) == -1 || UDPort != (int)port)
        errExit("*** ns_host_addr_get_port() ***\n");

    UDPort = 5000;
    if (ns_host_addr_set_port(&ns_addr, UDPort) == -1)
        errExit("*** ns_host_addr_get_port() ***\n");

    if (ns_host_addr_get_port(&ns_addr, &port) == -1 || UDPort != (int)port)
        errExit("*** ns_host_addr_get_port() ***\n");
    printf("SUCCESSO!\n");

    /* GRUPPO 3 DI TEST */
    printf("TEST MODIFICA struct ns_host_addr:\n");
    switch (ns_host_addr_loopback(&ns_addr))
    {
    case 1:
        printf("IPv4: 0.0.0.0 oppure 0::0\n");
        break;

    case 0:
        printf("IPv4 normale\n");
        break;
    
    default:
        errExit("*** ns_host_addr_loopback() ***\n");
    }
    printf("SUCCESSO!\n");

    if (close(socket) == -1)
        errExit("*** close() ***\n");

    printf("DONE!\n");

    return 0;
}
